//
// Created by Vaclav Samec on 5/7/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

class Resizer
{
public:

    /**
     *
     * @param inputDir input directory of images to be resized
     * @param outputDir output directory where the images will be resized
     * @param ratio ratio of resizing, eg. 0.5 will result in 50% smaller image
     * @param quality quality of output JPEG <1, 100>
     * @param options resizing algorithm
     */
    static void resizeCL(const std::string& inputDir,
                         const std::string& outputDir,
                         float ratio,
                         int quality,
                         const std::string& options,
                         std::unordered_map<std::string, float>& profiler);

    static void resizeCV(const std::string& inputDir,
                       const std::string& outputDir,
                       float ratio,
                       int quality,
                       const std::string& options,
                       std::unordered_map<std::string, float>& profiler);
};
