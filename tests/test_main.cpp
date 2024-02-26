#include "GrowingVectorVM.h"
#include <gtest/gtest.h>
#include "VectorsAdapter.h"

#include <algorithm>
#include <numeric>      // for std::iota

template <class T>
class VectorTest : public testing::Test {

protected:
    constexpr static bool useStd = T::value;
};

typedef testing::Types<std::true_type, std::false_type> Variants;
TYPED_TEST_SUITE(VectorTest, Variants);

TYPED_TEST(VectorTest, VectorDefaultCtor)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(adapter.Empty());
}

TYPED_TEST(VectorTest, VectorCapacityCtor)
{
    constexpr size_t count = 10;
    VectorAdapter<int, this->useStd> adapter(count);
    constexpr int defaultValue = 0;

    ASSERT_EQ(adapter.GetSize(), count);
    for (size_t i(0); i < count; i++)
    {
        ASSERT_EQ(adapter[i], defaultValue);
    }
}

TYPED_TEST(VectorTest, VectorCapacityCtorWithDefaultValue)
{
    constexpr size_t count = 10;
    constexpr int defaultValue = 99;
    VectorAdapter<int, this->useStd> adapter(count, defaultValue);

    ASSERT_EQ(adapter.GetSize(), count);
    for (size_t i(0); i < count; i++)
    {
        ASSERT_EQ(adapter[i], defaultValue);
    }
}

TYPED_TEST(VectorTest, VectorCtorWithInitializerList)
{
    VectorAdapter<int, this->useStd> adapter({ 1, 2, 3, 4, 5});
    ASSERT_EQ(adapter.GetSize(), 5);

    for (size_t i(0); i < adapter.GetSize(); i++)
    {
        ASSERT_EQ(adapter[i], i + 1);
    }
}

TYPED_TEST(VectorTest, VectorAssignOperatorWithInitializerList)
{
    VectorAdapter<int, this->useStd> adapter; //default ctor
    adapter = { 1, 2, 3, 4, 5 };
    ASSERT_EQ(adapter.GetSize(), 5);

    for (size_t i(0); i < adapter.GetSize(); i++)
    {
        ASSERT_EQ(adapter[i], i + 1);
    }
}

TYPED_TEST(VectorTest, VectorAssignOperatorWithInitializerListReplaceData)
{
    VectorAdapter<int, this->useStd> adapter({ 99, 98, 97 });
    ASSERT_EQ(adapter.GetSize(), 3);
    ASSERT_EQ(adapter[2], 97);

    adapter = { 1, 2, 3, 4, 5 };
    ASSERT_EQ(adapter.GetSize(), 5);

    for (size_t i(0); i < adapter.GetSize(); i++)
    {
        ASSERT_EQ(adapter[i], i + 1);
    }
}

TYPED_TEST(VectorTest, VectorCtorWithIterators)
{
    VectorAdapter<int, this->useStd> adapter({ 1, 2, 3, 44, 5 });
    ASSERT_EQ(adapter.GetSize(), 5);

    auto rangeStartIt = adapter.CBegin() + 2;
    ASSERT_EQ(*rangeStartIt, 3); // should point to 3
    auto rangeEndIt = adapter.CEnd();
    ASSERT_EQ(*(rangeEndIt - 1), 5); // should point to next after 5
    ASSERT_EQ(rangeEndIt - rangeStartIt, 3);

    VectorAdapter<int, this->useStd> partialCopiedVec(rangeStartIt, rangeEndIt);
    ASSERT_EQ(partialCopiedVec.GetSize(), 3);
    ASSERT_EQ(partialCopiedVec[0], 3);
    ASSERT_EQ(partialCopiedVec[1], 44);
}


TYPED_TEST(VectorTest, VectorMoveSemantic)
{
    VectorAdapter<int, this->useStd> adapter(4, 100);
    ASSERT_EQ(adapter.GetSize(), 4);

    VectorAdapter<int, this->useStd> adapter2(std::move(adapter));
    ASSERT_EQ(adapter.GetSize(), 0);
    ASSERT_EQ(adapter.Empty(), true);

    adapter = std::move(adapter2); // move operator
    const auto size = adapter.GetSize();
    ASSERT_EQ(size, 4);

    ASSERT_EQ(adapter2.Empty(), true);
    ASSERT_EQ(adapter2.GetData(), nullptr);
}

