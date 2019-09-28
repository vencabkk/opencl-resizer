//
// Created by Vaclav Samec on 4/28/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//
#include "oclManager.h"
#include <iostream>
#include <set>
#include <cassert>
#include "../Utils.h"
#include "../Image.h"

const std::string oclManager::preferredDeviceVendors[] =
{
    "NVIDIA",
    "AMD",
};

const cl::Platform& oclManager::getPlatform(DeviceType type) const
{
    // Get available platforms
    VECTOR_CLASS<cl::Platform> platforms;
    VECTOR_CLASS<cl::Device> devices;
    cl::Platform::get(&platforms);

    if(platforms.empty())
    {
        throw cl::Error(1, "No OpenCL platforms were found");
    }

    int platformID = -1;

    for(auto i = 0; i < platforms.size(); i++)
    {
        try
        {
            platforms[i].getDevices(type, &devices);
            platformID = i;
            break;
        }
        catch(cl::Error& e)
        {
            continue;
        }
    }

    if(platformID == -1)
    {
        throw cl::Error(1, "No compatible OpenCL platform found");
    }

    return platforms[platformID];
}

const cl::Device& oclManager::getDevice(const cl::Context& context) const
{
    // Select device and create a command queue for it
    VECTOR_CLASS<cl::Device> devices = context.getInfo<CL_CONTEXT_DEVICES>();

    for (const auto& device : devices)
    {
        for (const auto& deviceVendor : preferredDeviceVendors)
        {
            if (device.getInfo<CL_DEVICE_VENDOR>().find(deviceVendor) != std::string::npos)
            {
                return device;
            }
        }
    }

    return devices[0];
}

bool oclManager::createContext(DeviceType type)
{
    try
    {
        auto platform = getPlatform(type);

        // Use the preferred platform and create a context
        cl_context_properties cps[] = {
                CL_CONTEXT_PLATFORM,
                (cl_context_properties)(platform)(),
                0
        };

        m_context = cl::Context(type, cps);

        auto device = getDevice(m_context);

        m_queue = cl::CommandQueue(m_context, device);

        std::cout << "Using platform: " << platform.getInfo<CL_PLATFORM_VENDOR>() << std::endl;
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Using version: " << device.getInfo<CL_DEVICE_VERSION>() << std::endl;

        return true;
    }
    catch(cl::Error& error)
    {
        std::cerr << "Failed to create an OpenCL context!" << std::endl << error.what() << std::endl;
        return false;
    }
}

bool oclManager::addKernelProgram(const std::string &kernel)
{
    cl::Program::Sources source(1, std::make_pair(kernel.c_str(), kernel.length()+1));

    // Make program of the source code in the m_context
    m_program = cl::Program(m_context, source);

    VECTOR_CLASS<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();

    // Build program for these specific devices
    try
    {
        m_program.build(devices);
    }
    catch(cl::Error& error)
    {
        if(error.err() == CL_BUILD_PROGRAM_FAILURE)
        {
            std::cout << "Build log:" << std::endl << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
        }
        return false;
    }

    return true;
}

void oclManager::resizeImage(const Image& in, Image& out, float ratio, const std::string& programEntry)
{
    assert(ratio > 0);

    auto imageFormat = getImageFormat(in);

    // Create an OpenCL Image / texture and transfer data to the device
    cl::Image2D clImageIn = cl::Image2D(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, in.getWidth(), in.getHeight(), 0, (void*)in.getData().data());

    struct CLImage
    {
        CLImage(const Image& img, float ratio) : Width(img.getWidth() * ratio), Height(img.getHeight() * ratio) {}
        unsigned Width;    ///< Width of the image, in pixels
        unsigned Height;   ///< Height of the image, in pixels
    };

    CLImage sImageIn(in, 1.0f);
    CLImage sImageOut(in, ratio);

    // Create a buffer for the result
    cl::Image2D clImageOut = cl::Image2D(m_context, CL_MEM_WRITE_ONLY, imageFormat, sImageOut.Width, sImageOut.Height, 0, nullptr);

    // Run kernel
    cl::Kernel kernel = cl::Kernel(m_program, programEntry.c_str());
    kernel.setArg(0, clImageIn);
    kernel.setArg(1, clImageOut);
    kernel.setArg(2, sImageIn);
    kernel.setArg(3, sImageOut);
    kernel.setArg(4, ratio);
    kernel.setArg(5, ratio);

    m_queue.enqueueNDRangeKernel(
            kernel,
            cl::NullRange,
            cl::NDRange(Utils::maximum(sImageIn.Width, sImageOut.Width), Utils::maximum(sImageIn.Width, sImageOut.Height)),
            cl::NullRange
    );

    cl::size_t<3> origin;
    cl::size_t<3> region;
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    region[0] = sImageOut.Width;
    region[1] = sImageOut.Height;
    region[2] = 1;

    const unsigned int size (sImageOut.Width*sImageOut.Height*in.getChannels());
    out.setData(new unsigned char[size], size, sImageOut.Width, sImageOut.Height);
    m_queue.enqueueReadImage(clImageOut, CL_TRUE, origin, region, 0, 0, (void*)out.getData().data());
}

