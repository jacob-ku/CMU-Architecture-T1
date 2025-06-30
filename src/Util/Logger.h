#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>

// Enable or disable logging
#define LOGGER_ENABLE 1

#if LOGGER_ENABLE
    #define LOG(msg) (std::cout << (msg) << std::endl)
#else
    #define LOG(msg)
#endif

#endif // LOGGER_H
