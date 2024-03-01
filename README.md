Here is a pet-project covering several aspects of general programming stuff:

- Work with virtual memory (on Windows at least)
- Own container implementation (which is not ideal obviously, but including iterators, methods, memory management)
- Practice with cmake generation
- Quite not bad test covering where I implemented adapter over std::vector and my custom one (checking expected behavior is easier with this approach)
- Compatibility with STL as far as I test it based on iterators mostly
- Using C++20 features (for ex. operator <=>), but that's also a trade-off for previous C++ standards.
- Native visualization of custom type

--------------

### Usage ###

GrowingVectorVM - vector with  dynamic size, which can grow up to ReservePolicy size which you provide to it. It uses Virtual Memory and should be able to work without iterator invalidation on vector extending(PushBack/Resize). Erasing methods sure thing will invalidate iterators on the right from removing iterator. On the left - should keep their values.
But here is a huge limitation: there is no extending mechanism after reaching reserve size.
P.S. it's not expected to use this container  in production, just for educational purposes and fun!

-------------- 

#### Build & Run ####

You can use dev scripts I prepared, generate solution for Visual Studio for ex. you can easily done with regenerate_with_cmake.bat. Use corresponding script with tests in the name to have possibility to check container in the action.
Open generated solution, choose Examples or test_main project and launch it.

--------------

#### Improvements and Known issues #### 

1. Add Linux support (likely nmap should be used. PlatformHelper's methods should be implemented for required platform)
2. Output directories are not well-organized
3. Some validations are missed in iterators logic.
4. Back-compatibility support for C++17 at least will be huge improvement, but I don't want to introduce boilerplate code to reach that.
5. Large pages support is missing.
6. Behavior of the container when unaligned structure is used should be checked and corrected probably.