cl::ImageFormat oclManager::getImageFormat(const Image& img) const
{
	switch (img.getChannels())
	{
		case 3:
		{
#if defined(__APPLE__) || defined(__MACOSX)
			return cl::ImageFormat(CL_RGB, CL_UNORM_INT8);
#else
			return cl::ImageFormat(CL_RGB, CL_UNSIGNED_INT8);
#endif
		}
		break;

		case 4:
		{
			return cl::ImageFormat(CL_RGBA, CL_UNORM_INT8);
		}
		break;

		default:
			assert(false);
			break;
	}

	return cl::ImageFormat();
	assert(false);
}


char* oclManager::getCLErrorString(cl_int err)
{
    switch (err) {
        case CL_SUCCESS:                          return (char *) "Success!";
        case CL_DEVICE_NOT_FOUND:                 return (char *) "Device not found.";
        case CL_DEVICE_NOT_AVAILABLE:             return (char *) "Device not available";
        case CL_COMPILER_NOT_AVAILABLE:           return (char *) "Compiler not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE:    return (char *) "Memory object allocation failure";
        case CL_OUT_OF_RESOURCES:                 return (char *) "Out of resources";
        case CL_OUT_OF_HOST_MEMORY:               return (char *) "Out of host memory";
        case CL_PROFILING_INFO_NOT_AVAILABLE:     return (char *) "Profiling information not available";
        case CL_MEM_COPY_OVERLAP:                 return (char *) "Memory copy overlap";
        case CL_IMAGE_FORMAT_MISMATCH:            return (char *) "Image format mismatch";
        case CL_IMAGE_FORMAT_NOT_SUPPORTED:       return (char *) "Image format not supported";
        case CL_BUILD_PROGRAM_FAILURE:            return (char *) "Program build failure";
        case CL_MAP_FAILURE:                      return (char *) "Map failure";
        case CL_INVALID_VALUE:                    return (char *) "Invalid value";
        case CL_INVALID_DEVICE_TYPE:              return (char *) "Invalid device type";
        case CL_INVALID_PLATFORM:                 return (char *) "Invalid platform";
        case CL_INVALID_DEVICE:                   return (char *) "Invalid device";
        case CL_INVALID_CONTEXT:                  return (char *) "Invalid context";
        case CL_INVALID_QUEUE_PROPERTIES:         return (char *) "Invalid queue properties";
        case CL_INVALID_COMMAND_QUEUE:            return (char *) "Invalid command queue";
        case CL_INVALID_HOST_PTR:                 return (char *) "Invalid host pointer";
        case CL_INVALID_MEM_OBJECT:               return (char *) "Invalid memory object";
        case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:  return (char *) "Invalid image format descriptor";
        case CL_INVALID_IMAGE_SIZE:               return (char *) "Invalid image size";
        case CL_INVALID_SAMPLER:                  return (char *) "Invalid sampler";
        case CL_INVALID_BINARY:                   return (char *) "Invalid binary";
        case CL_INVALID_BUILD_OPTIONS:            return (char *) "Invalid build options";
        case CL_INVALID_PROGRAM:                  return (char *) "Invalid program";
        case CL_INVALID_PROGRAM_EXECUTABLE:       return (char *) "Invalid program executable";
        case CL_INVALID_KERNEL_NAME:              return (char *) "Invalid kernel name";
        case CL_INVALID_KERNEL_DEFINITION:        return (char *) "Invalid kernel definition";
        case CL_INVALID_KERNEL:                   return (char *) "Invalid kernel";
        case CL_INVALID_ARG_INDEX:                return (char *) "Invalid argument index";
        case CL_INVALID_ARG_VALUE:                return (char *) "Invalid argument value";
        case CL_INVALID_ARG_SIZE:                 return (char *) "Invalid argument size";
        case CL_INVALID_KERNEL_ARGS:              return (char *) "Invalid kernel arguments";
        case CL_INVALID_WORK_DIMENSION:           return (char *) "Invalid work dimension";
        case CL_INVALID_WORK_GROUP_SIZE:          return (char *) "Invalid work group size";
        case CL_INVALID_WORK_ITEM_SIZE:           return (char *) "Invalid work item size";
        case CL_INVALID_GLOBAL_OFFSET:            return (char *) "Invalid global offset";
        case CL_INVALID_EVENT_WAIT_LIST:          return (char *) "Invalid event wait list";
        case CL_INVALID_EVENT:                    return (char *) "Invalid event";
        case CL_INVALID_OPERATION:                return (char *) "Invalid operation";
        case CL_INVALID_GL_OBJECT:                return (char *) "Invalid OpenGL object";
        case CL_INVALID_BUFFER_SIZE:              return (char *) "Invalid buffer size";
        case CL_INVALID_MIP_LEVEL:                return (char *) "Invalid mip-map level";
        default:                                  return (char *) "Unknown";
    }
}
