#ifndef PIErrorMonitor_h
#define PIErrorMonitor_h
#include "Util/SshSession.h"
#include <Classes.hpp>
#include "AbstractErrorMonitor.h"
#include "ErrorDefinition.h"
#include <vector>
#include <memory>
extern std::shared_ptr<int> G_ErrorMask;
class PIErrorMonitor : public TThread {
public: 
    void __fastcall Execute();
    __fastcall PIErrorMonitor(bool value);
    ~PIErrorMonitor();
    int initSshConnection(const char* host, const char* user, const char* pass);
    void registerErrorHandler(TErrorHandler handler) {
        mOnErrorCbk = handler;
        for (auto& monitor : errorMonitors) {
            monitor->mOnErrorCbk = handler;
        }
    }
    void addErrorMonitor(std::shared_ptr<AbstractErrorMonitor> monitor) {
        if (monitor) {
            monitor->mSshSession = mSshSession; // Share the SSH session
            monitor->mOnErrorCbk = mOnErrorCbk;
            monitor->mErrorMask = G_ErrorMask; // Share the error mask
            errorMonitors.push_back(monitor);
        }
    }

private:
    std::shared_ptr<SSHPersistentSession> mSshSession;
    std::vector<std::shared_ptr<AbstractErrorMonitor>> errorMonitors;
    TErrorHandler mOnErrorCbk = nullptr; // Default error handler
};
#endif