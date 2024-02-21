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


int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}