TYPED_TEST(VectorTest, VectorSwaping)
{
    const int initValue = 10;
    VectorAdapter<int, this->useStd> adapter1(10, initValue);
    ASSERT_TRUE(adapter1.GetSize() == 10);
    VectorAdapter<int, this->useStd> adapter2(20);
    ASSERT_TRUE(adapter2.GetSize() == 20);

    std::swap(adapter1.GetInternalVectorRef(), adapter2.GetInternalVectorRef());
    ASSERT_TRUE(adapter1.GetSize() == 20);
    ASSERT_TRUE(adapter2.GetSize() == 10);

    // STD compat check
    ASSERT_TRUE(std::all_of(adapter2.CBegin(), adapter2.CEnd(), [initValue](const int e)
        {
            return e == initValue;
        }));
}


TYPED_TEST(VectorTest, VectorQuickAccessGetters)
{
    VectorAdapter<int, this->useStd> adapter;

    // Test PushBack, Front, and Back
    adapter.PushBack(1);
    EXPECT_EQ(adapter.Front(), 1);
    EXPECT_EQ(adapter.Back(), 1);

    adapter.PushBack(2);
    EXPECT_EQ(adapter.Front(), 1);
    EXPECT_EQ(adapter.Back(), 2);

    adapter.PushBack(3);
    EXPECT_EQ(adapter.Front(), 1);
    EXPECT_EQ(adapter.Back(), 3);

    // Modify the adapter and test again
    adapter.Front() = 0;
    adapter.Back() = 4;

    EXPECT_EQ(adapter.Front(), 0);
    EXPECT_EQ(adapter.Back(), 4);

    // Test with an empty adapter
    VectorAdapter<int, this->useStd> emptyAdapter;
    // Ensure calling Front() or Back() on an empty adapter doesn't cause undefined behavior
    EXPECT_EQ(emptyAdapter.GetSize(), 0);
    if (this->useStd)
    {
#if !NDEBUG
        EXPECT_DEATH({
            auto f = emptyAdapter.Front();
            auto b = emptyAdapter.Back();
            }, ".*");
#endif
    }
    else
    {
        EXPECT_ANY_THROW({
            auto f = emptyAdapter.Front();
            auto b = emptyAdapter.Back();
        });
    }
}

TYPED_TEST(VectorTest, VectorIteratorsCheck)
{
    VectorAdapter<int, this->useStd> adapter;

    adapter.PushBack(1);
    adapter.PushBack(2);
    adapter.PushBack(3);

    // Test forward iterators (Begin, End, CBegin, CEnd)
    EXPECT_EQ(*adapter.Begin(), 1);
    EXPECT_EQ(*adapter.CBegin(), 1);

    {
        auto it = adapter.Begin();
        ++it;
        EXPECT_EQ(*it, 2);
    }

    // Iterate using a range-based for loop
    int sum = 0;
    for (const auto& value : adapter.GetInternalVectorRef()) {
        sum += value;
    }
    EXPECT_EQ(sum, 6);

    // Test reverse iterators (RBegin, REnd, CRBegin, CREnd)
    EXPECT_EQ(*adapter.RBegin(), 3);
    EXPECT_EQ(*adapter.CRBegin(), 3);

    {
        auto it = adapter.CREnd();
        it--;
        EXPECT_EQ(*it, 1);
    }


    auto rit = adapter.RBegin();
    ++rit;
    EXPECT_EQ(*rit, 2);
}

TYPED_TEST(VectorTest, VectorPushPopCommon)
{
    VectorAdapter<int, this->useStd> adapter;

#if !NDEBUG
    EXPECT_DEATH({
        adapter.PopBack();
    }, ".*");
#endif

    EXPECT_EQ(adapter.GetSize(), 0);
    adapter.PushBack(1);
    EXPECT_EQ(adapter.GetSize(), 1);
    adapter.PushBack(2);
    EXPECT_EQ(adapter.GetSize(), 2);
    adapter.PushBack(3);
    EXPECT_EQ(adapter.GetSize(), 3);

    // Check the content of the adapter
    EXPECT_EQ(adapter.Front(), 1);
    EXPECT_EQ(adapter.Back(), 3);

    EXPECT_EQ(adapter.GetSize(), 3);
    adapter.PopBack();
    EXPECT_EQ(adapter.GetSize(), 2);
    adapter.PopBack();
    EXPECT_EQ(adapter.GetSize(), 1);
    adapter.PopBack();
    EXPECT_EQ(adapter.GetSize(), 0);

#if !NDEBUG
    EXPECT_DEATH({
        adapter.PopBack();
    }, ".*");
#endif
}

