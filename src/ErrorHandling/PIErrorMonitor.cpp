#include "PIErrorMonitor.h"
#include <iostream>
#include <thread>
#include "SdrUsbErrorMonitor.h"
#include "Dump1090ErrorMonitor.h"
#include "NetworkErrorMonitor.h"

std::shared_ptr<int> G_ErrorMask = std::make_shared<int>(0); // Shared error mask //TODO: use a better way to share error mask

__fastcall PIErrorMonitor::PIErrorMonitor(bool value) : TThread(value) {
	FreeOnTerminate = true; // Automatically free the thread object after execution
  addErrorMonitor(std::make_shared<SdrUsbErrorMonitor>(mOnErrorCbk));
  addErrorMonitor(std::make_shared<Dump1090ErrorMonitor>(mOnErrorCbk));
  addErrorMonitor(std::make_shared<NetworkErrorMonitor>(mOnErrorCbk));
}
__fastcall PIErrorMonitor::~PIErrorMonitor() { }

void __fastcall PIErrorMonitor::Execute(void)
{
  while (!Terminated) {
    // printf("PIErrorMonitor::Execute thread id %d\n", std::this_thread::get_id());
    for (auto& monitor : errorMonitors) {
      if (monitor) {
        monitor->CheckError();}
    }
    Sleep(1000);
  }
}

int PIErrorMonitor::initSshConnection(const char* host, const char* user, const char* pass) {
  try {
      // std::cout << "Initializing SSH connection to " << host << " as user " << user << "\n";
      mSshSession = std::make_shared<SSHPersistentSession>(host, user, pass);
      // *G_ErrorMask = 0; // Reset the error mask //FIXME:
      for (auto& monitor : errorMonitors) {
          if (monitor) {
              monitor->mSshSession = mSshSession; // Share the SSH session
          }
      }
      return 0; // Success
  } catch (const std::exception& ex) {
      std::cout << "SSH Error: " << ex.what() << "\n";
      return -1; // Failure
  }
}