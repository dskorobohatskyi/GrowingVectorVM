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

#ifndef LARGE_PAGE_ENABLE
#define LARGE_PAGE_ENABLE 0
#endif // !LARGE_PAGE_ENABLE 

#ifndef MEMORY_COMMIT_WITH_RESERVE
#define MEMORY_COMMIT_WITH_RESERVE 1
#endif // !LARGE_PAGE_ENABLE


// TODO test on smth bigger than int (probably with alignas())
// check when to use reserve without commit?
template<class T, /*size_t PageSize,*/ size_t PageAllocationStep = 1, bool CommitPagesWithReserve = false>
class GrowingVectorVM
{
public:
    using ElementType = T;
    constexpr static size_t ElementSize = sizeof(T);

    GrowingVectorVM()
        : m_dataBlocks{}
        , m_size(0)
        , m_committedPages(0)
        , m_reservedPages(0)
    {
#if !LARGE_PAGE_ENABLE
        SYSTEM_INFO sSysInfo;                   // Useful information about the system
        GetSystemInfo(&sSysInfo);               // Initialize the structure.

        m_pageSize = sSysInfo.dwPageSize;       // Page size on this computer
#else
        m_pageSize = GetLargePageMinimum();
#endif
        assert(m_pageSize % ElementSize == 0); // TODO is it valid check?

        m_dataBlocks.reserve(10);
    }
    ~GrowingVectorVM()
    {
        for (const AllocatedVirtualMemoryBlock& block : m_dataBlocks)
        {
            const bool bSuccess = VirtualFree(
                block.blockStart,           // Base address of block
                0,                          // Bytes of committed pages, 0 for MEM_RELEASE, non-zero for DECOMMIT
                MEM_RELEASE);               // Decommit the pages
            assert(bSuccess);
        }
    }

    [[nodiscard]] size_t GetSize() const { return m_size; }
    [[nodiscard]] size_t GetCapacity() const { return CalculateObjectAmountForNBytes(GetReservedBytes()); };

    bool ReserveBytes(size_t requestedBytes)
    {
        // TODO check if current capacity alrady covers requestedBytes

        const size_t requiredPages = CalculatePageCount(requestedBytes, PageAllocationStep * m_pageSize, m_pageSize);
        const size_t totalSize = requiredPages * m_pageSize;

        DWORD allocationFlags = MEM_RESERVE;
        DWORD protectionFlags = PAGE_NOACCESS;
        if (CommitPagesWithReserve)
        {
            allocationFlags |= MEM_COMMIT;
            protectionFlags = PAGE_READWRITE;
        }
#if LARGE_PAGE_ENABLE
        allocationFlags |= MEM_LARGE_PAGES;
#endif // LARGE_PAGE_ENABLE
        // lpvBase - Base address of the test memory
        LPVOID lpvBase = VirtualAlloc(
            NULL,                   // System selects address
            totalSize,              // Size of allocation
            allocationFlags,        // Allocate reserved pages
            protectionFlags);

        if (lpvBase == NULL)
        {
            return false;
        }
        // TODO keep lpvBase in proper way

        // Guiding by Exception Safety Guarantee, let's modify state of the object only if everything goes successfully
        // LPVOID lpNxtPage = lpvBase;
        // DO I still need block system if I reserve huge chunk of data?
        //const AllocatedVirtualMemoryBlock newBlock{ .blockStart = reinterpret_cast<ElementType*>(lpvBase), .blockBytes = totalSize };
        //m_dataBlocks.push_back(std::move(newBlock));

        m_reservedPages = requiredPages;
        m_committedPages = CommitPagesWithReserve ? m_reservedPages : 0;

        std::cout << "GrowingVectorVM preallocated " << totalSize << " bytes of vm for " << GetCapacity() << " objects\n";

        return true;
    }

    bool Reserve(size_t elementAmount)
    {
        // TODO validate that ElementSize is aligned
        return ReserveBytes(elementAmount * ElementSize);
    }

    // TODO adjust to new requirements
    const ElementType& operator[](size_t index) const
    {
        //if (index >= GetSize())
        //{
        //    //size_t newSize = index + 1;
        //    //ForceReallocateUnitialized(newSize);
        //    throw std::out_of_range{ "operator[] failed" }; // TODO extend with useful info
        //}

        const bool IsBasedOnSizeNotCapacity = true;
        const std::optional<InternalDataIndex> internalIndexOpt = CalculateInternalIndex(index, IsBasedOnSizeNotCapacity);

        if (!internalIndexOpt.has_value())
        {
            // TODO consider reallocation later
            throw std::out_of_range{ "operator[] failed" }; // TODO extend with useful info
        }

        return m_dataBlocks[internalIndexOpt->blockIndex][internalIndexOpt->dataIndex];
    }

    const ElementType& At(size_t index, const ElementType& defValue) const
    {
        if (index >= GetSize())
        {
            return defValue;
        }

        return this->operator[](index);
    }
    // TODO provide non-const versions of [] and At
    // TODOs:
    // PushBack
    // Emplace
    // empty
    // Erase (by value, index, iterator)

protected:

