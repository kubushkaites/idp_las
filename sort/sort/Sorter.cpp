#include "stdafx.h"
#include "Sorter.h"


Sorter::Sorter(std::fstream& fileIn, std::fstream& fileOut) :
	mFileIn(fileIn),
	mFileOut(fileOut)
{

}


void Sorter::fillFile(const uint32_t& numbersAmount)
{
	uint32_t counter = 0;
	std::mt19937 rng;
	std::uniform_int_distribution<std::mt19937::result_type> dist(1, 10765432);
	while (counter < numbersAmount)
	{
		rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
		uint32_t generatedNumber = dist(rng);
		FileUtils::writeData<uint32_t>(mFileIn, generatedNumber);
		++counter;
	}

#ifdef _DEBUG
	std::cout << "--------------------------------UNSORTED-------------------------------------------------" << std::endl;
	uint64_t size = 0;
	FileUtils::resetFilePosition(mFileIn);
	printFileContent(mFileIn);
	FileUtils::getFileSize(mFileIn, size);
	std::cout << "File size : " << size << std::endl;
	std::cout << "--------------------------------------------------------------------------------------" << std::endl;
#endif
}

void Sorter::countPartsAmount()
{
	uint64_t fileSize = 0;
	FileUtils::getFileSize(mFileIn, fileSize);
	mPartsAmount = fileSize / mSortingBufferSize;
}

void Sorter::setMaxRamUsage(const uint32_t ramUsage)
{
	mMaxRamUsage = ramUsage;
}

uint32_t Sorter::getMaxRamUsage() const
{
	return mMaxRamUsage;
}

void Sorter::sort()
{
	mSortingBufferSize = mMaxRamUsage / mWorkersAmount;

	countPartsAmount();

	sortParts();
	if (mSortingAborted)
	{
		onErrorOccured();
		return;
	}

	mergeParts();
	if (mMergingAborted)
	{
		onErrorOccured();
		return;
	}

#ifdef _DEBUG
	std::cout << "--------------------------SORTED---------------------------------------------------------" << std::endl;

	printFileContent(mFileOut);

	uint64_t sizeOfOut = 0;
	FileUtils::getFileSize(mFileOut, sizeOfOut);
	std::cout << "mFileOut size : " << sizeOfOut << std::endl;
#endif
}


bool Sorter::continueMerging()
{
	bool continueMerging = true;

	if (mAdditionalFilesNames.size() == 1) {
		continueMerging = false;
	}
	return continueMerging;
}

void Sorter::copyMergedToOutFile()
{
	std::fstream fileToCopy;
	fileToCopy.exceptions(mFileExceptionMask);
	try {
		fileToCopy.open(mAdditionalFilesNames.front(), std::fstream::in | std::fstream::binary);
		FileUtils::copyContent(fileToCopy, mFileOut);
		fileToCopy.close();
		removeAdditionalFiles(mAdditionalFilesNames.size());
	}
	catch (std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
	}
}

void Sorter::removeAdditionalFiles(const uint32_t amountOfFiles)
{
	for (uint32_t i = 0; i < amountOfFiles; i++)
	{
		try 
		{
			if (std::remove(mAdditionalFilesNames.front().c_str()) != 0) {
				std::string errorMsg = "Error removing file : " + mAdditionalFilesNames.front();
				mAdditionalFilesNames.pop_front();
				throw std::runtime_error(errorMsg);
			}
		}
		catch (std::exception& ex)
		{
			std::cerr << "ERROR : " << ex.what() << std::endl;
		}
		mAdditionalFilesNames.pop_front();
	}
}

void Sorter::mergeParts()
{
	std::list<std::thread> mergeThreads;

	uint32_t filesToMergeCount = mAdditionalFilesNames.size();//this is needed because nAdditionalFileNames vector will be chagned during merging(new megred file name will be added)

	for (uint32_t i = 0; i < filesToMergeCount; i += 2)
	{
		uint32_t idxFirst = i, idxSecond = i + 1;
		if (idxSecond > filesToMergeCount - 1)
			break;
		mergeThreads.push_back(std::thread(&Sorter::mergePartsHelper, this, idxFirst, idxSecond));
	}

	waitForStartedThreads(mergeThreads);

	if (mMergingAborted)
		return;

	removeAdditionalFiles(mMergedFilesCounter);
	mMergedFilesCounter = 0;

	if (continueMerging())
		mergeParts();
	else
		copyMergedToOutFile();

}