TYPED_TEST(VectorTest, VectorPushPopExtend)
{
    struct CustomData
    {
        explicit CustomData(size_t v)
            : value(v)
            , flag(false)
        {

        }
        size_t value;
        bool flag;
    };
    static_assert(sizeof(CustomData) == 16);
    static_assert(alignof(CustomData) == sizeof(size_t));
    static_assert(!std::is_default_constructible_v<CustomData>);

    constexpr size_t bytes = DS_MB(512);
    VectorAdapter<CustomData, this->useStd, CustomSizePolicyTag<bytes>> adapter;

    constexpr size_t objectAmountLimit = bytes / sizeof(CustomData);

    for (size_t i = 0; i < objectAmountLimit; i++)
    {
        adapter.PushBack(CustomData{i});
    }

    if (!this->useStd)
    {
        EXPECT_THROW({
            adapter.PushBack(CustomData{objectAmountLimit});
            }, std::bad_alloc);
    }
}

TYPED_TEST(VectorTest, VectorEmplaces)
{
    struct TestData {
        int value;

        // Add any necessary constructors or operations for TestData
        explicit TestData(int v) : value(v) {} // without explicit
    };

    VectorAdapter<TestData, this->useStd> adapter;

    {
        // EmplaceBack elements and check GetSize()
        EXPECT_EQ(adapter.GetSize(), 0);
        adapter.EmplaceBack(1);
        EXPECT_EQ(adapter.GetSize(), 1);
        adapter.EmplaceBack(2);
        EXPECT_EQ(adapter.GetSize(), 2);
        adapter.EmplaceBack(3);
        EXPECT_EQ(adapter.GetSize(), 3);

        // Check the content of the adapter
        EXPECT_EQ(adapter.Front().value, 1);
        EXPECT_EQ(adapter.Back().value, 3);
    }
    adapter.Clear();

    {
        adapter.EmplaceBack(2);
        adapter.EmplaceBack(3);

        // Test Emplace at iterator position and check GetSize()
        auto it = adapter.Begin();
        it = adapter.Emplace(it, 1);
        EXPECT_EQ(adapter.GetSize(), 3);

        // Check the content of the adapter
        EXPECT_EQ(adapter.Front().value, 1);
        EXPECT_EQ(it->value, 1);
        EXPECT_EQ(adapter.Back().value, 3);

        // Emplace at the end and check GetSize()
        it = adapter.Emplace(adapter.End(), 4);
        EXPECT_EQ(adapter.GetSize(), 4);

        // Check the content of the adapter
        EXPECT_EQ(adapter.Back().value, 4);
    }
}

TYPED_TEST(VectorTest, VectorEmplaceWithCapacityChange)
{
    VectorAdapter<int, this->useStd> adapter;
    
    EXPECT_EQ(adapter.GetSize(), 0);
    EXPECT_EQ(adapter.GetCapacity(), 0);

    const int anchorValue = 100;
    adapter.EmplaceBack(anchorValue);
    EXPECT_EQ(adapter.GetSize(), 1);
    int* validDataPtr = adapter.GetData();
    EXPECT_TRUE(validDataPtr != nullptr);

    const size_t oldCapacity = adapter.GetCapacity();
    const size_t objectAmountToFillCapacity = oldCapacity - 1;
    for (size_t i = 0; i < objectAmountToFillCapacity; i++)
    {
        adapter.Emplace(adapter.CBegin(), (int)i); // should shift anchorValue to the right
        EXPECT_EQ(adapter.Back(), anchorValue);
    }

    EXPECT_EQ(adapter.GetCapacity(), oldCapacity);

    bool invalidationPresense = validDataPtr != adapter.GetData();
    EXPECT_FALSE(invalidationPresense); // for both vectors valid

    adapter.Emplace(adapter.CEnd(), -anchorValue);
    EXPECT_NE(adapter.GetCapacity(), oldCapacity); // capacity changed
    EXPECT_EQ(adapter.GetSize(), oldCapacity + 1);

    // value checks (does shifting work well)
    for (size_t i = 0, expected = objectAmountToFillCapacity - 1; i < objectAmountToFillCapacity; i++)
    {
        EXPECT_EQ(adapter[i], expected);
        expected--;
    }
    EXPECT_EQ(adapter.Back(), -anchorValue);
    EXPECT_EQ(*(adapter.CEnd() - 2), anchorValue);

    invalidationPresense = validDataPtr != adapter.GetData();
    if (this->useStd)
    {
        EXPECT_TRUE(invalidationPresense);
    }
    else
    {
        EXPECT_FALSE(invalidationPresense);
    }
}

