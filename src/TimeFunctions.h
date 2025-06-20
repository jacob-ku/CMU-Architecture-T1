//---------------------------------------------------------------------------

#ifndef TimeFunctionsH
#define TimeFunctionsH

__int64 GetCurrentTimeInMsec(void);
char *TimeToChar(__int64 hmsm);
void TimeDifferenceInSecToChar(__int64 timestamp, char *buffer, int bufferSize);
//---------------------------------------------------------------------------
#endif
