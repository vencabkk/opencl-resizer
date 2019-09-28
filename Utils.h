//
// Created by Vaclav Samec on 5/7/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

#include <algorithm>
#include <string>
#include <vector>

class Utils
{
public:

    template<typename T>
    static T minimum(T a, T b)
    {
        #ifdef min
			return min(a, b);
        #else
            return std::min(a, b);
        #endif
    }

    template<typename T>
    static T maximum(T a, T b)
    {
        #ifdef max
			return max(a, b);
        #else
            return std::max(a, b);
        #endif
    }

    static bool isDirectory(const std::string& path);

    static bool createDirectory(const std::string& path);

    static void getFilesDir(const std::string& path, std::vector<std::string>& files, const std::string& mask);

	static std::string getExeDir();
};