TYPED_TEST(VectorTest, VectorInserts)
{
    struct TestData {
        int value;

        // Add any necessary constructors or operations for TestData
        explicit TestData(int v) : value(v) {} // without explicit
    };
    VectorAdapter<TestData, this->useStd> adapter;

    adapter.EmplaceBack(1);
    adapter.EmplaceBack(3);

    // Test Insert single element and check size and content
    auto it = adapter.Insert(adapter.Begin() + 1, TestData(2));
    EXPECT_EQ(adapter.GetSize(), 3);
    EXPECT_EQ(adapter.Front().value, 1);
    EXPECT_EQ((*it).value, 2);
    EXPECT_EQ(adapter.Back().value, 3);

    // Test Insert multiple elements and check size and content
    TestData newData(4);
    adapter.Insert(adapter.Begin(), 2, newData);
    EXPECT_EQ(adapter.GetSize(), 5);
    EXPECT_EQ(adapter.Front().value, 4);
    EXPECT_EQ((adapter.Begin() + 1)->value, 4);
    EXPECT_EQ((adapter.Begin() + 2)->value, 1);
    EXPECT_EQ((adapter.Begin() + 3)->value, 2);
    EXPECT_EQ((adapter.Begin() + 4)->value, 3);
    EXPECT_EQ(adapter.Back().value, 3);

    // Test Insert at the end
    adapter.Insert(adapter.End(), TestData(5));
    EXPECT_EQ(adapter.GetSize(), 6);
    EXPECT_EQ(adapter.Back().value, 5);
}

TYPED_TEST(VectorTest, VectorInsertWithCapacityChange)
{
    VectorAdapter<int, this->useStd> adapter;

    EXPECT_EQ(adapter.GetSize(), 0);
    EXPECT_EQ(adapter.GetCapacity(), 0);

    const int anchorValue = 100;
    adapter.EmplaceBack(anchorValue);
    EXPECT_EQ(adapter.GetSize(), 1);
    int* validDataPtr = adapter.GetData();
    EXPECT_TRUE(validDataPtr != nullptr);

    const size_t oldCapacity = adapter.GetCapacity();
    const size_t objectAmountToFillCapacity = oldCapacity - 1;
    for (size_t i = 0; i < objectAmountToFillCapacity; i++)
    {
        adapter.Insert(adapter.CBegin(), (int)i); // should shift anchorValue to the right
        EXPECT_EQ(adapter.Back(), anchorValue);
    }

    EXPECT_EQ(adapter.GetCapacity(), oldCapacity);

    bool invalidationPresense = validDataPtr != adapter.GetData();
    EXPECT_FALSE(invalidationPresense); // for both vectors valid

    adapter.Insert(adapter.CEnd(), -anchorValue);
    EXPECT_NE(adapter.GetCapacity(), oldCapacity); // capacity changed
    EXPECT_EQ(adapter.GetSize(), oldCapacity + 1);

    // value checks (does shifting work well)
    for (size_t i = 0, expected = objectAmountToFillCapacity - 1; i < objectAmountToFillCapacity; i++)
    {
        EXPECT_EQ(adapter[i], expected);
        expected--;
    }
    EXPECT_EQ(adapter.Back(), -anchorValue);
    EXPECT_EQ(*(adapter.CEnd() - 2), anchorValue);

    invalidationPresense = validDataPtr != adapter.GetData();
    if (this->useStd)
    {
        EXPECT_TRUE(invalidationPresense);
    }
    else
    {
        EXPECT_FALSE(invalidationPresense);
    }
}

TYPED_TEST(VectorTest, VectorInsertCountWithCapacityChange)
{
    VectorAdapter<int, this->useStd> adapter;
    adapter.Insert(adapter.CBegin(), -1);
    const int fillingValue = 10;

    constexpr int N = 100'000;
    adapter.Insert(adapter.CEnd(), N, fillingValue);
    adapter.PushBack(1);

    const int result = std::accumulate(adapter.CBegin(), adapter.CEnd(), 0);
    const int expectedSum = -1 + N * fillingValue + 1;
    EXPECT_EQ(result, expectedSum);

    EXPECT_EQ(adapter.Front(), -1);
    EXPECT_EQ(adapter.Back(), 1);
}

