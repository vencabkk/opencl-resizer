//
// Created by Vaclav Samec on 5/7/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

class BatchResizer
{
public:

    static void resize(const std::vector<std::string>& files, std::string outputDir, float ratio, int quality);
};
