// GrowingVectorVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "GrowingVectorVM.h"
#include <iostream>
#include <vector>
#include <assert.h>


struct alignas(256) MyHugeStruct
{

};
static_assert(alignof(MyHugeStruct) == 256);
static_assert(sizeof(MyHugeStruct) == 256);


struct A
{

};
static_assert(sizeof(A) == 1);
static_assert(alignof(A) == 1);


struct B
{
    B(int value)
        : a(value)
    {

    }
    int a;
    char b;
};

static_assert(sizeof(B) == 8);
static_assert(alignof(B) == 4);

using namespace ds; // not good practice in general, but that's examples

int main()
{
    GrowingVectorVM<B> bar;
    bar.PushBack({ 3 });
    bar.EmplaceBack(4);
    bar.Resize(4, B{1}); // OK
    //bar.Resize(5); // fails

    for (auto item : bar)
    {
        std::cout << item.a << std::endl;
    }

    //(*bar.CBegin()).a = 2; // fails
    (*bar.Begin()).a = 2; // ok
    (bar.Begin()[0]).a = 2; // ok

    std::vector<B> bad;
    auto bad_begin = bad.begin();
    auto bad_end = bad.end();
    bad.emplace(bad.begin(), 3);
    //assert(bad_begin != bad_end);
    //bad.insert(bad.end() + 1, 3, { 4 }); // fails
    
    //bad.resize(3); // compilation error
    std::vector<A> aad;
    aad.resize(3); // OK
    //bad.cbegin()->a = 2; // fail

    bar.Clear();
    for (const auto t : { 1, 2, 3, 4 })
    {
        bar.Insert(bar.Begin(), { t });
    }

    //bar.Insert(bar.End() + 1, { 10 });// fails

    constexpr size_t halfRAMHackyValue = DS_GB(16);
    GrowingVectorVM<double, CustomSizePolicyTag<halfRAMHackyValue>> vectorInf;

    try
    {
        GrowingVectorVM<double, MyHugeStruct /*as ReservePolicy*/> incorrectPolicyVector;
    }
    catch (const std::logic_error&)
    {
        // expected
    }

    const size_t maxAmount = vectorInf.GetCapacity();
    std::cout << "Capacity: " << maxAmount << "\n";
    std::cout << "Reserve: " << vectorInf.GetReserve() << "\n";

    try
    {
        vectorInf.PushBack(1.23f);
    }
    catch (const std::bad_array_new_length&)
    {
        std::cout << "Expected exception on reserve overflow\n";
    }

    const int newCapacity = 10;
    std::vector<int> v;
    assert(v.capacity() == 0);
    v.reserve(newCapacity);
    assert(v.capacity() == newCapacity);

    for (int i = 0; i < newCapacity; i++)
    {
        v.push_back(i);
        assert(v.capacity() == newCapacity);
    }

    GrowingVectorVM<int> mv;
    assert(mv.GetCapacity() == 0);
    //auto _ = mv.Back(); // OK: failed
    mv.Reserve(newCapacity);
    //assert(mv.GetCapacity() == 4096 * 1 / sizeof(int)); // failed

    //for (int i = 0; i < newCapacity; i++)
    //{
    //    mv.PushBack(i);
    //    assert(mv.GetCapacity() == newCapacity);
    //}

    std::vector<MyHugeStruct> testVector;
    std::cout << "Max size of vector - " << testVector.max_size() << std::endl;
    size_t counter = 0;
    auto emitOnEveryNthIteration = [&counter](const size_t limit)
        {
            counter++;
            if (counter == limit)
            {
                counter = 0;
                return true;
            }
            return false;
        };

    //while (true)
    //{
    //    if (emitOnEveryNthIteration(10000))
    //    {
    //        std::cout << "Capacity=" << testVector.capacity() << ", Size=" << testVector.size() << ", Alloc=" << testVector.capacity() * sizeof(MyHugeStruct) << "\n";
    //    }
    //    testVector.push_back({});
    //}
    // LIMIT size and capacity 136'216'567 = 34,871,441,152 bytes

    std::vector< MyHugeStruct> hey;
    //hey.reserve(300'000'000);
    GrowingVectorVM<MyHugeStruct> testMyVector;
    //bool success = testMyVector.Reserve(136'216'567); // OK  - 34,871,443,456 bytes allocated
    testMyVector.Reserve(100'000'000); // OK  12'500'000 pages = 51,200,000,000 bytes
    {
        for (int i = 0; i < testMyVector.GetCapacity(); i++)
        {
            if (emitOnEveryNthIteration(100000))
            {
                std::cout << "Capacity=" << testMyVector.GetCapacity() << ", Size=" << testMyVector.GetSize() << ", Alloc=" << testMyVector.GetCapacity() * sizeof(MyHugeStruct) << "\n";
            }
            testMyVector.PushBack({});
        }
        testMyVector.Clear();
    }

    return 0;


}
