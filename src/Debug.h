#ifndef DebugH    
#define DebugH

bool saveTextToFile(const std::string& filePath, const std::string& text);
std::ofstream openFile(const std::string& filePath);
void writeFile(std::ofstream& outFile, const std::string& text);
void closeFile(std::ofstream& outFile); 

void printLog(const std::string& text);

__int64 __fastcall getTime_Start();
std::string __fastcall getTime_Elapsed(__int64 startTime);

#endif

