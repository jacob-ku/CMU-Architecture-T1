#ifndef SDRUSBERRORMONITOR_H
#define SDRUSBERRORMONITOR_H
#include "AbstractErrorMonitor.h"
#include <memory>
#include <iostream>
#include <thread>

class SdrUsbErrorMonitor : public AbstractErrorMonitor {
public: 
    SdrUsbErrorMonitor(TErrorHandler handler) : AbstractErrorMonitor(handler) {}
    virtual ~SdrUsbErrorMonitor() = default;
    virtual void CheckError() override {
        // Implement the error checking logic for SDR USB
        // std::cout << "Checking SDR USB errors cmd...\n";
        if (mSshSession) {
            // std::cout << "Checking SDR USB errors " << *mErrorMask << " tid=" << std::this_thread::get_id() << "\n";
            std::string output;
            if (!mSshSession->run_command("lsusb | grep RTL2832U", output)) {
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
                if (output.find("RTL2832U") == std::string::npos) {
                    if ((*mErrorMask & BITMAKS_SDRUSB_DISCONNECTED) == 0) {
                        *mErrorMask |= BITMAKS_SDRUSB_DISCONNECTED; // Set the error mask
                        // std::cout << "Error to UI: " << *mErrorMask;
                        if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                    }
                } else {
                    // clear BITMAKS_SDRUSB_DISCONNECTED
                    if (*mErrorMask & BITMAKS_SDRUSB_DISCONNECTED) {
                        *mErrorMask &= ~BITMAKS_SDRUSB_DISCONNECTED; // Clear the error mask
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