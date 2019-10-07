//
//  main.cpp
//  resizer
//
//  Created by Vaclav Samec on 4/21/15 AD.
//  Copyright (c) 2015 Venca. All rights reserved.
//

#include <iostream>
#include <vector>
#include "Resizer.h"

int main(int argc, const char * argv[])
{
    if (argc == 1)
    {
        std::cout << "Example usage: resizer -output <output_dir> -input <input_dir> -ratio 0.8 -quality 90 -algo bicubic" << std::endl;
        return 1;
    }

    std::string inputDir;
    std::string outputDir;
    auto ratio(1.0f);
    auto quality(100);
    auto algo("bicubic");

    for (int i=1; i<argc; i++)
    {
        std::string arg = argv[i];

        if (arg == "-output")
        {
            outputDir = argv[i+1];
            i++;
            continue;
        }

        if (arg == "-ratio")
        {
            ratio = atof(argv[i+1]);
            i++;
            continue;
        }

        if (arg == "-quality")
        {
            quality = atoi(argv[i+1]);
            i++;
            continue;
        }

        if (arg == "-input")
        {
            inputDir = argv[i+1];
            i++;
            continue;
        }

        if (arg == "-algo")
        {
            algo = argv[i+1];
            i++;
            continue;
        }
    }

    std::unordered_map<std::string, float> profilerCL;
    std::unordered_map<std::string, float> profilerCV;

    auto getRatio = [](std::unordered_map<std::string, float>& stats1,
             std::unordered_map<std::string, float>& stats2,
             const std::string& key)
    {
        auto ratio = (int)(100 * stats1[key] / stats2[key] - 100);

        if (ratio > 0)
        {
            return "-" + std::to_string(ratio) + "%";
        }

        return "+" + std::to_string(ratio * -1) + "%";
    };

//    for (auto i=0; i< 5; i++)
    {
        Resizer::resizeCV(inputDir, outputDir, ratio, quality, algo, profilerCV);
        Resizer::resizeCL(inputDir, outputDir, ratio, quality, algo, profilerCL);
    }

    auto totalCL = profilerCL["read"] + profilerCL["write"] + profilerCL["resize"];
    auto totalCV = profilerCV["read"] + profilerCV["write"] + profilerCV["resize"];

    std::cout << "Operations: " << profilerCV["op"] << std::endl;
    std::cout << "Read: " << getRatio(profilerCL, profilerCV, "read") << std::endl;
    std::cout << "Write: " << getRatio(profilerCL, profilerCV, "write") << std::endl;
    std::cout << "Resize: " << getRatio(profilerCL, profilerCV, "resize") << std::endl;
    std::cout << "TotalCV: " << totalCV / profilerCV["op"] << std::endl;
    std::cout << "TotalCL: " << totalCL  / profilerCL["op"] << std::endl;

    return 0;
}
