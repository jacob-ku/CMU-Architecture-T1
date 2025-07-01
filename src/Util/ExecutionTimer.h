#ifndef EXECUTION_TIMER_H
#define EXECUTION_TIMER_H

#pragma once
#include <stdint.h>
#include "TimeFunctions.h"

#ifdef ELAPSED_TIME_CHK
    #define EXECUTION_TIMER(timerName) ExecutionTimer (timerName)
    #define EXECUTION_TIMER_ELAPSED(elapsed, timerName) int64_t (elapsed) = (timerName).Elapsed()
#else
    #define EXECUTION_TIMER(timerName)
    #define EXECUTION_TIMER_ELAPSED(elapsed, timerName) int64_t (elapsed) = 0
#endif

class ExecutionTimer {
public:
    ExecutionTimer();
    ~ExecutionTimer();
    int64_t Elapsed() const;
private:
    int64_t startTime;
};

#endif