    [[nodiscard]] size_t GetCommittedBytes() const { return m_committedPages * m_pageSize; }
    [[nodiscard]] size_t GetReservedBytes() const { return m_reservedPages * m_pageSize; }

    // TODO rename and refactor
    bool ForceReallocateUnitialized(size_t newAmount)
    {
        assert(newAmount > m_size);
        const size_t minimalRequiredBytes = ElementSize * (newAmount) - GetCommittedBytes();
        const size_t pageCount = CalculatePageCount(minimalRequiredBytes, PageAllocationStep * m_pageSize, m_pageSize);
        
        // TODO apply CommitPagesWithReserve
        DWORD allocationFlags = MEM_COMMIT;
#if LARGE_PAGE_ENABLE
        allocationFlags |= MEM_LARGE_PAGES;
#endif // LARGE_PAGE_ENABLE

        assert(!m_dataBlocks.empty());
       
        const AllocatedVirtualMemoryBlock& lastBlock = m_dataBlocks.back(); // potentially fails now
        LPVOID lpvBase = VirtualAlloc(
            lastBlock.blockStart + lastBlock.blockBytes,    // System selects address
            pageCount * m_pageSize,                         // Size of allocation
            allocationFlags,                                // Allocate reserved pages
            PAGE_READWRITE);

        if (lpvBase == NULL)
        {
            return false;
        }

        const AllocatedVirtualMemoryBlock newBlock{ .blockStart = reinterpret_cast<ElementType*>(lpvBase), .blockBytes = pageCount * m_pageSize };
        m_dataBlocks.push_back(std::move(newBlock));

        m_committedPages += pageCount;

        return true;
    }

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
        assert(bytes % ElementSize == 0);
        return bytes / ElementSize;
    }
    /////////////////////////////////////////////////////////////////////////////////////////

    struct InternalDataIndex
    {
        size_t blockIndex = 0;
        size_t dataIndex = 0;
    };

    // TODO validate
    std::optional<InternalDataIndex> CalculateInternalIndex(size_t externalIndex, bool IsBasedOnSizeNotCapacity = false) const
    {
        if (IsBasedOnSizeNotCapacity && externalIndex >= GetSize())
        {
            return std::nullopt;
        }

        if (externalIndex >= GetCapacity())
        {
            return std::nullopt;
        }

        // 0 1 2 3 4 | 5 6 | 7 8 9   <- external index
        // 0 1 2 3 4 | 0 1 | 0 1 2   <- block index
        // x x x x x | x x | x X x
        size_t currentCapacity = 0;
        for (size_t blockIndex = 0; const AllocatedVirtualMemoryBlock& block : m_dataBlocks)
        {
            const size_t blockCapacity = CalculateObjectAmountForNBytes(block.blockBytes);
            if (currentCapacity + blockCapacity > externalIndex)
            {
                return InternalDataIndex{ blockIndex, externalIndex - currentCapacity};
            }
            currentCapacity += blockCapacity;
            ++blockIndex;
        }

        return std::nullopt;
    }

private:
    struct AllocatedVirtualMemoryBlock
    {
        ElementType* blockStart = nullptr;
        size_t blockBytes = 0;

        ElementType& operator[](size_t index)
        {
            return blockStart[index];
        }
    };


    std::vector<AllocatedVirtualMemoryBlock> m_dataBlocks; // concern: default allocation system
    //size_t m_capacity; // capacity is calculated based on used pages and ElementSize value dynamically
    size_t m_size;

    size_t m_committedPages;
    size_t m_reservedPages;
    size_t m_pageSize;
};


// why I need that structure, use cases
// consider actual vector (push back) behavior (no reallocation, no invalidation on push_back())
// TODO iterations and algorithm


int main()
{

    // Recognize RAM and reserve 2x
    unsigned long long TotalMemoryInKilobytes;
    GetPhysicallyInstalledSystemMemory(&TotalMemoryInKilobytes);

    unsigned long long TotalMemoryInBytes = TotalMemoryInKilobytes * 1024;
    unsigned long long RecommendedReserveInBytes = TotalMemoryInBytes * 2;

    {
        GrowingVectorVM<double, 1, true> vectorInf;
        if (!vectorInf.ReserveBytes(RecommendedReserveInBytes))
        {
            std::cerr << "Can't reserve " << RecommendedReserveInBytes << " bytes. Consider smaller allocation\n";
        }
    }

    GrowingVectorVM<int> vectorInf;
    if (!vectorInf.ReserveBytes(GB(4)))
    {
        std::cerr << "Can't reserve " << GB(4) << " bytes for vectorInf. Consider smaller allocation. Quitting...\n";
        return 1;
    }

    const size_t maxAmount = vectorInf.GetCapacity();
    // TODO impl use cases

    return 0;
}
