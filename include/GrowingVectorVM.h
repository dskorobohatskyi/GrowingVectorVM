#pragma once

// TODO Think about user experience with including your header-only library
// Provide switching mechanism with static or dynamic library later.
// Analyze which implemented functionality can be wrapped by free functions and be moved to cpp to reduce the compilation time.
#ifdef WIN32
#include <windows.h>
#include <memoryapi.h>

#include <sysinfoapi.h>                 // for GetPhysicallyInstalledSystemMemory
#else
#error Unsupported
#endif

#include <stdint.h>
#include <assert.h>
#include <iostream>                     // TODO remove this dependency, consider printf as much lightweight analogue
#include <compare>                      // for operator <=>
#include <xutility>                     // for std::reverse_iterator
#include <stdexcept>                    // for std::logic_error
#include <memory>                       // for uninitialized_default_construct_n and uninitialized_fill_n (potential candidate to implement on my own)

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

// TODO make it a namespace or separate decl from implementation in the future
struct ObjectLifecycleHelper
{
    template<typename T, typename... Args>
    static void ConstructObject(T* destination, Args&&... args)
    {
        new (destination) T(std::forward<Args...>(args)...);
    }

    template <typename T>
    static void DestructObject(const T* object)
    {
        if constexpr (!std::is_trivially_destructible_v<T>) // otherwise we can do nothing, memory deallocation will handle that
        {
            if (object)
            {
                object->~T();
            }
        }
    }
};

// TODO analyze what can be constexpr and nodiscard again in the code?
// TODO validate that iterators are compatible with each other (_Compat method in STL)
// Custom iterator classes
template <typename Container>
class ConstIterator
{
public:
    // Typedefs for better compatibility
    using value_type = typename Container::value_type;
    using pointer = typename Container::const_pointer;
    using reference = typename Container::const_reference;
    using difference_type = typename Container::difference_type;
    using iterator_category = std::random_access_iterator_tag; // Specify the iterator category


    // Constructor
    ConstIterator(typename Container::pointer p) noexcept : ptr(p) {}
    ConstIterator(nullptr_t) = delete; // mark this as delete to not create iterator over direct null ptr

    // Dereference operator
    reference operator*() const
    {
        return *ptr;
    }

    // Arrow operator
    pointer operator->() const noexcept
    {
        return ptr;
    }

    // Prefix increment operator
    ConstIterator& operator++() noexcept
    {
        ++ptr;
        return *this;
    }

    // Postfix increment operator
    ConstIterator operator++(int) noexcept
    {
        ConstIterator temp = *this;
        ++ptr;
        return temp;
    }

    // Prefix decrement operator
    ConstIterator& operator--() noexcept
    {
        --ptr;
        return *this;
    }

    // Postfix decrement operator
    ConstIterator operator--(int) noexcept
    {
        ConstIterator temp = *this;
        --ptr;
        return temp;
    }

    // Difference for two iterators
    difference_type operator-(const ConstIterator other) const noexcept
    {
        return ptr - other.ptr;
    }

    // Addition operator for forward movement
    ConstIterator operator+(difference_type n) const noexcept
    {
        return ConstIterator(ptr + n);
    }

    // Subtraction operator for backward movement
    ConstIterator operator-(difference_type n) const noexcept
    {
        return ConstIterator(ptr - n);
    }

    // Compound assignment addition operator
    ConstIterator& operator+=(difference_type n) noexcept
    {
        // TODO verify offset
        ptr += n;
        return *this;
    }

    // Compound assignment subtraction operator
    ConstIterator& operator-=(difference_type n) noexcept
    {
        // TODO verify offset
        ptr -= n;
        return *this;
    }

    // Equality operator
    bool operator==(const ConstIterator& other) const noexcept
    {
        return ptr == other.ptr;
    }

    // TODO if cpp standard is c++20 then <=>, otherwise need to implement all the needed cmp operators
    // Inequality operator
    std::strong_ordering operator<=>(const ConstIterator& other) const
    {
        return ptr <=> other.ptr;
    }
    // else < c++20
    //bool operator!=(const ConstIterator& other) const noexcept
    //{
    //    return ptr != other.ptr;
    //}

    // Subscript operator for random access
    reference operator[](difference_type index) const noexcept
    {
        return *(ptr + index);
    }

    // No need to make it a private/protected since it can be accessed via operator ->
    typename Container::pointer ptr;  // Pointer to the current element
};

