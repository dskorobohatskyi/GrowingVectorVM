// GrowingVectorVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <assert.h>
#include <optional>

#include <stdint.h>
#include <windows.h>
#include <memoryapi.h>

#include <sysinfoapi.h>                 // for GetPhysicallyInstalledSystemMemory


#define KB(x) (x) * (size_t)1024
#define MB(x) (KB(x)) * 1024
#define GB(x) (MB(x)) * 1024

static_assert(KB(1) == 1024);
static_assert(KB(4) == 4096);
static_assert(MB(2) == 1'048'576 * 2);
static_assert(GB(4) == 4'294'967'296);


// TODO read TLB, TLB miss, physical memory access optimization, 512 times less of page faults and TLB misses
// TODO natvis for GrowingVector

// TODO test on smth bigger than int (probably with alignas())
// TODO carry out class logic to separate file
// TODO internal reserve should be bigger than user requests (user's is about commited memory)
// TODO to support large pages CommitPagesWithReserve option should work
template<class T, bool LargePagesEnabled = false, bool CommitPagesWithReserve = false || LargePagesEnabled>
class GrowingVectorVM
{
public:
    using ElementType = T;
    constexpr static size_t ElementSize = sizeof(T);

    GrowingVectorVM()
        : m_data(nullptr)
        , m_size(0)
        , m_committedPages(0)
        , m_reservedPages(0)
    {
        if constexpr (!LargePagesEnabled)
        {
            SYSTEM_INFO sSysInfo;                   // Useful information about the system
            GetSystemInfo(&sSysInfo);               // Initialize the structure.

            m_pageSize = sSysInfo.dwPageSize;       // Page size on this computer
        }
        else
        {
            m_pageSize = GetLargePageMinimum(); // 2MB on my machine 
        }
        assert(m_pageSize % ElementSize == 0); // TODO get rid of this assert (keep it as reminder now)
    }
    ~GrowingVectorVM()
    {
        const bool bSuccess = VirtualFree(
            m_data,                     // Base address of block
            0,                          // Bytes of committed pages, 0 for MEM_RELEASE, non-zero for DECOMMIT
            MEM_RELEASE);               // Decommit the pages
        assert(bSuccess);
    }

    [[nodiscard]] size_t GetSize() const { return m_size; }
    [[nodiscard]] size_t GetCapacity() const { return CalculateObjectAmountForNBytes(GetCommittedBytes()); };
    [[nodiscard]] size_t GetReserve() const { return CalculateObjectAmountForNBytes(GetReservedBytes()); };

    // TODO make it private
    bool ReserveBytes(size_t requestedBytes)
    {
        // TODO check if current capacity already covers requestedBytes

        const size_t requiredPages = CalculatePageCount(requestedBytes, m_pageSize, m_pageSize);
        const size_t totalSize = requiredPages * m_pageSize;

        DWORD allocationFlags = MEM_RESERVE;
        DWORD protectionFlags = PAGE_NOACCESS;
        if constexpr (CommitPagesWithReserve)
        {
            allocationFlags |= MEM_COMMIT;
            protectionFlags = PAGE_READWRITE;
        }

        // Large-page memory must be reserved and committed as a single operation.
        // In other words, large pages cannot be used to commit a previously reserved range of memory.
        if constexpr (LargePagesEnabled)
        {
            allocationFlags |= MEM_LARGE_PAGES;
        }
        // lpvBase - Base address of the allocated memory
        LPVOID lpvBase = VirtualAlloc(
            nullptr,                    // System selects address
            totalSize,                  // Size of allocation
            allocationFlags,            // Allocate reserved pages
            protectionFlags);

        if (lpvBase == nullptr)
        {
            return false;
        }

        // Guiding by Exception Safety Guarantee, let's modify state of the object only if everything goes successfully
        m_data = reinterpret_cast<ElementType*>(lpvBase);

        m_reservedPages = requiredPages;
        m_committedPages = CommitPagesWithReserve ? m_reservedPages : 0;

        std::cout << "GrowingVectorVM preallocated " << totalSize << " bytes of vm for " << GetCapacity() << " objects\n";

        return true;
    }

    bool Reserve(size_t elementAmount)
    {
        // TODO validate that ElementSize is aligned
        return ReserveBytes(elementAmount * ElementSize); // TODO CommitMemory should be used here
    }

    [[nodiscard]] inline bool Empty() const { return GetSize() == 0; }

    const ElementType& operator[](size_t index) const
    {
        return const_cast<const GrowingVectorVM*>(this)->operator[](index);
    }

    ElementType& operator[](size_t index)
    {
        if (index >= GetSize())
        {
            throw std::out_of_range{ "operator[] failed" }; // TODO extend with useful info
        }

        return m_data[index];
    }

    const ElementType& At(size_t index, const ElementType& defValue) const
    {
        if (index >= GetSize())
        {
            return defValue;
        }

        return this->operator[](index);
    }

    void PushBack(const ElementType& value)
    {
        if (GetSize() >= GetCapacity())
        {
            if (!CommitAdditionalPage())
            {
                // The container couldn't allocate one more virtual page
                throw std::bad_array_new_length();
            }
            assert(GetSize() < GetCapacity());
        }

        //if constexpr (std::is_trivially_copyable_v<ElementType>) // << no boost noticed
        //{
        //    memcpy(&m_data[GetSize()], &value, ElementSize);
        //}
        //else
        {
            new (&m_data[GetSize()]) ElementType(value);
        }
        ++m_size;
    }

    // void PushBack(const ElementType&& value)


    // TODO provide non-const versions of [] and At
    // TODOs:
    // Emplace
    // Erase (by value, index, iterator)

    // compatibility with stl, generate, transform algorithms for ex.

private:
    bool CommitAdditionalPage()
    {
        if (GetCommittedBytes() + m_pageSize > GetReservedBytes())
        {
            // TODO think about attempt to extend it
            //m_reservedPages = requiredPages;
            return false;
        }

        LPVOID lpvBase = VirtualAlloc(
            reinterpret_cast<char*>(m_data) + GetCommittedBytes(),
            m_pageSize,
            MEM_COMMIT,
            PAGE_READWRITE);

        if (lpvBase == nullptr)
        {
            // OUT of MEMORY
            return false;
        }

        ++m_committedPages;

        // TODO debug output

        return true;
    }

protected:

    [[nodiscard]] size_t GetCommittedBytes() const { return m_committedPages * m_pageSize; }
    [[nodiscard]] size_t GetReservedBytes() const { return m_reservedPages * m_pageSize; }

    constexpr static size_t CalculateAlignedMemorySize(size_t bytesToAllocate, size_t alignment)
    {
        // Calculate the remainder when bytes_to_allocate is divided by alignment
        const size_t remainder = bytesToAllocate % alignment;

        // If remainder is zero, then bytesToAllocate is already aligned
        // Otherwise, compute the amount of bytes needed to reach the next aligned value
        return (remainder == 0) ? bytesToAllocate : bytesToAllocate + (alignment - remainder);
    }
    static_assert(CalculateAlignedMemorySize(10, 4) == 12);
    static_assert(CalculateAlignedMemorySize(4000, 4096) == 4096);
    static_assert(CalculateAlignedMemorySize(4096, 4096) == 4096);
    static_assert(CalculateAlignedMemorySize(4097, 4096) == 8192);

    constexpr static size_t CalculatePageCount(size_t bytes, size_t alignment, size_t pageSize)
    {
        const size_t alignedBytes = CalculateAlignedMemorySize(bytes, alignment);
        assert(alignedBytes % alignment == 0);
        assert(alignedBytes % pageSize == 0); // TODO reformulate
        return alignedBytes / pageSize;
    }

    constexpr static size_t CalculateObjectAmountForNBytes(size_t bytes)
    {
        // TODO set proper constraint on alignment for total size and element size
        //assert(bytes % ElementSize == 0); // invalid
        return bytes / ElementSize;
    }
    /////////////////////////////////////////////////////////////////////////////////////////

private:
    ElementType* m_data;
    //size_t m_capacity; // capacity is calculated based on used pages and ElementSize value dynamically
    size_t m_size;

    size_t m_committedPages;
    size_t m_reservedPages;
    size_t m_pageSize;
};


int main()
{
    // Recognize RAM and reserve 2x
    unsigned long long TotalMemoryInKilobytes;
    GetPhysicallyInstalledSystemMemory(&TotalMemoryInKilobytes);

    unsigned long long TotalMemoryInBytes = TotalMemoryInKilobytes * 1024;
    unsigned long long RecommendedReserveInBytes = TotalMemoryInBytes /** 2*/;

    GrowingVectorVM<double> vectorInf;
    if (!vectorInf.ReserveBytes(RecommendedReserveInBytes))
    {
        std::cerr << "Can't reserve " << RecommendedReserveInBytes << " bytes. Consider smaller allocation\n";
    }

    const size_t maxAmount = vectorInf.GetCapacity();
    std::cout << "Capacity: " << maxAmount << "\n";
    std::cout << "Reserve: " << vectorInf.GetReserve() << "\n";

    // TODO impl use cases
    for (size_t i = 0; i < vectorInf.GetReserve(); i++)
    {
        vectorInf.PushBack(i * 1.23f);
    }

    try
    {
        vectorInf.PushBack(1.23f);
    }
    catch (const std::bad_array_new_length&)
    {
        std::cout << "Expected exception on reserve overflow\n";
    }
    return 0;
}
