cmake_minimum_required(VERSION 3.14)
project(resizer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )

include_directories(.)
include_directories(OpenCL)
include_directories(OpenCL/kernels)

if (APPLE)
    include_directories(/opt/libjpeg-turbo/include)
    include_directories(/usr/local/Frameworks/opencv2.framework)
endif (APPLE)

#windows specific
if (WIN32)
    include_directories("c:/libjpeg-turbo-gcc/include")
    include_directories("c:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/include")
    include_directories("c:/opencv/include/")
endif (WIN32)

add_executable(resizer
        OpenCL/oclManager.cpp
        OpenCL/oclManager.h
        Resizer.cpp
        Resizer.h
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

if (APPLE)
    target_link_libraries(resizer "-framework OpenCL")
    target_link_libraries(resizer /opt/libjpeg-turbo/lib64/libturbojpeg.dylib)
    target_link_libraries(resizer /usr/local/Frameworks/opencv2.framework)
endif (APPLE)

#windows specific
if (WIN32)
    target_link_libraries(resizer "c:/libjpeg-turbo-gcc/lib/libturbojpeg.a")
    target_link_libraries(resizer "c:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v10.1/lib/Win32/OpenCL.lib")
    target_link_libraries(resizer "c:/opencv/x64/vc14/lib/opencv_world411.lib")
endif (WIN32)