template <typename Container>
class Iterator : public ConstIterator<Container>
{
public:
    using Base = ConstIterator<Container>;

    // Typedefs for better compatibility
    using value_type = typename Container::value_type;
    using pointer = typename Container::pointer;
    using reference = typename Container::reference;
    using difference_type = typename Container::difference_type;
    using iterator_category = std::random_access_iterator_tag; // Specify the iterator category

    // Constructor
    using Base::Base;

    // Dereference operator
    reference operator*() const noexcept
    {
        return const_cast<reference>(Base::operator*());
    }

    // Arrow operator
    pointer operator->() const noexcept
    {
        return this->ptr;
    }

    // Prefix increment operator
    Iterator& operator++() noexcept
    {
        Base::operator++();
        return *this;
    }

    // Postfix increment operator
    Iterator operator++(int) noexcept
    {
        Iterator temp = *this;
        Base::operator++();
        return temp;
    }

    // Prefix decrement operator
    Iterator& operator--() noexcept
    {
        Base::operator--();
        return *this;
    }

    // Postfix decrement operator
    Iterator operator--(int) noexcept
    {
        Iterator temp = *this;
        Base::operator--();
        return temp;
    }

    // Difference for two iterators
    difference_type operator-(const Iterator other) const noexcept
    {
        return this->ptr - other.ptr;
    }

    // Addition operator for forward movement
    Iterator operator+(difference_type n) const noexcept
    {
        return Iterator(this->ptr + n);
    }

    // Subtraction operator for backward movement
    Iterator operator-(difference_type n) const noexcept
    {
        return Iterator(this->ptr - n);
    }

    // Compound assignment addition operator
    Iterator& operator+=(difference_type n) noexcept
    {
        Base::operator+=(n);
        return *this;
    }

    // Compound assignment subtraction operator
    Iterator& operator-=(difference_type n) noexcept
    {
        Base::operator-=(n);
        return *this;
    }

    // Equality operator
    bool operator==(const Iterator& other) const noexcept
    {
        return this->ptr == other.ptr;
    }

    // Inequality operator
    bool operator!=(const Iterator& other) const noexcept
    {
        return this->ptr != other.ptr;
    }

    // Subscript operator for random access
    reference operator[](difference_type index) const noexcept
    {
        return const_cast<reference>(Base::operator[](index));
    }
};



// TODO read TLB, TLB miss, physical memory access optimization, 512 times less of page faults and TLB misses
// TODO natvis for GrowingVector
// TODO need to analyze places where exceptions can throw
// Guarantee compatibility with stl, generate, transform algorithms for ex.


// TODO to support large pages CommitPagesWithReserve option should work
template<typename T, typename ReservePolicy = RAMSizePolicyTag, bool LargePagesEnabled = false, bool CommitPagesWithReserve = false || LargePagesEnabled>
class GrowingVectorVM
{
public:
    using value_type = T;
    constexpr static size_t ElementSize = sizeof(T);

    // For compatibility with STL
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    using SelfType = GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>;
    using iterator = Iterator<SelfType>;
    using const_iterator = ConstIterator<SelfType>;

    // that's not good to include additional libraries to own one, since that's increase compilation time and actual codesize during includes.
    // but I really don't have time now to implement my own version of reverse_iterator, so I reuse STD one.
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    GrowingVectorVM();
    ~GrowingVectorVM() noexcept;

    // From cppreference: After the move, other is guaranteed to be empty().
    GrowingVectorVM(const GrowingVectorVM&& other)
        : m_data(std::exchange(other.m_data, nullptr))
        , m_size(std::exchange(other.m_size, 0))
        , m_committedPages(std::exchange(other.m_committedPages, 0))
        , m_reservedPages(std::exchange(other.m_reservedPages, 0))
        , m_pageSize(std::exchange(other.m_pageSize, 0))
    {
    }

    GrowingVectorVM& operator=(const GrowingVectorVM&& other)
    {
        if (this != &other)
        {
            ReleaseMemory();

            m_data = std::exchange(other.m_data, nullptr);
            m_size = std::exchange(other.m_size, 0);
            m_committedPages = std::exchange(other.m_committedPages, 0);
            m_reservedPages = std::exchange(other.m_reservedPages, 0);
            m_pageSize = std::exchange(other.m_pageSize, 0);
        }

        return *this;
    }

