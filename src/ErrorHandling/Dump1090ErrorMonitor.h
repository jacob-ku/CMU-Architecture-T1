#ifndef DUMP1090ERRORMONITOR_H
#define DUMP1090ERRORMONITOR_H
#include "AbstractErrorMonitor.h"
#include <memory>
#include <iostream>
#include <thread>

class Dump1090ErrorMonitor : public AbstractErrorMonitor {
public: 
    Dump1090ErrorMonitor(TErrorHandler handler) : AbstractErrorMonitor(handler) {}
    virtual ~Dump1090ErrorMonitor() = default;
    virtual void CheckError() override {
        if (mSshSession) {
            // std::cout << "Checking dump1090 " << *mErrorMask << " tid=" << std::this_thread::get_id() << "\n";
            std::string output;
            if (!mSshSession->run_command("ps aux | grep '[d]ump1090'", output)) {
                // std::cout << "Command failed. PI may disconnected " << *mErrorMask << std::endl;
                if ((*mErrorMask & BITMASK_SSH_DISCONNECTED) == 0) {
                    *mErrorMask |= BITMASK_SSH_DISCONNECTED; // Set the error mask
                    // std::cout << "Error to UI: " << *mErrorMask;
                    if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                }
            } else {
                if (*mErrorMask & BITMASK_SSH_DISCONNECTED) {
                    *mErrorMask &= ~BITMASK_SSH_DISCONNECTED; // Clear the error mask
                    // std::cout << "Error to UI: " << *mErrorMask;
                    if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                }
                // check if RTL2832U is present in the output
                if (output.find("dump1090") == std::string::npos) {
                    if ((*mErrorMask & BITMASK_DUMP1090_NOT_RUNNING) == 0) {
                        *mErrorMask |= BITMASK_DUMP1090_NOT_RUNNING; // Set the error mask
                        // std::cout << "Error to UI: " << *mErrorMask;
                        if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                    }
                } else {
                    // clear BITMASK_DUMP1090_NOT_RUNNING
                    if (*mErrorMask & BITMASK_DUMP1090_NOT_RUNNING) {
                        *mErrorMask &= ~BITMASK_DUMP1090_NOT_RUNNING; // Clear the error mask
                        // std::cout << "Error to UI: " << *mErrorMask;
                        if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                    }
                }
                // std::cout << output << std::endl;
            }
        }
    }
};

#endif