#include "stdafx.h"
#include "FileUtils.h"


void FileUtils::getFileSize(std::fstream & file, uint64_t & out)
{
	file.seekg(0, std::ios::end);
	out = static_cast<uint64_t>(file.tellg());
	resetFilePosition(file);
}

void FileUtils::resetFilePosition(std::fstream & file)
{
	file.clear();                 // clear fail and eof bits
	file.seekg(0, std::ios::beg);
}

void FileUtils::copyContent(std::fstream & fileIn, std::fstream & fileOut, bool copyFullFile)
{
	if (copyFullFile)
		resetFilePosition(fileIn);
	while (true)
	{
		uint32_t data = 0;
		readData<uint32_t>(fileIn,data);
		if (!isEoF(fileIn)) {
			writeData<uint32_t>(fileOut, data);
		}
		else
			break;
	}
}

bool FileUtils::isEoF(std::fstream & file)
{
	bool isEoF = false;
	if (file.eof())
		isEoF = true;
	return isEoF;
}