    // Disable copy construction for now, since memory reserve should be done for another vector as well and it will throw without careful setup.
    GrowingVectorVM(const GrowingVectorVM& other) = delete;
    GrowingVectorVM& operator=(const GrowingVectorVM& other) = delete;
    /////////////////////////////////////////////////////////////////////
    //GrowingVectorVM(const size_type count); // TODO
    //GrowingVectorVM(const size_type count, const value_type& value); // TODO
    // template<typename InputIt>
    //GrowingVectorVM(InputIt first, InputIt last); // TODO
    //GrowingVectorVM(std::initializer_list<T> init); // TODO
    //GrowingVectorVM& operator=(std::initializer_list<T> ilist);

    // void swap( vector& other ) noexcept(/* see below */);
    // cmp methods

    [[nodiscard]] inline size_type GetSize() const noexcept { return m_size; }
    [[nodiscard]] inline size_type GetCapacity() const noexcept { return CalculateObjectAmountForNBytes(GetCommittedBytes()); };
    [[nodiscard]] inline size_type GetReserve() const noexcept { return CalculateObjectAmountForNBytes(GetReservedBytes()); };
    [[nodiscard]] size_type GetPageSize() const noexcept
    {
        if (m_pageSize == 0) [[unlikely]]
        {
            m_pageSize = CalculatePageSize();
        }

        return m_pageSize;
    }
    
    [[nodiscard]] inline pointer GetData() noexcept { return m_data; }
    [[nodiscard]] inline const_pointer GetData() const noexcept { return m_data; }

    [[nodiscard]] inline const value_type& Back() const { return this->operator[](GetSize() - 1); }
    [[nodiscard]] inline value_type& Back() { return this->operator[](GetSize() - 1); }

    [[nodiscard]] inline const value_type& Front() const { return this->operator[](0); }
    [[nodiscard]] inline value_type& Front() { return this->operator[](0); }

    // Note from cppreference: https://en.cppreference.com/w/cpp/container/vector/begin
    // If the vector is empty, the returned (begin) iterator will be equal to end().
    // 
    // Since GrowingVector is implemented on virtual memory and does internal memory reserve in ctor,
    // then m_data is valid ptr even if container is empty.
    // Indicator of emptiness are: Begin() == End() so this is supported in this functionality.
    // TODO It'd make sense to wrap the nullptr for iterators in case if container is empty.
    // It's easier to debug and much obvious how to resolve.
    [[nodiscard]] inline iterator Begin() noexcept { return iterator{ m_data }; }
    [[nodiscard]] inline const_iterator Begin() const noexcept { return const_iterator{ m_data }; }
    [[nodiscard]] inline iterator End() noexcept { return iterator{ m_data + GetSize() }; }
    [[nodiscard]] inline const_iterator End() const noexcept { return const_iterator{ m_data + GetSize() }; }

    [[nodiscard]] inline const_iterator CBegin() const noexcept { return const_iterator{ m_data }; }
    [[nodiscard]] inline const_iterator CEnd() const noexcept { return const_iterator{ m_data + GetSize() }; }

    // Reverse versions
    [[nodiscard]] inline reverse_iterator RBegin() noexcept { return reverse_iterator{ End() }; }
    [[nodiscard]] inline const_reverse_iterator RBegin() const noexcept { return const_reverse_iterator{ End() }; }
    [[nodiscard]] inline reverse_iterator REnd() noexcept { return reverse_iterator{ Begin() }; }
    [[nodiscard]] inline const_reverse_iterator REnd() const noexcept { return const_reverse_iterator{ Begin() }; }

    [[nodiscard]] inline const_reverse_iterator CRBegin() const noexcept { return const_reverse_iterator{ End() }; }
    [[nodiscard]] inline const_reverse_iterator CREnd() const noexcept { return const_reverse_iterator{ Begin() }; }


    bool Reserve(size_t elementAmount)
    {
        return CommitOverallMemory(elementAmount * ElementSize);
    }

    [[nodiscard]] inline bool Empty() const noexcept { return GetSize() == 0; }

    const value_type& operator[](size_type index) const
    {
        return const_cast<const GrowingVectorVM*>(this)->operator[](index);
    }

    value_type& operator[](size_type index)
    {
        if (index >= GetSize())
        {
            throw std::out_of_range{ "operator[] failed" }; // TODO extend with useful info
        }

        return m_data[index];
    }

    [[nodiscard]] value_type& At(size_type index, const value_type& defValue)
    {
        if (index >= GetSize())
        {
            return defValue;
        }

        return this->operator[](index);
    }

