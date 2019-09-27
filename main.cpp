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
#include "Utils.h"

int main(int argc, const char * argv[])
{
    if (argc == 1)
    {
        std::cout << "Example usage: resizer -output <output_dir> -input <input_dir> -ratio 0.8 -quality 90" << std::endl;
        return 1;
    }

    std::vector<std::string> files;
    std::string outputDir;
    auto ratio(1.0f);
    auto quality(100);

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
            Utils::getFilesDir(argv[i+1], files, ".jpg");
            i++;
            continue;
        }
    }

    if (!files.empty())
    {
        BatchResizer::resize(files, outputDir, ratio, quality);
    }

    return 0;
}