TYPED_TEST(VectorTest, VectorErase)
{
    VectorAdapter<int, this->useStd> adapter = { 1, 2, 0, 0, 3, 4, 5, 6, };
    EXPECT_EQ(adapter.End(), adapter.Begin() + adapter.GetSize());
    const size_t oldCapacity = adapter.GetCapacity();
    const size_t oldSize = adapter.GetSize();

    auto startIter = std::find(adapter.Begin(), adapter.End(), 2) + 1;
    auto endIter = std::find(adapter.Begin(), adapter.End(), 3);

    const size_t expectedRemovingRangeSize = (size_t)std::distance(startIter, endIter);

    auto nextAfterRemovingIter = adapter.Erase(startIter, endIter);
    auto expectedIterator = std::find(adapter.Begin(), adapter.End(), 3);
    EXPECT_EQ(nextAfterRemovingIter, expectedIterator);

    EXPECT_EQ(adapter.GetCapacity(), oldCapacity);
    EXPECT_EQ(adapter.GetSize(), oldSize - expectedRemovingRangeSize);
}

TYPED_TEST(VectorTest, VectorClear)
{
    VectorAdapter<int, this->useStd> adapter;

    adapter.EmplaceBack(1);
    adapter.EmplaceBack(2);
    adapter.EmplaceBack(3);

    // Save initial capacity
    const size_t initialCapacity = adapter.GetCapacity();

    // Test Clear and check size and capacity
    adapter.Clear();
    EXPECT_EQ(adapter.GetSize(), 0);
    EXPECT_EQ(adapter.GetCapacity(), initialCapacity);

    // Test Clear on an empty vector
    adapter.Clear();
    EXPECT_EQ(adapter.GetSize(), 0);
    EXPECT_EQ(adapter.GetCapacity(), initialCapacity);

    // Add more elements after clearing
    adapter.EmplaceBack(4);
    adapter.EmplaceBack(5);

    // Check size and capacity after adding more elements
    EXPECT_EQ(adapter.Front(), 4);
    EXPECT_EQ(adapter.Back(), 5);
}

TYPED_TEST(VectorTest, VectorResizeWithinCapacity)
{
    VectorAdapter<int, this->useStd> adapter;

    constexpr size_t reservedCapacity = 1000; // NOTE: for GrowingVector can be differ after Reserve call()
    adapter.Reserve(reservedCapacity);

    adapter.Resize(10); // default constractible
    EXPECT_TRUE(std::is_default_constructible_v<int> == true);

    for (size_t i = 0; i < 10; i++)
    {
        EXPECT_EQ(adapter.At(i), 0);
    }

    adapter.Resize(200, 99);

    for (size_t i = 10; i < 200; i++)
    {
        EXPECT_EQ(adapter.At(i), 99);
    }

    const size_t oldCapacity = adapter.GetCapacity();
    adapter.Resize(reservedCapacity, 100);
    EXPECT_EQ(adapter.GetCapacity(), oldCapacity);

    for (size_t i = 200; i < reservedCapacity; i++)
    {
        EXPECT_EQ(adapter.At(i), 100);
    }
}

TYPED_TEST(VectorTest, VectorResizeOutsideCapacity)
{
    VectorAdapter<int, this->useStd> adapter;

    constexpr size_t reservedCapacity = 10; // NOTE: for GrowingVector can be differ after Reserve call()
    adapter.Reserve(reservedCapacity);
    EXPECT_EQ(adapter.GetSize(), 0);

    size_t oldCapacity = adapter.GetCapacity();
    adapter = { 1, 2, 3 };
    EXPECT_EQ(adapter.GetCapacity(), oldCapacity);

    adapter.Resize(oldCapacity * 2, 200);
    EXPECT_NE(adapter.GetCapacity(), oldCapacity);

    EXPECT_EQ(adapter.At(0), 1);
    EXPECT_EQ(adapter.At(1), 2);
    EXPECT_EQ(adapter.At(2), 3);
    for (size_t i = 3; i < oldCapacity * 2; i++)
    {
        EXPECT_EQ(adapter.At(i), 200);
    }

    adapter.Resize(5000, 99);

    EXPECT_EQ(adapter.At(oldCapacity * 2 - 1), 200);
    EXPECT_EQ(adapter.At(oldCapacity * 2), 99);
    EXPECT_EQ(adapter.Back(), 99);
}


