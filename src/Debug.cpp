//------------------------------------------------------------------------------
// Debug.cpp
// Implements a function to save a given text string to a text file.
//------------------------------------------------------------------------------

#include <fstream>
#include <string>
#include "Debug.h"
#include <iostream>
#include "TimeFunctions.h"

// Saves the given text to the specified file path.
// Parameters:
//   filePath - the path to the text file to write
//   text - the string content to save
// Returns true if successful, false otherwise.
bool saveTextToFile(const std::string& filePath, const std::string& text)
{
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (!outFile.is_open()) {
        return false;
    }
    outFile << text;
    std::cout << text;
    
    outFile.close();
    return true;
}

//------------------------------------------------------------------------------
// Opens a text file for writing (creates if it doesn't exist) and returns a reference.
// Parameters:
//   filePath - the path to the text file to open
// Returns: reference to the opened std::ofstream object
std::ofstream openFile(const std::string& filePath)
{
    std::ofstream outFile(filePath, std::ios::out | std::ios::app);
    return outFile;
}

// Writes the given text to the provided file stream.
// Parameters:
//   outFile - reference to an open std::ofstream
//   text - the string content to write
void writeFile(std::ofstream& outFile, const std::string& text)
{
    if (outFile.is_open()) {
        outFile << text << std::endl;
    }
}

// Closes the provided file stream.
// Parameters:
//   outFile - reference to an open std::ofstream
void closeFile(std::ofstream& outFile)
{
    if (outFile.is_open()) {
        outFile.close();
    }
}

void printLog(const std::string& text)
{
    std::cout << text << std::endl;
}

__int64 __fastcall getTime_Start()
{
    return GetCurrentTimeInMsec();
}

std::string __fastcall getTime_Elapsed(__int64 startTime)
{
    __int64 endTime = GetCurrentTimeInMsec();
    __int64 elapsedTime = endTime - startTime;
    std::string result = TimeToChar(elapsedTime);
    return result;
}