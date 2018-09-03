#pragma once

struct FileUtils
{
	template<typename T>
	static void writeData(std::fstream& file, const T& data);

	template<typename T>
	static void readData(std::fstream& file, T& outData);

	template<typename T>
	static void writeData(std::fstream& file, std::vector<T>& vec, const size_t bytesToWrite);

	template<typename T>
	static void readData(std::fstream& file, std::vector<T>& vec, const size_t bytesToRead);

	static void getFileSize(std::fstream& file, uint64_t& out);
	static void resetFilePosition(std::fstream& file);
	static void copyContent(std::fstream& fileIn, std::fstream& fileOut, bool copyFullFile = true);
	static bool isEoF(std::fstream& file);
};

template<typename T>
inline void FileUtils::writeData(std::fstream & file, const T & data)
{
	file.write(reinterpret_cast<const char*>(&data), sizeof(T));
}

template<typename T>
inline void FileUtils::readData(std::fstream & file, T & outData)
{
	file.read(reinterpret_cast<char*>(&outData), sizeof(T));
}

template<typename T>
inline void FileUtils::writeData(std::fstream & file, std::vector<T>& vec, const size_t bytesToWrite)
{
	file.write(reinterpret_cast<char*>(&vec[0]), bytesToWrite);
}

template<typename T>
inline void FileUtils::readData(std::fstream & file, std::vector<T>& vec, const size_t bytesToRead)
{
	file.read(reinterpret_cast<char*>(&vec[0]), bytesToRead);
}