    [[nodiscard]] const value_type& At(size_type index, const value_type& defValue) const
    {
        return const_cast<const GrowingVectorVM*>(this)->At(index, defValue);
    }

    void PushBack(const value_type& value)
    {
        // TODO profile why this code is not effective as expected
        //if constexpr (std::is_trivially_copyable_v<value_type>) // << no boost noticed
        //{
        //    memcpy(&m_data[GetSize()], &value, ElementSize);
        //}
        //else
        {
            //new (&m_data[GetSize()]) value_type(value);
            EmplaceBackReallocate(value);
        }
    }

    void PushBack(const value_type&& value)
    {
        EmplaceBackReallocate(std::move(value));
    }

    void PopBack()
    {
        assert(!Empty());
        Erase(CEnd() - 1, CEnd());
    }

    template<typename... Args>
    void EmplaceBack(Args&&... args)
    {
        EmplaceBackReallocate(std::forward<Args...>(args)...);
    }


    iterator Insert(const_iterator position, const value_type& value)
    {
        return Emplace(position, value);
    }
    iterator Insert(const_iterator position, const value_type&& value)
    {
        return Emplace(position, std::move(value));
    }
    iterator InsertAtIndex(difference_type index, const value_type& value)
    {
        return EmplaceAtIndex(index, value);
    }
    iterator InsertAtIndex(difference_type index, const value_type&& value)
    {
        return EmplaceAtIndex(index, std::move(value));
    }

    iterator Insert(const_iterator position, const size_type count, const value_type& value)
    {
        if (GetCapacity() < GetSize() + count)
        {
            // [README] Naive solution, potentially can be rewritten in the future.
            // Important: count can be bigger than one page can provide, so Reserve is used here
            if (Reserve(GetSize() + count) == false)
            {
                // TODO check if this is ok?
                throw std::bad_alloc{};
            }
        }

        // Ensure the position is within the bounds of the vector
        assert((position >= CBegin()) && (position <= CEnd()));
        assert(CBegin().ptr != nullptr);

        const difference_type offset = position - CBegin();
        iterator nonConstIterator = Begin() + offset; // shouldn't be invalidated since all the elements on the right change

        ShiftElementsToTheRight(nonConstIterator, count);

        std::uninitialized_fill_n(nonConstIterator, count, value);

        m_size += count;

        return nonConstIterator;
    }

    template<typename... Args>
    iterator EmplaceAtIndex(difference_type index, Args&&... args)
    {
        // position is ignored if the container is empty
        if (Empty())
        {
            EmplaceBack(std::forward<Args...>(args)...);
            return Begin();
        }

        assert(index >= 0 && index < (difference_type)GetSize());

        ReallocateIfNeed();

        ShiftElementsToTheRight(Begin() + index, 1);

        EmplaceAtPlace(&m_data[index], std::forward<Args...>(args)...);

        // Return iterator pointing to the inserted element
        return Begin() + index;
    }

    template<typename... Args>
    iterator Emplace(const_iterator position, Args&&... args)
    {
        // position is ignored if the container is empty
        if (Empty())
        {
            EmplaceBack(std::forward<Args...>(args)...);
            return Begin();
        }

        // Ensure the position is within the bounds of the vector
        if ((position < CBegin()) || (position > CEnd()))
        {
            assert(false);
            return End();  // Return end iterator as an indication of failure
        }

        // Calculate the index corresponding to the iterator
        const difference_type index = std::distance(CBegin(), position);

        return EmplaceAtIndex(index, std::forward<Args...>(args)...);
    }

    // DOn't forget about dtor call
    iterator Erase(const_iterator pos)
    {
        // From cppreference: If pos refers to the last element, then the end() iterator is returned.
        // TODO validate range
        return Erase(pos, pos + 1);
    }

    iterator Erase(const_iterator first, const_iterator last)
    {
        // From cppreference: If last == end() prior to removal, then the updated end() iterator is returned.
        // If[first, last) is an empty range, then last is returned.
        assert(false && "Not implemented");
        assert(first <= last);
        // TODO validate begin <= first <= last <= end
        const difference_type removingRangeSize = last - first;
        if (removingRangeSize == 0)
        {
            return last;
        }

        const bool hasLastElementAffected = last == CEnd();

        // Destructing range
        for (auto it(first); it != last; ++it)
        {
            ObjectLifecycleHelper::DestructObject(it->ptr);
        }

        // test cases
        // empty range
        // pop_back (end - 1, end)
        // front_back(begin, begin + 1)
        // clear ~ erase (begin, end)  ~ removingRangeSize == size
        // removingRangeSize > shifting objects count
        // removingRangeSize < shifting objects count


        m_size -= removingRangeSize;
        // TODO return
    }