void Sorter::mergePartsHelper(const uint32_t indexFirst, const uint32_t indexSecond)
{
	std::fstream sortedFileFirst;
	std::fstream sortedFileSecond;
	std::fstream newSortedFile;

	std::string sortedFileNameFirst = getAdditionalFileNameByIndex(indexFirst);
	std::string sortedFileNameSecond = getAdditionalFileNameByIndex(indexSecond);

	sortedFileFirst.exceptions(mFileExceptionMask);
	sortedFileSecond.exceptions(mFileExceptionMask);
	newSortedFile.exceptions(mFileExceptionMask);

	std::string newSortedFileName;
	{
		std::lock_guard<std::mutex> lock(mAdditionalFilesMtx);
		newSortedFileName = generateSortedFileName();
		mAdditionalFilesNames.push_back(newSortedFileName);
	}

	try {
		sortedFileFirst.open(sortedFileNameFirst, std::fstream::in | std::fstream::binary);//vector was chosen to simplify indexing
		sortedFileSecond.open(sortedFileNameSecond, std::fstream::in | std::fstream::binary);
		newSortedFile.open(newSortedFileName, std::fstream::in | std::fstream::app | std::fstream::binary);

		if (!sortedFileFirst.is_open() || !sortedFileSecond.is_open() || !newSortedFile.is_open())
			throw std::runtime_error("ERROR : Error on file opening");

		while (!mMergingAborted)
		{
			uint32_t firstNumber = 0, secondNumber = 0;
			uint64_t lastPosFirst = sortedFileFirst.tellg(), lastPosSecond = sortedFileSecond.tellg();

			FileUtils::readData<uint32_t>(sortedFileFirst, firstNumber);
			FileUtils::readData<uint32_t>(sortedFileSecond, secondNumber);

			if (isReadTillEnd(sortedFileFirst, sortedFileSecond, newSortedFile))
				break;

			if (firstNumber < secondNumber) {
				FileUtils::writeData<uint32_t>(newSortedFile, firstNumber);
				sortedFileSecond.seekg(lastPosSecond);
			}
			else if (firstNumber > secondNumber) {
				FileUtils::writeData<uint32_t>(newSortedFile, secondNumber);
				sortedFileFirst.seekg(lastPosFirst);
			}
			else if (firstNumber == secondNumber) {
				FileUtils::writeData<uint32_t>(newSortedFile, firstNumber);
				FileUtils::writeData<uint32_t>(newSortedFile, secondNumber);
			}
		}
		mMergedFilesCounter += 2;
#ifdef _DEBUG
		std::lock_guard<std::mutex> lock(mDebugPrintMtx);
		printFileContent(newSortedFile);
#endif		
	}
	catch (std::exception& ex)
	{
		{
			std::lock_guard<std::mutex> lock(mAdditionalFilesMtx);
			mMergingAborted = true;
			mFileFailures.push_back(ex.what());
			return;
		}
	}
}

std::string Sorter::getAdditionalFileNameByIndex(const uint32_t idx)
{
	std::string fileName;
	auto additionalFileNameIt = mAdditionalFilesNames.begin();
	std::advance(additionalFileNameIt, idx);
	fileName = *additionalFileNameIt;
	return fileName;
}


bool Sorter::isReadTillEnd(std::fstream& first, std::fstream& second, std::fstream& out)
{
	bool isReadTillEnd = false;

	if (FileUtils::isEoF(first) == true && FileUtils::isEoF(second) == false) {
		readTillEnd(second, out);
		isReadTillEnd = true;
	}
	else if (FileUtils::isEoF(second) == true && FileUtils::isEoF(first) == false) {
		readTillEnd(first, out);
		isReadTillEnd = true;
	}
	else if (FileUtils::isEoF(first) && FileUtils::isEoF(second))
		isReadTillEnd = true;
	return isReadTillEnd;
}


void Sorter::readTillEnd(std::fstream & in, std::fstream & out)
{
	uint64_t pos = in.tellg();
	in.seekg(pos - sizeof(uint32_t));
	FileUtils::copyContent(in, out, false);
}



