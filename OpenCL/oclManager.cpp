//
// Created by Vaclav Samec on 4/28/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//
#include <sys/stat.h>
#include <ctime>

#include "oclManager.h"
#include <iostream>
#include <fstream>
#include <set>
#include <cassert>
#include "../Utils.h"
#include "../Image.h"

const std::string oclManager::preferredDeviceVendors[] =
{
    "NVIDIA",
    "AMD",
};

std::string oclManager::getKernelDir()
{
	return Utils::getExeDir() + "/../../resizer";
}

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
        try {
            platforms[i].getDevices(type, &devices);
            platformID = i;
            break;
        } catch(cl::Error& e) {
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

        std::cout << "Using platform vendor: " << platform.getInfo<CL_PLATFORM_VENDOR>() << std::endl;
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_VERSION>() << std::endl;

        return true;
    }
    catch(cl::Error& error)
    {
        std::cerr << "Failed to create an OpenCL context!" << std::endl << error.what() << std::endl;
        return false;
    }
}

void oclManager::writeBinary(const std::string& filename, const std::string& buildOptions)
{
    // Build program from source file and store the binary file
    buildProgramFromSource(filename, buildOptions);

    VECTOR_CLASS<std::size_t> binarySizes;
    binarySizes = m_program.getInfo<CL_PROGRAM_BINARY_SIZES>();

    VECTOR_CLASS<char *> binaries;
    binaries = m_program.getInfo<CL_PROGRAM_BINARIES>();

    std::string binaryFilename = filename + ".bin";
    FILE * file = fopen(binaryFilename.c_str(), "wb");
    if(!file)
        printf("could not write to file\n");
    fwrite(binaries[0], sizeof(char), (int)binarySizes[0], file);
    fclose(file);

    // Write cache file
    std::string cacheFilename = filename + ".cache";
    FILE * cacheFile = fopen(cacheFilename.c_str(), "w");
    std::string timeStr;
#ifdef WIN32
    #else
    struct stat attrib; // create a file attribute structure
    stat(filename.c_str(), &attrib);
    timeStr = ctime(&(attrib.st_mtime));
#endif
    VECTOR_CLASS<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();
    timeStr += "-" + devices[0].getInfo<CL_DEVICE_NAME>();
    fwrite(timeStr.c_str(), sizeof(char), timeStr.size(), cacheFile);
    fclose(cacheFile);
}

bool oclManager::readBinary(const std::string& filename)
{
    std::ifstream sourceFile(filename.c_str(), std::ios_base::binary | std::ios_base::in);

	if (sourceFile.is_open())
	{
		std::string sourceCode(
			std::istreambuf_iterator<char>(sourceFile),
			(std::istreambuf_iterator<char>()));
		cl::Program::Binaries binary(1, std::make_pair(sourceCode.c_str(), sourceCode.length()));

		VECTOR_CLASS<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();
		if (devices.size() > 1) {
			// Currently only support compiling for one device
			cl::Device device = devices[0];
			devices.clear();
			devices.push_back(device);
		}

		m_program = cl::Program(m_context, devices, binary);

		// Build program for these specific devices
		m_program.build(devices);

		return true;
	}

	return false;
}

bool oclManager::buildProgramFromBinary(const std::string& filename, const std::string& buildOptions)
{
    std::string binaryFilename = filename + ".bin";

    // Check if a binary file exists
    std::ifstream binaryFile(binaryFilename.c_str(), std::ios_base::binary | std::ios_base::in);
    if(binaryFile.fail())
	{
		std::cout << "Failed to load binary file ... writing binary." << std::endl;

        writeBinary(filename, buildOptions);
    }

    return readBinary(binaryFilename);
}

void oclManager::buildProgramFromSource(const std::string& filename, const std::string& buildOptions)
{
    // Read source file
    std::ifstream sourceFile(filename.c_str());
    if(sourceFile.fail())
        throw cl::Error(1, "Failed to open OpenCL source file");
    std::string sourceCode(
            std::istreambuf_iterator<char>(sourceFile),
            (std::istreambuf_iterator<char>()));
    cl::Program::Sources source(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));

    // Make program of the source code in the m_context
    m_program = cl::Program(m_context, source);

    VECTOR_CLASS<cl::Device> devices = m_context.getInfo<CL_CONTEXT_DEVICES>();

    // Build program for these specific devices
    try{
        m_program.build(devices, buildOptions.c_str());
    } catch(cl::Error& error) {
        if(error.err() == CL_BUILD_PROGRAM_FAILURE) {
            std::cout << "Build log:" << std::endl << m_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
        }
        throw error;
    }
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

void oclManager::resizeImage(const Image& in, Image& out, float ratio)
{
    assert(ratio > 0);

    auto imageFormat = getImageFormat(in);

    // Create an OpenCL Image / texture and transfer data to the device
    cl::Image2D clImageIn = cl::Image2D(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, imageFormat, in.getWidth(), in.getHeight(), 0, (void*)in.getData().data());

    struct SImage
    {
        SImage(const Image& img, float ratio) : Width(img.getWidth()*ratio), Height(img.getHeight()*ratio) {}
        unsigned Width;    ///< Width of the image, in pixels
        unsigned Height;   ///< Height of the image, in pixels
    };

    SImage sImageIn(in, 1.0f);
    SImage sImageOut(in, ratio);

    // Create a buffer for the result
    cl::Image2D clImageOut = cl::Image2D(m_context, CL_MEM_WRITE_ONLY, imageFormat, sImageOut.Width, sImageOut.Height, 0, nullptr);

    // Run kernel
//    cl::Kernel kernel = cl::Kernel(m_program, ratio<1.0f?"resize_supersample":"resize_bicubic");
    cl::Kernel kernel = cl::Kernel(m_program, "resize_bicubic");
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
