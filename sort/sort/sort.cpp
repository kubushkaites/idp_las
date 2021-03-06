// sort.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Sorter.h"

int main(int argc, char* argv[])
{
	std::srand(std::time(0));

	std::fstream numbersIn;
	std::fstream numbersOut;
	numbersIn.exceptions(std::fstream::badbit);
	numbersOut.exceptions(std::fstream::badbit);

	if (argc == 3)
	{
		numbersIn.open(argv[1], std::fstream::in | std::fstream::app | std::fstream::binary);
		numbersOut.open(argv[2], std::fstream::in | std::fstream::app | std::fstream::binary);
	}
	else 
	{
		numbersIn.open("numbersIn", std::fstream::in | std::fstream::app | std::fstream::binary);
		numbersOut.open("numbersOut", std::fstream::in | std::fstream::app | std::fstream::binary);
	}
	
	if (numbersIn.is_open() && numbersOut.is_open()) {
		Sorter sorter(numbersIn, numbersOut);
		auto start = std::chrono::high_resolution_clock::now();
		sorter.sort();
		auto finish = std::chrono::high_resolution_clock::now();
		auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(finish - start);
		std::cout << "Duration of sorting : " << durationMs.count() << " ms ;" << std::endl;
		std::cout << "Is sorted correctly? " << std::boolalpha << sorter.isSorted(numbersOut, sorter.getMaxRamUsage()) << std::endl;
	}
	return 0;
}