void Sorter::sortParts()
{
	std::list<std::thread> sortingThreads;
	for (int i = 0; i < mWorkersAmount; i++)
	{
		sortingThreads.push_back(std::thread(&Sorter::sortPartsHelper, this));
	}

	waitForStartedThreads(sortingThreads);
}

void Sorter::sortPartsHelper()
{
	try
	{
		while (!endSorting())
		{
			std::vector<uint32_t> vecToSort(mSortingBufferSize / sizeof(uint32_t));
			{
				std::lock_guard<std::mutex> lock(mFileInMtx);
				FileUtils::readData<uint32_t>(mFileIn, vecToSort, mSortingBufferSize);
			}

			std::sort(vecToSort.begin(), vecToSort.end());

			std::string newSortedFileName;
			{
				std::lock_guard<std::mutex> lock(mAdditionalFilesMtx);
				newSortedFileName = generateSortedFileName();
				mAdditionalFilesNames.push_back(newSortedFileName);
			}

			std::fstream newSortedFile;
			newSortedFile.exceptions(mFileExceptionMask);
			newSortedFile.open(newSortedFileName, std::fstream::in | std::fstream::app | std::fstream::binary);

			if (!newSortedFile.is_open())
				throw std::runtime_error("ERROR : Error on file opening");

			FileUtils::writeData<uint32_t>(newSortedFile, vecToSort, mSortingBufferSize);

#ifdef _DEBUG
			std::lock_guard<std::mutex> lock(mDebugPrintMtx);
			printFileContent(newSortedFile);
#endif
		}
	}
	catch (std::exception& ex)
	{
		{
			std::lock_guard<std::mutex> lock(mAdditionalFilesMtx);
			mSortingAborted = true;
			mFileFailures.push_back(ex.what());
			return;
		}
	}
}


bool Sorter::endSorting()
{
	bool isSortingEnded = false;
	{
		std::lock_guard<std::mutex> lock(mPartsMtx);
		++mSortedParts;
		if ((mSortedParts > mPartsAmount) || mSortingAborted)
			isSortingEnded = true;
	}
	return isSortingEnded;
}


std::string Sorter::generateSortedFileName()
{
	std::string additionalFileName = "sorted_part_" + std::to_string(++mAdditionalFilesCounter);
	return additionalFileName;
}

void Sorter::waitForStartedThreads(std::list<std::thread>& threads)
{
	for (std::thread& thread : threads) {
		thread.join();
	}
}

void Sorter::printFileContent(std::fstream& file)
{
	std::cout << "----------------------------------------------------------------------------------------------------" << std::endl;

	uint64_t fileSize = 0;
	FileUtils::getFileSize(file, fileSize);

	std::vector<uint32_t> vec(fileSize / sizeof(uint32_t));

	FileUtils::resetFilePosition(file);
	FileUtils::readData<uint32_t>(file, vec, fileSize);

	for (auto& el : vec)
		std::cout << el << std::endl;
	FileUtils::resetFilePosition(file);

	std::cout << "----------------------------------------------------------------------------------------------------" << std::endl;
}

void Sorter::onErrorOccured()
{
	for (auto& failure : mFileFailures)
	{
		std::cerr << failure << std::endl;
	}
	removeAdditionalFiles(mAdditionalFilesNames.size());
}

bool Sorter::isSorted(std::fstream & file, const uint32_t maxCheckBufferSize)
{
	uint64_t fileSize = 0;
	FileUtils::getFileSize(file, fileSize);
	FileUtils::resetFilePosition(file);

	uint32_t lastInBuffer = 0, idx = 0, partsAmount = fileSize / maxCheckBufferSize;
	bool isSorted = true, compareWithLast = false;
	if (fileSize < maxCheckBufferSize) 
	{
		std::vector<uint32_t> vec(fileSize / sizeof(uint32_t));
		isSorted = std::is_sorted(vec.begin(), vec.end());
	}
	else 
	{
		while (idx < partsAmount)
		{
			std::vector<uint32_t> vec(maxCheckBufferSize / sizeof(uint32_t));
			FileUtils::readData<uint32_t>(file, vec, maxCheckBufferSize);
			isSorted = std::is_sorted(vec.begin(), vec.end());
			if (!isSorted)
				break;
			if ((idx > 0 && lastInBuffer > vec.front()))
			{
				isSorted = false;
				break;
			}
			lastInBuffer = vec.back();
			++idx;
		}
	}
	return isSorted;
}

