//
// Created by Vaclav Samec on 4/28/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

#include <ctime>
#include <iostream>
#include <unordered_map>
#include <string>

class Profiler
{
public:

    static void start(const std::string& name)
    {
        m_clocks[name] = clock();
    }

    static void stop(const std::string& name)
    {
        const auto& clock_curr = m_clocks.find(name);
        auto ms = float(clock() - clock_curr->second) / CLOCKS_PER_SEC * 1000.0f;
        std::cout << "Elapsed time " << name << ": " << ms << "[ms]" << std::endl;
        m_clocks.erase(clock_curr);
    }

private:

    static std::unordered_map<std::string, clock_t> m_clocks;
};
