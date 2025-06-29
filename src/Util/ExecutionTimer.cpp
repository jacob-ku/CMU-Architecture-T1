// Utility/ExecutionTimer.cpp
#include "ExecutionTimer.h"
#include <stdio.h>

ExecutionTimer::ExecutionTimer()
{
    startTime = GetCurrentTimeInMsec();
}

ExecutionTimer::~ExecutionTimer()
{  ;
}

int64_t ExecutionTimer::Elapsed() const
{
    return GetCurrentTimeInMsec() - startTime;
}
