#include "GrowingVectorVM.h"

#ifdef WIN32
#include <windows.h>
#include <memoryapi.h>

#include <sysinfoapi.h>                 // for GetPhysicallyInstalledSystemMemory
#else
#error Unsupported
#endif

namespace PlatformHelper
{
    size_t CalculateVirtualPageSize(const bool isLargePagesEnabled)
    {
        size_t pageSize = 0;
        if (!isLargePagesEnabled)
        {
#if WIN32
            SYSTEM_INFO sSysInfo;                   // Useful information about the system
            GetSystemInfo(&sSysInfo);               // Initialize the structure.

            pageSize = sSysInfo.dwPageSize;         // Page size on this computer
#else
#error Not implemented
#endif
        }
        else
        {
#if WIN32
            pageSize = GetLargePageMinimum();
#else
#error Not implemented
#endif
        }

        return pageSize;
    }

    void NullifyMemory(void* destination, const size_t size)
    {
#if WIN32
        memset(destination, 0, size);
#else
#error Not implemented
#endif
    }

    size_t CalculateInstalledRAM()
    {
        unsigned long long TotalMemoryInBytes = 0;
#if WIN32
        GetPhysicallyInstalledSystemMemory(&TotalMemoryInBytes);
        TotalMemoryInBytes *= 1024;
#else
#error Not implemented
#endif
        return TotalMemoryInBytes;
    }

    void* ReserveVirtualMemory(
        const size_t alignedAllocationSize,
        const size_t pageSize,
        void* base/* = nullptr*/,
        const bool shouldCommitWithReserve/* = false*/,
        const bool isLargePagesEnabled/* = false*/
    )
    {
        void* memory = nullptr;
#if WIN32
        DWORD allocationFlags = MEM_RESERVE;
        DWORD protectionFlags = PAGE_NOACCESS;
        if (shouldCommitWithReserve)
        {
            allocationFlags |= MEM_COMMIT;
            protectionFlags = PAGE_READWRITE;
        }

        // Large-page memory must be reserved and committed as a single operation.
        // In other words, large pages cannot be used to commit a previously reserved range of memory.
        if (isLargePagesEnabled)
        {
            allocationFlags |= MEM_LARGE_PAGES;
        }
        // memory - Base address of the allocated memory
        memory = VirtualAlloc(
            base,                           // System selects address
            alignedAllocationSize,          // Size of allocation
            allocationFlags,                // Allocate reserved pages
            protectionFlags);
#else
#error Not implemented
#endif
        return memory;
    }

    void* CommitVirtualMemory(
        void*& destination,
        const size_t memorySizeToCommit
        // TODO probably flags should be customizable also
    )
    {
        void* committedMemory = nullptr;
#if WIN32
        committedMemory = VirtualAlloc(
            destination,
            memorySizeToCommit,
            MEM_COMMIT,
            PAGE_READWRITE);
#else
#error Not implemented
#endif
        return committedMemory;
    }

    bool DecommitVirtualMemory(
        void*& destination,
        const size_t memorySizeToDecommit
    )
    {
        bool result = false;
#if WIN32
        result = VirtualFree(
            destination,
            memorySizeToDecommit,
            MEM_DECOMMIT);
#else
#error Not implemented
#endif
        return result;
    }

    bool ReleaseVirtualMemory(
        void* destination
    )
    {
        bool result = false;
#if WIN32
        result = VirtualFree(
            destination,                // Base address of block
            0,                          // Bytes of committed pages, 0 for MEM_RELEASE, non-zero for DECOMMIT
            MEM_RELEASE);               // Decommit the pages
#else
#error Not implemented
#endif
        return result;
    }
};