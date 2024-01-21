// GrowingVectorVM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <assert.h>
#include "GrowingVectorVM.h"

#define KB(x) (x) * (size_t)1024
#define MB(x) (KB(x)) * 1024
#define GB(x) (MB(x)) * 1024

static_assert(KB(1) == 1024);
static_assert(KB(4) == 4096);
static_assert(MB(2) == 1'048'576 * 2);
static_assert(GB(4) == 4'294'967'296);




struct alignas(256) MyHugeStruct
{

};
static_assert(alignof(MyHugeStruct) == 256);
static_assert(sizeof(MyHugeStruct) == 256);

struct alignas(256) MyHugeStructWithoutAlignment
{

};

struct A
{

};
static_assert(sizeof(A) == 1);
static_assert(alignof(A) == 1);


struct B
{
    int a;
    char b;
};

static_assert(sizeof(B) == 8);
static_assert(alignof(B) == 4);


int main()
{
    constexpr size_t halfRAMHackyValue = GB(16);
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

    // TODO impl use cases
    //for (size_t i = 0; i < vectorInf.GetReserve(); i++)
    {
        //vectorInf.PushBack(i * 1.23f);
    }

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

    GrowingVectorVM<MyHugeStruct> testMyVector;
    //bool success = testMyVector.Reserve(136'216'567); // OK  - 34,871,443,456 bytes allocated
    bool success = testMyVector.Reserve(200'000'000); // OK  12'500'000 pages = 51,200,000,000 bytes

    while (true)
    {
        if (emitOnEveryNthIteration(10000))
        {
            std::cout << "Capacity=" << testMyVector.GetCapacity() << ", Size=" << testMyVector.GetSize() << ", Alloc=" << testMyVector.GetCapacity() * sizeof(MyHugeStruct) << "\n";
        }
        testMyVector.PushBack({});
    }

    return 0;


}



#include <process.h>

//typedef void(__stdcall* _tls_callback_type)(void*, unsigned long, void*);
void onExitCallback(void*, unsigned long, void*)
{
    std::cout << "goodbye";
}

// _register_thread_local_exe_atexit_callback(&onExitCallback);
