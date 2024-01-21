#pragma once

#include <stdint.h>
#include <windows.h>
#include <memoryapi.h>

#include <sysinfoapi.h>                 // for GetPhysicallyInstalledSystemMemory

#include <iostream>

struct RAMSizePolicyTag {};
struct RAMDoubleSizePolicyTag {};

template <size_t N>
struct CustomSizePolicyTag
{
    constexpr static size_t size = N;
};


// Helper trait to check if a type has a 'size' member
template <typename T>
struct is_custom_sizing_policy : std::false_type {};

template <size_t N>
struct is_custom_sizing_policy<CustomSizePolicyTag<N>> : std::true_type {};


// TODO read TLB, TLB miss, physical memory access optimization, 512 times less of page faults and TLB misses
// TODO natvis for GrowingVector

// TODO test on smth bigger than int (probably with alignas())
// TODO to support large pages CommitPagesWithReserve option should work
template<typename T, typename ReservePolicy = RAMSizePolicyTag, bool LargePagesEnabled = false, bool CommitPagesWithReserve = false || LargePagesEnabled>
class GrowingVectorVM
{
public:
    using ElementType = T;
    constexpr static size_t ElementSize = sizeof(T);

    GrowingVectorVM();
    ~GrowingVectorVM() noexcept;

    [[nodiscard]] size_t GetSize() const { return m_size; }
    [[nodiscard]] size_t GetCapacity() const { return CalculateObjectAmountForNBytes(GetCommittedBytes()); };
    [[nodiscard]] size_t GetReserve() const { return CalculateObjectAmountForNBytes(GetReservedBytes()); };

    bool Reserve(size_t elementAmount)
    {
        return CommitOverallMemory(elementAmount * ElementSize);
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

    ElementType& At(size_t index, const ElementType& defValue)
    {
        if (index >= GetSize())
        {
            return defValue;
        }

        return this->operator[](index);
    }

    const ElementType& At(size_t index, const ElementType& defValue) const
    {
        return const_cast<const GrowingVectorVM*>(this)->At(index, defValue);
    }

    void PushBack(const ElementType& value)
    {
        ReallocateIfNeed();

        //if constexpr (std::is_trivially_copyable_v<ElementType>) // << no boost noticed
        //{
        //    memcpy(&m_data[GetSize()], &value, ElementSize);
        //}
        //else
        {
            //new (&m_data[GetSize()]) ElementType(value);
            EmplaceAtPlace(&m_data[GetSize()], value);
        }
        ++m_size;
    }

    void PushBack(const ElementType&& value)
    {
        ReallocateIfNeed();

        EmplaceAtPlace(&m_data[GetSize()], std::move(value));
        ++m_size;
    }

    // TODOs:
    // Emplace
    // Erase (by value, index, iterator)

    // compatibility with stl, generate, transform algorithms for ex.

private:
    bool ReserveBytes(size_t requestedBytes);

    // TODO test it
    bool CommitOverallMemory(size_t bytes);

    bool CommitAdditionalPage()
    {
        return CommitOverallMemory(GetCommittedBytes() + m_pageSize);
    }

    void ReallocateIfNeed()
    {
        // TODO validate size in case of unaligned structure
        if (GetSize() >= GetCapacity())
        {
            if (!CommitAdditionalPage())
            {
                // The container couldn't allocate one more virtual page
                throw std::bad_array_new_length();
            }
            assert(GetSize() < GetCapacity());
        }
    }

    template<typename... Args>
    void EmplaceAtPlace(ElementType* destination, Args&&... args)
    {
        new (destination) ElementType(std::forward<Args...>(args)...);
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



////////////////// IMPLEMENTATION //////////////////////////////
template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::GrowingVectorVM()
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

    unsigned long long TotalMemoryInBytes = 0;
    if constexpr (std::is_same_v<ReservePolicy, RAMSizePolicyTag>)
    {
        GetPhysicallyInstalledSystemMemory(&TotalMemoryInBytes);
        TotalMemoryInBytes *= 1024;
    }
    else if constexpr (std::is_same_v<ReservePolicy, RAMDoubleSizePolicyTag>)
    {
        GetPhysicallyInstalledSystemMemory(&TotalMemoryInBytes);
        TotalMemoryInBytes *= (1024 * 2);
    }
    else if constexpr (is_custom_sizing_policy<ReservePolicy>::value)
    {
        TotalMemoryInBytes = ReservePolicy::size;
    }
    else
    {
        // TODO find a way to make it static_assert
        //assert(false && "Unallowed Reserve Policy type is used! Use RAMSizePolicyTag, RAMDoubleSizePolicyTag or CustomSizePolicyTag");
        throw std::logic_error("Unallowed Reserve Policy type is used! Use RAMSizePolicyTag, RAMDoubleSizePolicyTag or CustomSizePolicyTag");
    }

    assert(TotalMemoryInBytes % m_pageSize == 0);
    if (ReserveBytes(TotalMemoryInBytes) == false)
    {
        throw std::bad_alloc{};
    }
}

template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::~GrowingVectorVM() noexcept
{
    const bool bSuccess = VirtualFree(
        m_data,                     // Base address of block
        0,                          // Bytes of committed pages, 0 for MEM_RELEASE, non-zero for DECOMMIT
        MEM_RELEASE);               // Decommit the pages
    assert(bSuccess);
}

template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline bool GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::ReserveBytes(size_t requestedBytes)
{
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

template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline bool GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::CommitOverallMemory(size_t bytes)
{
    if constexpr (CommitPagesWithReserve)
    {
        // TODO handle this properly
    }
    else
    {
        if (bytes <= GetCommittedBytes())
        {
            return true;
        }

        if (GetCommittedBytes() + m_pageSize > GetReservedBytes())
        {
            // TODO think about attempt to extend it
            return false;
        }

        const size_t wantage = bytes - GetCommittedBytes();
        const size_t requiredPages = CalculatePageCount(wantage, m_pageSize, m_pageSize);
        const size_t totalMemoryToCommit = requiredPages * m_pageSize;

        LPVOID lpvBase = VirtualAlloc(
            reinterpret_cast<char*>(m_data) + GetCommittedBytes(),
            totalMemoryToCommit,
            MEM_COMMIT,
            PAGE_READWRITE);

        if (lpvBase == nullptr)
        {
            // OUT of MEMORY
            return false;
        }

        m_committedPages += requiredPages;

        // TODO debug output

        return true;
    }
}