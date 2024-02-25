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
            emptyAdapter.Front();
            emptyAdapter.Back();
            }, ".*");
#endif
    }
    else
    {
        EXPECT_ANY_THROW({
            emptyAdapter.Front();
            emptyAdapter.Back();
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

    constexpr size_t bytes = DS_GB(2);
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
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorEmplaceWithCapacityChange)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorInserts)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorInsertWithCapacityChange)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorErase)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorClear)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorResizeWithinCapacity)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorResizeOutsideCapacity)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorResizeOutsideReserve)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorSTLAlgoCompatibility)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}


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



// access outside the size should fail
// TODO object memory management checks (ctor, dtor calls)
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}