#include "IO.h"
#include <string>
#include <vector>
#include <cassert>
#if defined(__APPLE__) || defined(__MACOSX)
#include <fstream>
#elif defined(WIN32)
#include <windows.h>
#endif

bool IO::readBinaryFile(const std::string& fileName, std::vector<unsigned char>& data)
{
#if defined(WIN32)
	DWORD dwBytesRead = 0;
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned long size = GetFileSize(hFile, NULL);
	data.resize(size);
	return ReadFile(hFile, (char*)(&(data)[0]), size, &dwBytesRead, NULL);
#elif defined(__APPLE__) || defined(__MACOSX)

    FILE* file = std::fopen(fileName.c_str(), "rb");

    if (file)
    {
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        rewind(file);
        data.resize(size);

        fread((char*)(&(data)[0]), 1, size, file);
        fclose(file);
        return true;
    }

	return false;
#endif
}

bool IO::writeBinaryFile(const std::string& fileName, const std::vector<unsigned char>& data, unsigned size)
{
    unsigned long length (data.size());

    if (size > 0)
    {
        length = size;
    }
#if defined(WIN32)
	DWORD dwBytesWritten = 0;
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	return WriteFile(hFile, (char*)(&(data)[0]), length, &dwBytesWritten, NULL);

#elif defined(__APPLE__) || defined(__MACOSX)

    FILE* file = std::fopen(fileName.c_str(), "wb");

    if (file)
    {
        fwrite((char*)(&(data)[0]), 1, length, file);
        fclose(file);
        return true;
    }

	return false;
#endif
}