//////////////// STL COMPATIBILITY //////////////////////////////////

TYPED_TEST(VectorTest, Sorting)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };

    std::sort(adapter.Begin(), adapter.End());

    ASSERT_TRUE(std::is_sorted(adapter.Begin(), adapter.End()));
}

TYPED_TEST(VectorTest, MinElement)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };

    auto minElement = std::min_element(adapter.Begin(), adapter.End());

    EXPECT_EQ(1, *minElement);
}

TYPED_TEST(VectorTest, Accumulate)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };

    const int sum = std::accumulate(adapter.Begin(), adapter.End(), 0);

    EXPECT_EQ(45, sum);
}

TYPED_TEST(VectorTest, AllOf)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, -1, 7, 3, 9, 4, 6 };

    bool allPositive = std::all_of(adapter.Begin(), adapter.End(), [](int x) { return x > 0; });

    EXPECT_FALSE(allPositive);
}

TYPED_TEST(VectorTest, Transform)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };
    VectorAdapter<int, this->useStd> resultVector(adapter.GetSize());

    std::transform(adapter.Begin(), adapter.End(), resultVector.Begin(), [](int x) { return x * 2; });

    auto expected(VectorAdapter<int, this->useStd>{10, 4, 16, 2, 14, 6, 18, 8, 12});
    // TODO uncomment when operator== is overriden
    //EXPECT_EQ(resultVector, expected);
    for (size_t i = 0; i < resultVector.GetSize(); i++)
    {
        EXPECT_EQ(resultVector[i], expected[i]);
    }
}

TYPED_TEST(VectorTest, Unique)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6, 5, 2, 8 };

    std::sort(adapter.Begin(), adapter.End());
    auto uniqueEnd = std::unique(adapter.Begin(), adapter.End());
    adapter.Erase(uniqueEnd, adapter.End());

    auto expected(VectorAdapter<int, this->useStd>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    // TODO uncomment when operator== is overriden
    //EXPECT_EQ(resultVector, expected);
    for (size_t i = 0; i < adapter.GetSize(); i++)
    {
        EXPECT_EQ(adapter[i], expected[i]);
    }
}

TYPED_TEST(VectorTest, Reverse)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };

    std::reverse(adapter.Begin(), adapter.End());

    auto expected(VectorAdapter<int, this->useStd>{6, 4, 9, 3, 7, 1, 8, 2, 5});
    // TODO uncomment when operator== is overriden
    //EXPECT_EQ(adapter, expected);
    for (size_t i = 0; i < adapter.GetSize(); i++)
    {
        EXPECT_EQ(adapter[i], expected[i]);
    }
}

TYPED_TEST(VectorTest, Count)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };

    const int countOfThrees = std::count(adapter.Begin(), adapter.End(), 3);

    EXPECT_EQ(countOfThrees, 1);
}

TYPED_TEST(VectorTest, IsSorted)
{
    VectorAdapter<int, this->useStd> adapter = { 1, 2, 3, 4, 5 };

    const bool isSorted = std::is_sorted(adapter.Begin(), adapter.End());

    EXPECT_TRUE(isSorted);
}

TYPED_TEST(VectorTest, ForEach)
{
    VectorAdapter<int, this->useStd> adapter = { 5, 2, 8, 1, 7, 3, 9, 4, 6 };

    std::for_each(adapter.Begin(), adapter.End(), [](int& x) { x += 10; });
    auto expected(VectorAdapter<int, this->useStd>{15, 12, 18, 11, 17, 13, 19, 14, 16});
    // TODO uncomment when operator== is overriden
    //EXPECT_EQ(adapter, expected);
    for (size_t i = 0; i < adapter.GetSize(); i++)
    {
        EXPECT_EQ(adapter[i], expected[i]);
    }
}
//////////////////////////////////////////////////


