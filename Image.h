//
// Created by Vaclav Samec on 4/21/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

class Image
{
public:

    Image();
    explicit Image(const std::vector<unsigned char>& data);
    virtual ~Image();

    virtual void clone(const Image& copy);

    virtual bool load(const std::string& fileName);
    virtual bool save(const std::string& fileName, int compressionRatio);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return 4; }

    const std::vector<unsigned char>& getData() const { return *m_data; }
    void setData(unsigned char* data, unsigned int size, int width, int height);

    bool resize();

protected:

    bool loadFile(const std::string& fileName);
    bool saveFile(const std::string& fileName);

    virtual bool decode() = 0;
    virtual bool encode(int quality) = 0;

    void swapBuffers();

    std::vector<unsigned char>* m_data;
    int                         m_width{};
    int                         m_height{};
    int                         m_size;
    bool                        m_encoded{};

    static std::vector<unsigned char>* m_buffer;
};
