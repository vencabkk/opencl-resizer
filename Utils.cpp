//
// Created by Vaclav Samec on 5/7/15 AD.
// Copyright (c) 2015 Venca. All rights reserved.
//

#include "Utils.h"
#include "Profiler.h"
#include <sys/types.h>
#include <sys/stat.h>

#if defined(WIN32)
#include <direct.h>
#include <Windows.h>
#elif defined(__APPLE__) || defined(__MACOSX)
#include <dirent.h>
#include <mach-o/dyld.h>
#endif

std::unordered_map<std::string, clock_t> Profiler::m_clocks;

bool Utils::isDirectory(const std::string& path)
{
    struct stat info{};

    if (stat(path.c_str(), &info) != 0)
    {
        return false;
    }

    return (info.st_mode & S_IFDIR) != 0;
}

bool Utils::createDirectory(const std::string& path)
{
#if defined(__APPLE__) || defined(__MACOSX)
    return mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0;
#else
	return _mkdir(path.c_str()) == 0;
#endif
}

void Utils::getFilesDir(const std::string& path, std::vector<std::string>& files, std::string mask)
{
    files.reserve(64);

#if defined(__APPLE__) || defined(__MACOSX)
    DIR* dirFile = opendir( path.c_str());
    if ( dirFile )
    {
        struct dirent* hFile;
        errno = 0;
        while (( hFile = readdir( dirFile )) != nullptr )
        {
            if ( !strcmp( hFile->d_name, "."  )) continue;
            if ( !strcmp( hFile->d_name, ".." )) continue;

            if ( hFile->d_name[0] == '.' ) continue;

            if (strstr( hFile->d_name, mask.c_str()))
            {
                files.push_back(path + "/" + hFile->d_name);
            }
        }
        closedir( dirFile );
    }
#elif defined(WIN32)
	HANDLE dir;
	WIN32_FIND_DATA file_data;

	if ((dir = FindFirstFile((path + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
		return; /* No files found */

	do {
		const std::string file_name = file_data.cFileName;
		const std::string full_file_name = path + "/" + file_name;
		const bool is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

		if (file_name[0] == '.')
			continue;

		if (is_directory)
			continue;

		if (file_name.rfind(mask) != std::string::npos)
		{
			files.push_back(full_file_name);
		}

	} while (FindNextFile(dir, &file_data));

	FindClose(dir);
#endif
}

std::string Utils::getExeDir()
{
#if defined(WIN32)
	HMODULE hModule = GetModuleHandleW(NULL);
	char path[MAX_PATH] = {0};
	GetModuleFileName(hModule, path, MAX_PATH);
#elif defined(__APPLE__) || defined(__MACOSX)
	char path[1024] = {0};
	uint32_t size = sizeof(path);
	_NSGetExecutablePath(path, &size);
#endif

	std::string filename = path;

	if (!filename.empty())
	{
		const size_t last_slash_idx = filename.find_last_of("/\\");
		if (std::string::npos != last_slash_idx)
		{
			return filename.substr(0, last_slash_idx);
		}
	}

	return std::string();
}
