//
// Created by Vaclav Samec on 5/7/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#include "Resizer.h"
#include "Profiler.h"
#include "OpenCL/oclManager.h"
#include "JPEGImage.h"
#include "Utils.h"
#include "OpenCL/kernels/resize_kernel.h"

void Resizer::resize(const std::string& inputDir, const std::string& outputDir, float ratio, int quality, const std::string& algo)
{
    Profiler::start("resizeCL");

    std::vector<std::string> files;
    auto outDir = outputDir;
    std::string samplingAlgo;
    Utils::getFilesDir(inputDir, files, ".jpg");

    // check for input directory
    if (files.empty())
    {
        std::cerr << "No input images found in: " << inputDir << std::endl;
        return;
    }

    // check for output directory
    if (outDir.empty())
    {
        outDir = files[0].substr(0, files[0].find_last_of("/\\") + 1) + "output";
    }

    if (!Utils::isDirectory(outDir))
    {
        Utils::createDirectory(outDir);
    }

    // check for re-sampling algorithm
    for (const auto& e : entries)
    {
        if (e.find(algo) != std::string::npos)
        {
            samplingAlgo = e;
            break;
        }
    }

    if (samplingAlgo.empty())
    {
        std::cerr << "Invalid resampling method, available methods: [nn, linear, bicubic]" << std::endl;
        return;
    }

    // init OpenCL
    oclManager ocl;

    if (!ocl.createContext(oclManager::GPU))
    {
        return;
    }

    // compile program
    if (!ocl.addKernelProgram(resizeKernel))
    {
        std::cerr << "Error building kernel." << std::endl;
        return;
    }

    JPEGImage imageIn;
    JPEGImage imageOut;

    for (int i=0; i<files.size(); i++)
    {
        if (imageIn.load(files[i]))
        {
            ocl.resizeImage(imageIn, imageOut, ratio, samplingAlgo);

            std::string outFile = outDir + files[i].substr(files[0].find_last_of("/\\"));

            imageOut.save(outFile, quality);
        }
    }

    Profiler::stop("resizeCL");
}


#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

void Resizer::resizeCV(const std::string &inputDir, const std::string &outputDir, float ratio, int quality,
                       const std::string &options)
{
    Profiler::start("resizeCV");

    using namespace cv;

    std::vector<std::string> files;
    auto outDir = outputDir;
    std::string samplingAlgo;
    Utils::getFilesDir(inputDir, files, ".jpg");

    // check for input directory
    if (files.empty())
    {
        std::cerr << "No input images found in: " << inputDir << std::endl;
        return;
    }

    // check for output directory
    if (outDir.empty())
    {
        outDir = files[0].substr(0, files[0].find_last_of("/\\") + 1) + "output";
    }

    if (!Utils::isDirectory(outDir))
    {
        Utils::createDirectory(outDir);
    }

    Mat imageIn;
    Mat imageOut;
    std::vector<int> compression_params;
    compression_params.push_back(IMWRITE_JPEG_QUALITY);
    compression_params.push_back(quality);
//    compression_params.push_back(IMWRITE_JPEG_PROGRESSIVE);
//    compression_params.push_back(1);
//    compression_params.push_back(IMWRITE_JPEG_OPTIMIZE);
//    compression_params.push_back(1);
//
    for (int i=0; i<files.size(); i++)
    {
        imageIn = imread(files[i]);

        if(!imageIn.data)
        {
            std::cout <<  "Could not open or find the image" << std::endl ;
            return;
        }

        cv::resize(imageIn, imageOut, cv::Size(imageIn.cols * ratio, imageIn.rows * ratio), 0, 0, INTER_CUBIC);

        std::string outFile = outDir + files[i].substr(files[0].find_last_of("/\\"));

        imwrite(outFile, imageOut, compression_params);
    }

    Profiler::stop("resizeCV");
}
