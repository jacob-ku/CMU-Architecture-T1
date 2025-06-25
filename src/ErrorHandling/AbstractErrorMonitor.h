#ifndef ABSTRACT_ERROR_MONITOR_H
#define ABSTRACT_ERROR_MONITOR_H
#include "ErrorDefinition.h"
#include "Util/SShSession.h"
#include <memory>
#include <Classes.hpp>

class AbstractErrorMonitor : public TObject {
public: 
    TErrorHandler mOnErrorCbk = nullptr;
    std::shared_ptr<int> mErrorMask; //FIXME: bug when reconnect, error is not cleared
    std::shared_ptr<SSHPersistentSession> mSshSession;
    AbstractErrorMonitor(TErrorHandler handler = nullptr) : mOnErrorCbk(handler) {}
    AbstractErrorMonitor(const AbstractErrorMonitor&) = delete; // Disable copy constructor
    virtual ~AbstractErrorMonitor() = default;
    virtual void CheckError() = 0;
};

#endif