    void Clear() noexcept
    {
        // README Didn't use Resize(0) to not trigger assert(is_default_constructible) in else-constexpr section there (thank you c++)

        if (GetSize() == 0)
        {
            return;
        }

        // TODO probably it makes sense to encapsulate memset call in separate function
        memset(m_data, 0, GetSize() * ElementSize);
        m_size = 0;
    }

    template <typename U>
    void Resize(const size_type newSize, const U& def)
    {
        if (newSize == GetSize())
        {
            return;
        }
        else if (newSize < GetSize())
        {
            void* const erasingDestination = (m_data + newSize);
            memset(erasingDestination, 0, (GetSize() - newSize) * ElementSize);
            m_size = newSize;
        }
        else // newSize > GetSize()
        {
            if (newSize > GetCapacity())
            {
                Reserve(newSize);
            }

            if constexpr (std::is_same_v<T, U>)
            {
                std::uninitialized_fill_n(End(), newSize - GetSize(), def);
            }
            else
            {
                static_assert(std::is_same_v<U, DefaultContructTag>);
                static_assert(std::is_default_constructible_v<value_type> && "Use Resize with Default value for this type if Resize(const size_t newSize) fails");
                std::uninitialized_default_construct_n(End(), newSize - GetSize());
            }

            m_size = newSize;
        }
    }

    void Resize(const size_type newSize)
    {
        Resize(newSize, DefaultContructTag{}); // spied on STL
    }

private:
    bool InitialReserveBytes(size_t requestedBytes);

    // TODO test it
    bool CommitOverallMemory(size_t bytes);

    bool CommitAdditionalPage()
    {
        return CommitOverallMemory(GetCommittedBytes() + GetPageSize());
    }

