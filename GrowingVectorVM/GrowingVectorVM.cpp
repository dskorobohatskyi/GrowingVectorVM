// GrowingVectorVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <assert.h>
#include <optional>

#include <stdint.h>
#include <windows.h>
#include <memoryapi.h>

#define PAGELIMIT 1'000'000 // practical
//constexpr size_t PAGELIMIT = 4'503'599'627'370'496; // theory
//                18'446'744'073'709'551'616 (2^64) / 4096


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

template<class T, size_t AllocationBytesStep = GB(4), bool ReserveOnConstruct = false>
class GrowingVectorVM
{
public:
    using ElementType = T;
    constexpr static size_t ElementSize = sizeof(T);

    GrowingVectorVM()
        : m_dataBlocks{}
        , m_size(0)
#if !MEMORY_COMMIT_WITH_RESERVE
        , m_committedPages(0)
#endif
        , m_reservedPages(0)
    {
#if !LARGE_PAGE_ENABLE
        SYSTEM_INFO sSysInfo;                   // Useful information about the system
        GetSystemInfo(&sSysInfo);               // Initialize the structure.

        m_pageSize = sSysInfo.dwPageSize;       // Page size on this computer
#else
        m_pageSize = GetLargePageMinimum();
#endif

        m_dataBlocks.reserve(10);
        if (ReserveOnConstruct)
        {
            ReserveBytes(AllocationBytesStep);
        }
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

    size_t GetSize() const { return m_size; }
    size_t GetCapacity() const { return CalculateObjectAmountForNBytes(GetAllocatedBytes()); };
    size_t GetAllocatedBytes() const { return m_reservedPages * m_pageSize; }

    bool ReserveBytes(size_t requestedBytes)
    {
        // TODO Simplify this later
        assert(requestedBytes % m_pageSize == 0);

        const size_t requiredPages = CalculatePageCount(requestedBytes, AllocationBytesStep, m_pageSize);
        const size_t totalSize = requiredPages * m_pageSize;
        assert(requestedBytes == totalSize);

        DWORD allocationFlags = MEM_RESERVE | MEM_COMMIT;
#if LARGE_PAGE_ENABLE
        allocationFlags |= MEM_LARGE_PAGES;
#endif // LARGE_PAGE_ENABLE
        // lpvBase - Base address of the test memory
        LPVOID lpvBase = VirtualAlloc(
            NULL,                   // System selects address
            totalSize,              // Size of allocation
            allocationFlags,        // Allocate reserved pages
            /*PAGE_NOACCESS */PAGE_READWRITE);

        if (lpvBase == NULL)
            return false;

        // Guiding by Exception Safety Guarantee, let's modify state of the object only if everything goes successfully
        // LPVOID lpNxtPage = lpvBase;
        const AllocatedVirtualMemoryBlock newBlock{ .blockStart = reinterpret_cast<ElementType*>(lpvBase), .blockBytes = totalSize };
        m_dataBlocks.push_back(std::move(newBlock));

        m_reservedPages = requiredPages;
#if !MEMORY_COMMIT_WITH_RESERVE
        m_committedPages = m_reservedPages;
#endif // !MEMORY_COMMIT_WITH_RESERVE
        std::cout << "GrowingVectorVM preallocated " << totalSize << " bytes of vm for " << GetCapacity() << " objects\n";

        return true;
    }

    bool Reserve(size_t elementAmount)
    {
        // TODO validate that ElementSize is aligned
        return ReserveBytes(elementAmount * ElementSize);
    }

    bool ForceReallocateUnitialized(size_t newAmount)
    {
        assert(newAmount > m_size);
        const size_t minimalRequiredBytes = ElementSize * (newAmount) - GetAllocatedBytes();
        const size_t pageCount = CalculatePageCount(minimalRequiredBytes, AllocationBytesStep, m_pageSize);

        DWORD allocationFlags = MEM_RESERVE | MEM_COMMIT;
#if LARGE_PAGE_ENABLE
        allocationFlags |= MEM_LARGE_PAGES;
#endif // LARGE_PAGE_ENABLE
        
        assert(!m_dataBlocks.empty());
        const AllocatedVirtualMemoryBlock& lastBlock = m_dataBlocks.back();
        LPVOID lpvBase = VirtualAlloc(
            lastBlock.blockStart + lastBlock.blockBytes,    // System selects address
            pageCount * m_pageSize,                         // Size of allocation
            allocationFlags,                                // Allocate reserved pages
            /*PAGE_NOACCESS */PAGE_READWRITE);

        if (lpvBase == NULL)
        {
            return false;
        }

        const AllocatedVirtualMemoryBlock newBlock{ .blockStart = reinterpret_cast<ElementType*>(lpvBase), .blockBytes = pageCount * m_pageSize };
        m_dataBlocks.push_back(std::move(newBlock));
        // TODO lpvBase should be released separately
        // TODO introduce memory block and immitate continuous one
        m_reservedPages += pageCount;

        return true;
    }

    ElementType& operator[](size_t index)
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

    ElementType& GetOrAdd(size_t index, const ElementType& defValue)
    {
        if (index >= GetSize())
        {
            if (index < GetCapacity())
            {
                // TODO wrap by method
                for (size_t i = m_size; i <= index; i++)
                {
                    // TODO consider switching between allocated blocks
                    // HACK for now
                    new (&m_dataBlocks[0][i]) ElementType(defValue);
                    // TODO check std::uninitialized_value_construct_n
                }
                m_size = index + 1;
            }
            else
            {
                // the memory limit exceed
                //ForceReallocateUnitialized(index + 1);

                // TODO wrap by method
                //auto oldSize = m_size;
                //for (size_t i = 0; i <= index - oldSize; i++)
                //{
                //    new ((ElementType*)&m_dataBlocks[1][i]) ElementType(defValue);
                //    // TODO check std::uninitialized_value_construct_n
                //}
                //m_size = index + 1;
                //return ((ElementType*)hackyPtr)[index - oldSize];

                throw std::out_of_range{ "operator[] failed" }; // temporary
            }
        }

        return this->operator[](index);
    }

private:
    constexpr static size_t CalculateAlignedMemorySize(size_t bytesToAllocate, size_t alignment)
    {
        // Calculate the remainder when bytes_to_allocate is divided by alignment
        const size_t remainder = bytesToAllocate % alignment;

        // If remainder is zero, then bytesToAllocate is already aligned
        if (remainder == 0)
        {
            return bytesToAllocate;
        }

        // Otherwise, compute the amount of bytes needed to reach the next aligned value
        return bytesToAllocate + (alignment - remainder);
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



    std::vector<AllocatedVirtualMemoryBlock> m_dataBlocks;
    //size_t m_capacity; // capacity is calculated based on used pages and ElementSize value dynamically
    size_t m_size;

#if !MEMORY_COMMIT_WITH_RESERVE
    size_t m_committedPages; // TODO if reserve and commit are separate
#endif
    size_t m_reservedPages;
    size_t m_pageSize;
};


int main()
{
    {
        GrowingVectorVM<double> vectorInf;
        vectorInf.ReserveBytes(GB(4));
    }

    GrowingVectorVM<int> vectorInf;
    vectorInf.ReserveBytes(GB(4));

    try
    {
        vectorInf[0] = 3;
    }
    catch (const std::out_of_range& e)
    {
        std::cout << "Processed: " << e.what() << std::endl;
    }

    assert(vectorInf.GetOrAdd(0, 3) == 3);
    vectorInf.GetOrAdd(10, 2);
    assert(vectorInf[10] == 2);
    assert(vectorInf[8] == 2); // also inited by 2
    vectorInf.GetOrAdd(10000, 5);
    assert(vectorInf[9999] == 5);

    size_t maxAmount = vectorInf.GetCapacity();
    vectorInf.GetOrAdd(maxAmount - 1, 100); // 4ms on my machine
    assert(vectorInf[maxAmount - 1] == 100);

    int value = vectorInf.GetOrAdd((maxAmount - 1) * 2, 99); // why it works without assignment during direct access to memory (no custom operator[]) (optimization)??

    // TODO
    //int value = vectorInf.GetOrAdd(5'000'000'000, 99); // why it works without assignment (optimization)??

    //assert(vectorInf[5'000'000'000] == 99); // Crash already, first explicit access to the memory
    //assert(vectorInf[4'999'999'999] == 99);

    return 0;
}
