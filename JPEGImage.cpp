//
// Created by Vaclav Samec on 4/21/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#include "JPEGImage.h"
#include "turbojpeg.h"

bool JPEGImage::decode()
{
    int jpegSubsamp, ret;

    tjhandle jpegDecompressor = tjInitDecompress();
    ret = tjDecompressHeader2(jpegDecompressor, m_data->data(), m_data->size(), &m_width, &m_height, &jpegSubsamp);

    if (!ret)
    {
        auto size = static_cast<unsigned long>(m_width*m_height*getChannels());
        m_buffer->resize(size);

		int channels = getChannels() == 3 ? TJPF_RGB : TJPF_RGBA;
		ret = tjDecompress2(jpegDecompressor, m_data->data(), m_data->size(), m_buffer->data(), m_width, 0/*pitch*/, m_height, channels, TJFLAG_FASTDCT | TJFLAG_NOREALLOC);
        tjDestroy(jpegDecompressor);

        if (!ret)
        {
            swapBuffers();
            m_encoded = false;
        }
    }

    return ret == 0;
}

bool JPEGImage::encode(int quality)
{
    int ret;
    auto size = static_cast<unsigned long>(m_width*m_height*getChannels());
    m_buffer->reserve(size);

    unsigned char* unCompressedData = m_buffer->data();

    tjhandle jpegCompressor = tjInitCompress();

	int channels = getChannels() == 3 ? TJPF_RGB : TJPF_RGBA;
    ret = tjCompress2(jpegCompressor, m_data->data(), m_width, 0, m_height, channels,
            &unCompressedData, &size, TJSAMP_444, quality,
            TJFLAG_ACCURATEDCT | TJFLAG_NOREALLOC);

    if (!ret)
    {
        swapBuffers();
        m_encoded = true;
        m_size = static_cast<int>(size);
    }

    tjDestroy(jpegCompressor);

    return ret == 0;
}
