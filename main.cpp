//
//  main.cpp
//  resizer
//
//  Created by Vaclav Samec on 4/21/15 AD.
//  Copyright (c) 2015 Venca. All rights reserved.
//

#include <iostream>
#include <vector>
#include "BatchResizer.h"

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

    BatchResizer::resize(inputDir, outputDir, ratio, quality, algo);

    return 0;
}
