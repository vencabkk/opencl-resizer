//
// Created by Vaclav Samec on 4/21/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//
#include "Image.h"
#include "IO.h"

std::vector<unsigned char>* Image::m_buffer = nullptr;

Image::Image() :
	m_data(new std::vector<unsigned char>()),
	m_size(0)
{
    if (!Image::m_buffer)
    {
        Image::m_buffer = new std::vector<unsigned char>();
    }
}

Image::Image(const std::vector<unsigned char>& data) :
    m_data(new std::vector<unsigned char>()),
	m_size(0)
{
    // copy data
    *m_data = data;
}

void Image::clone(const Image& copy)
{
    m_data = new std::vector<unsigned char>(*copy.m_data);
    m_width = copy.m_width;
    m_height = copy.m_height;
    m_size = copy.m_size;
    m_encoded = copy.m_encoded;
}

Image::~Image()
{
    delete m_data;
}

bool Image::loadFile(const std::string& fileName)
{
	if (IO::readBinaryFile(fileName, *m_data))
	{
		m_size = m_data->size();
		return true;
	}

	return false;
}

bool Image::saveFile(const std::string& fileName)
{
	return IO::writeBinaryFile(fileName, *m_data, m_size);
}

void Image::swapBuffers()
{
    auto tmp = m_data;
    m_data = m_buffer;
    m_buffer = tmp;
}

bool Image::resize()
{
    return false;
}

void Image::setData(unsigned char* data, unsigned int size, int width, int height)
{
    m_data->clear();
    m_data->assign(data, data + size);

    m_size = size;
    m_width = width;
    m_height = height;

    m_encoded = false;
}

bool Image::load(const std::string& fileName)
{
    m_encoded = true;

    if (loadFile(fileName))
    {
        return decode();
    }

    return false;
}

bool Image::save(const std::string& fileName, int compressionRatio)
{
    if (!m_encoded)
    {
 		encode(compressionRatio);
    }

    return saveFile(fileName);
}
