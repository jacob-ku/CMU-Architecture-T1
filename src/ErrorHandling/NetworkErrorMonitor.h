#ifndef NETWORKERRORMONITOR_H
#define NETWORKERRORMONITOR_H
#include "AbstractErrorMonitor.h"
#include <memory>
#include <iostream>
#include <thread>
#include <cstdlib>

class NetworkErrorMonitor : public AbstractErrorMonitor {
public: 
    NetworkErrorMonitor(TErrorHandler handler) : AbstractErrorMonitor(handler) {}
    virtual ~NetworkErrorMonitor() = default;
    virtual void CheckError() override {
            std::string output;
            // check remote connection by ping to mSshSession->mHost
            // ping the remote host to see if it’s reachable
            std::string host = mSshSession->mHost;
            std::string pingCmd = "ping -n 1 " + host + " >NUL 2>&1";
            if (std::system(pingCmd.c_str()) != 0) {
                // host unreachable → mark SSH disconnected
                if ((*mErrorMask & BITMASK_SSH_DISCONNECTED) == 0) {
                    *mErrorMask |= BITMASK_SSH_DISCONNECTED;
                    if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                }
            } else {
                // host reachable again → clear SSH disconnected
                if (*mErrorMask & BITMASK_SSH_DISCONNECTED) {
                    *mErrorMask &= ~BITMASK_SSH_DISCONNECTED;
                    if (mOnErrorCbk) mOnErrorCbk(*mErrorMask);
                }
            }
    }
};

#endif