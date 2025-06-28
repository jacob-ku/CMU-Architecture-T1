#ifndef DATAUPDATERINTERFACE_H
#define DATAUPDATERINTERFACE_H

#include <string>
#include <functional>

class DataUpdaterInterface {
public:
    virtual ~DataUpdaterInterface() = default;
    

    virtual bool update(const std::string& src, const std::function<void(bool)>& callback) = 0;
    virtual bool initialize() = 0;
    virtual bool reset() = 0;
    virtual bool stop() = 0;
    
    virtual void scheduleUpdate(int intervalMinutes) = 0;
    virtual void stopUpdates() = 0;
};

#endif