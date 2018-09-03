#pragma once
#include "FileUtils.h"

class Sorter
{
public:
	Sorter(std::fstream& fileIn, std::fstream& fileOut);
	void setMaxRamUsage(const uint32_t ramUsage);
	uint32_t getMaxRamUsage() const;
	void fillFile(const uint32_t& numbersAmount);
	void sort();
	static bool isSorted(std::fstream& file, const uint32_t maxCheckBufferSize = 256);
private:
	void countPartsAmount();

	void sortParts();
	void sortPartsHelper();
	bool endSorting();

	void mergeParts();
	bool continueMerging();
	void copyMergedToOutFile();
	void removeAdditionalFiles(const uint32_t amountOfFiles);
	void mergePartsHelper(const uint32_t indexFirst, const uint32_t indexSecond);

	bool isReadTillEnd(std::fstream& first, std::fstream& second, std::fstream& out);
	void readTillEnd(std::fstream& in, std::fstream& out);

	std::string generateSortedFileName();
	std::string getAdditionalFileNameByIndex(const uint32_t idx);
	void waitForStartedThreads(std::list<std::thread>&);
	void printFileContent(std::fstream& file);

	void onErrorOccured();	
private:
	std::fstream& mFileIn;
	std::fstream& mFileOut;
	std::mutex mFileInMtx;

	std::mutex mDebugPrintMtx;

	std::mutex mPartsMtx;
	uint32_t mPartsAmount = 0;
	uint32_t mSortedParts = 0;
	uint32_t mWorkersAmount = 4;
	uint32_t mMaxRamUsage = 256;
	size_t mSortingBufferSize = 0;


	std::mutex mAdditionalFilesMtx;
	std::uint32_t mAdditionalFilesCounter = 0;
	std::atomic<uint32_t> mMergedFilesCounter = 0;
	int mFileExceptionMask = std::ifstream::badbit;

	std::list<std::string> mFileFailures;
	std::list<std::string> mAdditionalFilesNames;

	std::atomic<bool> mMergingAborted = false;
	std::atomic<bool> mSortingAborted = false;
};