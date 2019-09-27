//
// Created by Vaclav Samec on 5/7/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#include "BatchResizer.h"
#include "Profiler.h"
#include "OpenCL/oclManager.h"
#include "JPEGImage.h"
#include "Utils.h"

void BatchResizer::resize(const std::vector<std::string>& files, std::string outputDir, float ratio, int quality)
{
    Profiler::start("BatchResizer::resize");

    // init OpenCL
    oclManager ocl;

    if (!ocl.createContext(oclManager::GPU))
    {
        return;
    }

    // compile program
    ocl.buildProgramFromSource(ocl.getKernelDir() + "/OpenCL/kernels/resize.cl", "");

    if (outputDir.empty())
    {
        outputDir = files[0].substr(0, files[0].find_last_of("/\\") + 1) + "output";
    }

    if (!Utils::isDirectory(outputDir))
    {
        Utils::createDirectory(outputDir);
    }

    JPEGImage imageIn;
    JPEGImage imageOut;

    for (int i=0; i<files.size(); i++)
    {
        if (imageIn.load(files[i]))
        {
            ocl.resizeImage(imageIn, imageOut, ratio);

            std::string outFile = outputDir + files[i].substr(files[0].find_last_of("/\\"));

            imageOut.save(outFile, quality);
        }
    }

    Profiler::stop("BatchResizer::resize");
}
