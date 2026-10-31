cmake_minimum_required(VERSION 3.20)
project(OpenANC LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ANC_CORE STATIC
    src/DeviceManager.cpp
    src/DeviceManager.h
    src/AudioEngine.cpp
    src/AudioEngine.h
    src/FxLMS.cpp
    src/FxLMS.h
    src/HighPassFilter.cpp
    src/HighPassFilter.h
    src/Limiter.h
    src/AcousticCalibrator.h
)

target_include_directories(ANC_CORE PUBLIC src)

target_link_libraries(ANC_CORE PUBLIC
    ole32
    oleaut32
    uuid
    avrt
    comctl32
)

add_executable(OpenANCApp WIN32
    src/main.cpp
    src/Win32App.cpp
    src/Win32App.h
)

target_link_libraries(OpenANCApp PRIVATE ANC_CORE comctl32)