    bool ReleaseMemory()
    {
        if (m_data == nullptr)
        {
            return true;
        }

        const bool success = VirtualFree(
            m_data,                     // Base address of block
            0,                          // Bytes of committed pages, 0 for MEM_RELEASE, non-zero for DECOMMIT
            MEM_RELEASE);               // Decommit the pages
        
        m_data = nullptr;
        assert(success);
        return success;
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
    void EmplaceBackReallocate(Args&&... args)
    {
        ReallocateIfNeed();
        EmplaceAtPlace(&m_data[GetSize()], std::forward<Args...>(args)...);
    }

    template<typename... Args>
    void EmplaceAtPlace(value_type* destination, Args&&... args)
    {
        ObjectLifecycleHelper::ConstructObject<value_type>(destination, std::forward<Args...>(args)...);
        ++m_size;
    }

    void ShiftElementsToTheRight(iterator position, size_type affectedElementsCount)
    {
        assert(position >= Begin() && position < End());
        if constexpr (std::is_nothrow_move_assignable_v<value_type> || !std::is_copy_assignable_v<value_type>)
        {
            std::move_backward(position, Begin() + GetSize(), End() + affectedElementsCount);
        }
        else
        {
            std::copy_backward(position, Begin() + GetSize(), End() + affectedElementsCount);
        }
    }

protected:
    struct DefaultContructTag { // tag to identify default constuction for resize (for now)
        explicit DefaultContructTag() = default;
    };

    [[nodiscard]] inline size_t GetCommittedBytes() const noexcept { return m_committedPages * GetPageSize(); }
    [[nodiscard]] inline size_t GetReservedBytes() const noexcept { return m_reservedPages * GetPageSize(); }

    [[nodiscard]] size_t CalculatePageSize() const
    {
        size_t pageSize = 0;
        if constexpr (!LargePagesEnabled)
        {
            SYSTEM_INFO sSysInfo;                   // Useful information about the system
            GetSystemInfo(&sSysInfo);               // Initialize the structure.

            pageSize = sSysInfo.dwPageSize;       // Page size on this computer
        }
        else
        {
            pageSize = GetLargePageMinimum(); // 2MB on my machine 
        }
        assert(pageSize % ElementSize == 0); // TODO get rid of this assert (keep it as reminder now)

        return pageSize;
    }

    // TODO carry out these functions to separate helper namespace, no need to be part of templated class
    [[nodiscard]] constexpr static size_t CalculateAlignedMemorySize(size_t bytesToAllocate, size_t alignment) noexcept
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

    [[nodiscard]] constexpr static size_t CalculateAlignedGrowthInBytes(
        size_t bytesToAllocate, size_t bytesInUse, size_t pageSize, size_t* outRequiredPages = nullptr) noexcept
    {
        const size_t wantage = bytesToAllocate - bytesInUse;
        const size_t requiredPages = CalculatePageCount(wantage, pageSize);
        if (outRequiredPages != nullptr)
        {
            *outRequiredPages = requiredPages;
        }
        const size_t totalMemoryToCommit = requiredPages * pageSize;

        return totalMemoryToCommit;
    }

    [[nodiscard]] constexpr static size_t CalculatePageCount(size_t bytes, size_t pageSize)
    {
        assert(pageSize != 0);

        const size_t alignedBytes = CalculateAlignedMemorySize(bytes, pageSize);
        assert(alignedBytes % pageSize == 0); // TODO reformulate
        return alignedBytes / pageSize;
    }

    [[nodiscard]] constexpr static size_t CalculateObjectAmountForNBytes(size_t bytes)
    {
        static_assert(ElementSize != 0);
        return bytes / ElementSize;
    }

    [[nodiscard]] constexpr size_t CalculateGrowthInternal(size_t requestedBytes, size_t* outRequiredPages = nullptr) const noexcept
    {
        const size_t alignedGrowthSize = CalculateAlignedGrowthInBytes(requestedBytes, GetCommittedBytes(), GetPageSize(), outRequiredPages);
        assert(alignedGrowthSize % GetPageSize() == 0);

        return alignedGrowthSize;
    }
    /////////////////////////////////////////////////////////////////////////////////////////

private:
    value_type* m_data;
    //size_type m_capacity; // capacity is calculated based on used pages and ElementSize value dynamically
    size_type m_size;

    size_t m_committedPages;
    size_t m_reservedPages;
    mutable size_t m_pageSize; // mutable is used here to initialize the value in getter after reset
};



////////////////// IMPLEMENTATION //////////////////////////////
template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::GrowingVectorVM()
    : m_data(nullptr)
    , m_size(0)
    , m_committedPages(0)
    , m_reservedPages(0)
{
    m_pageSize = CalculatePageSize();

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

    assert(TotalMemoryInBytes % GetPageSize() == 0);
    if (InitialReserveBytes(TotalMemoryInBytes) == false)
    {
        throw std::bad_alloc{};
    }
}

template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::~GrowingVectorVM() noexcept
{
    ReleaseMemory();
}

template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline bool GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::InitialReserveBytes(size_t requestedBytes)
{
    size_t requiredPages = 0;
    const size_t alignedGrowthSize = CalculateGrowthInternal(requestedBytes, &requiredPages);

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
        alignedGrowthSize,          // Size of allocation
        allocationFlags,            // Allocate reserved pages
        protectionFlags);

    if (lpvBase == nullptr)
    {
        return false;
    }

    // Guiding by Exception Safety Guarantee, let's modify state of the object only if everything goes successfully
    m_data = reinterpret_cast<pointer>(lpvBase);

    m_reservedPages = requiredPages;
    m_committedPages = CommitPagesWithReserve ? m_reservedPages : 0;

    std::cout << "GrowingVectorVM preallocated " << alignedGrowthSize << " bytes of vm for " << GetCapacity() << " objects\n";

    return true;
}

template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
inline bool GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::CommitOverallMemory(size_t bytes)
{
    if constexpr (CommitPagesWithReserve)
    {
        // TODO handle this properly
        assert(false);
    }
    else
    {
        if (bytes <= GetCommittedBytes())
        {
            return true;
        }

        if (GetCommittedBytes() + GetPageSize() > GetReservedBytes())
        {
            // TODO think about attempt to extend it
            return false;
        }

        size_t requiredPages = 0;
        const size_t totalMemoryToCommit = CalculateGrowthInternal(bytes, &requiredPages);

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



namespace std
{
    // begin and end implementations - for compatibility with range-based for
    template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
    inline GrowingVectorVM<T, ReservePolicy, LargePagesEnabled,CommitPagesWithReserve>::iterator
        begin(GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>& container)
    {
        return container.Begin();
    }

    template<typename T, typename ReservePolicy, bool LargePagesEnabled, bool CommitPagesWithReserve>
    inline GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>::iterator
        end(GrowingVectorVM<T, ReservePolicy, LargePagesEnabled, CommitPagesWithReserve>& container)
    {
        return container.End();
    }
}