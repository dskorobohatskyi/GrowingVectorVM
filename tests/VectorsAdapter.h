#pragma once

#include "GrowingVectorVM.h"
#include <vector>
#include <optional>

template <typename T, bool useSTD = false, typename ReservePolicy = _4GBSisePolicyTag>
class VectorAdapter {
public:
    using StdVectorType = std::vector<T>;
    using CustomVectorType = GrowingVectorVM<T, ReservePolicy>;
    using InternalVector = std::conditional_t<useSTD, StdVectorType, CustomVectorType>;

    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    using iterator = std::conditional_t <useSTD, typename StdVectorType::iterator, typename CustomVectorType::iterator>;
    using const_iterator = std::conditional_t <useSTD, typename StdVectorType::const_iterator, typename CustomVectorType::const_iterator>;
    using reverse_iterator = std::conditional_t <useSTD, typename StdVectorType::reverse_iterator, typename CustomVectorType::reverse_iterator>;
    using const_reverse_iterator = std::conditional_t <useSTD, typename StdVectorType::const_reverse_iterator, typename CustomVectorType::const_reverse_iterator>;

    InternalVector& GetInternalVectorRef()
    {
        if constexpr (useSTD)
        {
            return stdVector.value();
        }
        else
        {
            return myVector.value();
        }
    }

    std::optional<InternalVector>& GetInternalVectorOpt()
    {
        if constexpr (useSTD)
        {
            return stdVector;
        }
        else
        {
            return myVector;
        }
    }


    // Constructors
    VectorAdapter()
    {
        _ValidateEmptiness();

        // FIXME default and move ctor here(
        GetInternalVectorOpt().emplace(InternalVector{});
    }

    VectorAdapter(VectorAdapter&& other) noexcept
    {
        _ValidateEmptiness();

        GetInternalVectorOpt().emplace(std::move(other.GetInternalVectorRef()));
    }

    VectorAdapter& operator=(VectorAdapter&& other) noexcept
    {
        if (this != &other)
        {
            GetInternalVectorRef() = std::move(other.GetInternalVectorRef());
        }
        return *this;
    }

    // Disable copy construction for now, since memory reserve should be done for another vector as well and it will throw without careful setup.
    VectorAdapter(const VectorAdapter& other) = delete;
    VectorAdapter& operator=(const VectorAdapter& other) = delete;
    /////////////////////////////////////////////////////////////////////

    explicit VectorAdapter(const size_type count)
    {
        _ValidateEmptiness();
        GetInternalVectorOpt().emplace(InternalVector(count));
    }

    VectorAdapter(const size_type count, const value_type& value)
    {
        _ValidateEmptiness();
        GetInternalVectorOpt().emplace(InternalVector( count, value ));
    }

    template<typename InputIt, std::enable_if_t<std::_Is_iterator_v<InputIt>, int> = 0>
    VectorAdapter(InputIt first, InputIt last)
    {
        _ValidateEmptiness();
        GetInternalVectorOpt().emplace(InternalVector( first, last ));
    }

    VectorAdapter(std::initializer_list<T> ilist)
    {
        _ValidateEmptiness();

        GetInternalVectorOpt().emplace(InternalVector( ilist ));
    }

    VectorAdapter& operator=(std::initializer_list<T> ilist)
    {
        GetInternalVectorRef() = ilist;
        return *this;
    }

    void Swap(VectorAdapter& other) noexcept
    {
        std::swap(GetInternalVectorRef(), other.GetInternalVectorRef());
    }

