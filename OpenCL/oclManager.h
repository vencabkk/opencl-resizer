//
// Created by Vaclav Samec on 4/28/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

//#define __NO_STD_VECTOR // Use cl::vector instead of STL version
#define __CL_ENABLE_EXCEPTIONS

#if defined(__APPLE__) || defined(__MACOSX)
#include "cl.hpp"
#else
    #include <CL/cl.hpp>
#endif

#include <string>

class Image;
class oclManager
{
public:

    enum DeviceType
    {
        CPU = CL_DEVICE_TYPE_CPU,
        GPU = CL_DEVICE_TYPE_GPU
    };

    bool createContext(DeviceType type);

    void buildProgramFromSource(const std::string& filename, const std::string& buildOptions);

	bool buildProgramFromBinary(const std::string& filename, const std::string& buildOptions);

    void resizeImage(const Image& in, Image& out, float ratio);

    static std::string getKernelDir();

protected:

    const cl::Platform& getPlatform(DeviceType type) const;
    const cl::Device& getDevice(const cl::Context& context) const;

	void writeBinary(const std::string& filename, const std::string& buildOptions);

	bool readBinary(const std::string& filename);

    char *getCLErrorString(cl_int err);

	cl::ImageFormat getImageFormat(const Image&) const;

	static const std::string preferredDeviceVendors[];

    cl::Context         m_context;
    cl::CommandQueue    m_queue;
    cl::Program         m_program;
};
