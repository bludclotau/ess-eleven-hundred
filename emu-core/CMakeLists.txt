add_library(s1100_core STATIC
    src/machine.cpp
    src/sample.cpp
    src/voice.cpp)

target_include_directories(s1100_core PUBLIC include)
target_compile_features(s1100_core PUBLIC cxx_std_20)

if(NOT MSVC)
    target_compile_options(s1100_core PRIVATE -Wall -Wextra)
endif()

add_executable(s1100_smoke tests/smoke.cpp)
target_link_libraries(s1100_smoke PRIVATE s1100_core)
add_test(NAME smoke COMMAND s1100_smoke)