    [[nodiscard]] inline size_type GetSize() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->size();
        }
        else
        {
            return myVector->GetSize();
        }
    }
    [[nodiscard]] inline size_type GetCapacity() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->capacity();
        }
        else
        {
            return myVector->GetCapacity();
        }
    }
    [[nodiscard]] inline size_type GetReserve() const noexcept
    {
        if constexpr (useSTD)
        {
            return 0; // TODO not applicable
        }
        else
        {
            return myVector->GetReserve();
        }
    }
    [[nodiscard]] size_type GetPageSize() const noexcept // should it be public?
    {
        if constexpr (useSTD)
        {
            return 0; // todo n/a
        }
        else
        {
            return myVector->GetPageSize();
        }
    }

    [[nodiscard]] inline pointer GetData() noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->data();
        }
        else
        {
            return myVector->GetData();
        }
    }
    [[nodiscard]] inline const_pointer GetData() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->data();
        }
        else
        {
            return myVector->GetData();
        }
    }

    [[nodiscard]] inline const value_type& Back() const
    {
        if constexpr (useSTD)
        {
            return stdVector->back();
        }
        else
        {
            return myVector->Back();
        }
    }
    [[nodiscard]] inline value_type& Back()
    {
        if constexpr (useSTD)
        {
            return stdVector->back();
        }
        else
        {
            return myVector->Back();
        }
    }

    [[nodiscard]] inline const value_type& Front() const
    {
        if constexpr (useSTD)
        {
            return stdVector->front();
        }
        else
        {
            return myVector->Front();
        }
    }
    [[nodiscard]] inline value_type& Front()
    {
        if constexpr (useSTD)
        {
            return stdVector->front();
        }
        else
        {
            return myVector->Front();
        }
    }


    [[nodiscard]] inline iterator Begin() noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->begin();
        }
        else
        {
            return myVector->Begin();
        }
    }
    [[nodiscard]] inline const_iterator Begin() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->begin();
        }
        else
        {
            return myVector->Begin();
        }
    }
    [[nodiscard]] inline iterator End() noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->end();
        }
        else
        {
            return myVector->End();
        }
    }
    [[nodiscard]] inline const_iterator End() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->end();
        }
        else
        {
            return myVector->End();
        }
    }

    [[nodiscard]] inline const_iterator CBegin() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->cbegin();
        }
        else
        {
            return myVector->CBegin();
        }
    }
    [[nodiscard]] inline const_iterator CEnd() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->cend();
        }
        else
        {
            return myVector->CEnd();
        }
    }

    // Reverse versions
    [[nodiscard]] inline reverse_iterator RBegin() noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->rbegin();
        }
        else
        {
            return myVector->RBegin();
        }
    }
    [[nodiscard]] inline const_reverse_iterator RBegin() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->rbegin();
        }
        else
        {
            return myVector->RBegin();
        }
    }
    [[nodiscard]] inline reverse_iterator REnd() noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->rend();
        }
        else
        {
            return myVector->REnd();
        }
    }
    [[nodiscard]] inline const_reverse_iterator REnd() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->rend();
        }
        else
        {
            return myVector->REnd();
        }
    }

    [[nodiscard]] inline const_reverse_iterator CRBegin() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->crbegin();
        }
        else
        {
            return myVector->CRBegin();
        }
    }
    [[nodiscard]] inline const_reverse_iterator CREnd() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->crend();
        }
        else
        {
            return myVector->CREnd();
        }
    }


    // Note: can throw with bad_alloc if reserve limitation is exceed or allocation was failed
    void Reserve(const size_t elementAmount)
    {
        if constexpr (useSTD)
        {
            return stdVector->reserve(elementAmount);
        }
        else
        {
            return myVector->Reserve(elementAmount);
        }
    }

    [[nodiscard]] inline bool Empty() const noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->empty();
        }
        else
        {
            return myVector->Empty();
        }
    }

    const value_type& operator[](size_type index) const
    {
        return GetInternalVectorRef()[index];
    }

    value_type& operator[](size_type index)
    {
        return GetInternalVectorRef()[index];
    }

    [[nodiscard]] reference At(size_type index)
    {
        return this->operator[](index);
    }

    [[nodiscard]] const_reference At(size_type index) const
    {
        return this->operator[](index);
    }

    [[nodiscard]] value_type& At(size_type index, const value_type& defValue)
    {
        if constexpr (useSTD)
        {
            return stdVector->at(index, defValue);
        }
        else
        {
            return myVector->At(index, defValue);
        }
    }

    [[nodiscard]] const value_type& At(size_type index, const value_type& defValue) const
    {
        if constexpr (useSTD)
        {
            return stdVector->at(index, defValue);
        }
        else
        {
            return myVector->At(index, defValue);
        }
    }

    void PushBack(const value_type& value)
    {
        if constexpr (useSTD)
        {
            return stdVector->push_back(value);
        }
        else
        {
            return myVector->PushBack(value);
        }
    }

    void PushBack(const value_type&& value)
    {
        if constexpr (useSTD)
        {
            return stdVector->push_back(std::move(value));
        }
        else
        {
            return myVector->PushBack(std::move(value));
        }
    }

    void PopBack()
    {
        if constexpr (useSTD)
        {
            return stdVector->pop_back();
        }
        else
        {
            return myVector->PopBack();
        }
    }

    template<typename... Args>
    void EmplaceBack(Args&&... args)
    {
        if constexpr (useSTD)
        {
            stdVector->emplace_back(std::forward<Args...>(args)...);
        }
        else
        {
            myVector->EmplaceBack(std::forward<Args...>(args)...);
        }
    }

    iterator Insert(const_iterator position, const value_type& value)
    {
        if constexpr (useSTD)
        {
            return stdVector->insert(position, value);
        }
        else
        {
            return myVector->Insert(position, value);
        }
    }

    iterator Insert(const_iterator position, const value_type&& value)
    {
        if constexpr (useSTD)
        {
            return stdVector->insert(position, std::move(value));
        }
        else
        {
            return myVector->Insert(position, std::move(value));
        }
    }

    iterator Insert(const_iterator position, const size_type count, const value_type& value)
    {
        if constexpr (useSTD)
        {
            return stdVector->insert(position, count, value);
        }
        else
        {
            return myVector->Insert(position, count, value);
        }
    }

    template<typename... Args>
    iterator Emplace(const_iterator position, Args&&... args)
    {
        if constexpr (useSTD)
        {
            return stdVector->emplace(position, std::forward<Args...>(args)...);
        }
        else
        {
            return myVector->Emplace(position, std::forward<Args...>(args)...);
        }
    }

    iterator Erase(const_iterator position)
    {
        if constexpr (useSTD)
        {
            return stdVector->erase(position);
        }
        else
        {
            return myVector->Erase(position);
        }
    }

    iterator Erase(const_iterator first, const_iterator last)
    {
        if constexpr (useSTD)
        {
            return stdVector->erase(first, last);
        }
        else
        {
            return myVector->Erase(first, last);
        }
    }

    void Clear() noexcept
    {
        if constexpr (useSTD)
        {
            return stdVector->clear();
        }
        else
        {
            return myVector->Clear();
        }
    }

    void Resize(const size_t newSize, const T& def)
    {
        if constexpr (useSTD)
        {
            return stdVector->resize(newSize, def);
        }
        else
        {
            return myVector->Resize(newSize, def);
        }
    }

    void Resize(const size_t newSize)
    {
        if constexpr (useSTD)
        {
            return stdVector->resize(newSize);
        }
        else
        {
            return myVector->Resize(newSize);
        }
    }

private:
    void _ValidateEmptiness()
    {
        assert(!stdVector.has_value());
        assert(!myVector.has_value());
    }

private:
    // Data members for both vector types
    std::optional<StdVectorType> stdVector;
    std::optional<CustomVectorType> myVector;
};

