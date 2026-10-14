cmake_minimum_required(VERSION 3.22)
project(ess-eleven-hundred VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(S1100_BUILD_APP "Build the JUCE standalone app" ON)

enable_testing()

# The emulator core has no UI or OS dependencies, so it builds on its own.
add_subdirectory(emu-core)

if(S1100_BUILD_APP)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
    add_subdirectory(app)
endif()
