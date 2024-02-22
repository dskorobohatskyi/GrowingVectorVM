#include "GrowingVectorVM.h"
#include <gtest/gtest.h>
#include "VectorsAdapter.h"


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


TYPED_TEST(VectorTest, VectorMoveCtor)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false); // TODO
}

TYPED_TEST(VectorTest, VectorMoveAssignment)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorSwaping)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorCapacityCheck)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorQuickAccessGetters)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorIteratorsCheck)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorReserveCheck)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorReservePolicyCheck)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorPushBack)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
}

TYPED_TEST(VectorTest, VectorPopBack)
{
    VectorAdapter<int, this->useStd> adapter;

    ASSERT_TRUE(false);  // TODO
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

// access outside the size should fail
// TODO object memory management checks (ctor, dtor calls)
int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}