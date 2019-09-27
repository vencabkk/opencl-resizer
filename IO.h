#pragma once

#include <string>
#include <vector>

class IO
{
public:

	static bool readBinaryFile(const std::string& fileName, std::vector<unsigned char>& data);

	static bool writeBinaryFile(const std::string& fileName, const std::vector<unsigned char>& data, unsigned size=0);
};
