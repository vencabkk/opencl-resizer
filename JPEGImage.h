//
// Created by Vaclav Samec on 4/21/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

#include "Image.h"
#include <string>

class JPEGImage : public Image
{
public:

protected:

    bool decode() override;
    bool encode(int quality) override;
};
