cmake_minimum_required(VERSION 3.14)
project(resizer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )

include_directories(.)
include_directories(OpenCL)
include_directories(OpenCL/kernels)
include_directories(/opt/libjpeg-turbo/include)

add_executable(resizer
        OpenCL/oclManager.cpp
        OpenCL/oclManager.h
        BatchResizer.cpp
        BatchResizer.h
        Image.cpp
        Image.h
        IO.cpp
        IO.h
        JPEGImage.cpp
        JPEGImage.h
        main.cpp
        Profiler.h
        Utils.cpp
        Utils.h)

target_link_libraries(resizer "-framework OpenCL")
target_link_libraries(resizer /opt/libjpeg-turbo/lib64/libturbojpeg.dylib)