TYPED_TEST(VectorTest, VectorRangeBasedFor)
{
    constexpr const size_t elementsAmount = 100;
    VectorAdapter<int, this->useStd> adapter;
    adapter.Resize(elementsAmount);
    std::iota(adapter.Begin(), adapter.End(), 0); // fill vector in progression way with step 1: 0, 1, 2.... 99

    size_t counter = 0;
    for (const int elem : adapter.GetInternalVectorRef())
    {
        ASSERT_EQ(elem, counter++);
    }
}

///////////////////////////////////////////////////

TEST(GrowingVectorTest, VectorNativeCtorReserveCheck)
{
    {
        GrowingVectorVM<int, _4GBSisePolicyTag> grVec; // default ctor
        ASSERT_TRUE(grVec.GetSize() == 0);
        ASSERT_TRUE(grVec.GetCapacity() == 0);
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(4) / sizeof(int));
    }
    {
        GrowingVectorVM<double, _4GBSisePolicyTag> grVec({ 1, 2, 3 }); // ctor with initializer_list
        ASSERT_TRUE(grVec.GetSize() == 3);
        ASSERT_TRUE(grVec.GetCapacity() == grVec.GetPageSize() / sizeof(double));
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(4) / sizeof(double));

        ASSERT_TRUE(grVec[2] == 3);
    }
    {
        GrowingVectorVM<char, _16GBSisePolicyTag> grVec(10, 10); // ctor with count and value
        ASSERT_TRUE(grVec.GetSize() == 10);
        ASSERT_TRUE(grVec.GetCapacity() == grVec.GetPageSize() / sizeof(char));
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(16) / sizeof(char));
    }
    {
        struct SomeStruct
        {
            int a = 0;
            size_t size = 0;
            void* data = nullptr;
        };
        static_assert(alignof(SomeStruct) == sizeof(size_t));
        using CheckingType = SomeStruct;

        // Calculate RAM size
        const unsigned long long TotalMemoryInBytes = PlatformHelper::CalculateInstalledRAM();
        GrowingVectorVM<CheckingType, RAMSizePolicyTag> grVec(20); // ctor with default count object
        ASSERT_TRUE(grVec.GetSize() == 20);
        ASSERT_TRUE(grVec.GetCapacity() == grVec.GetPageSize() / sizeof(CheckingType));
        ASSERT_TRUE(grVec.GetReserve() == TotalMemoryInBytes / sizeof(CheckingType));
    }
}

TEST(GrowingVectorTest, VectorReservePolicyCheck)
{
    {
        GrowingVectorVM<int, _4GBSisePolicyTag> grVec; // default ctor
        ASSERT_TRUE(grVec.GetSize() == 0);
        ASSERT_TRUE(grVec.GetCapacity() == 0);
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(4) / sizeof(int));
    }
    {
        GrowingVectorVM<int, _8GBSisePolicyTag> grVec({ 1, 2, 3 }); // ctor with initializer_list
        ASSERT_TRUE(grVec.GetSize() == 3);
        ASSERT_TRUE(grVec.GetCapacity() == grVec.GetPageSize() / sizeof(int));
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(8) / sizeof(int));

        ASSERT_TRUE(grVec[2] == 3);
    }
    {
        GrowingVectorVM<int, _4GBSisePolicyTag> grVec(10, 10); // ctor with count and value
        ASSERT_TRUE(grVec.GetSize() == 10);
        ASSERT_TRUE(grVec.GetCapacity() == grVec.GetPageSize() / sizeof(int));
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(4) / sizeof(int));
    }
    {
        GrowingVectorVM<int, _4GBSisePolicyTag> grVec(20); // ctor with default count object
        ASSERT_TRUE(grVec.GetSize() == 20);
        ASSERT_TRUE(grVec.GetCapacity() == grVec.GetPageSize() / sizeof(int));
        ASSERT_TRUE(grVec.GetReserve() == DS_GB(4) / sizeof(int));
    }
}

TEST(GrowingVectorTest, VectorResizeOutsideReserve)
{
    constexpr size_t bytes = DS_KB(512);
    GrowingVectorVM<double, CustomSizePolicyTag<bytes>> vec;

    EXPECT_EQ(vec.GetReserve(), bytes / sizeof(double));
    vec.Resize(vec.GetReserve(), 1.0);

    EXPECT_THROW(
        {
            vec.PushBack(0.0);
        }, std::bad_alloc);

    EXPECT_THROW(
        {
            vec.Resize(vec.GetSize() + 1);
        }, std::bad_alloc);
}



// access outside the size should fail
// TODO object memory management checks (ctor, dtor calls)
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}