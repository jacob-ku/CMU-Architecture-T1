#ifndef ErrorType_h
#define ErrorType_h
#include <Classes.hpp>

typedef void __fastcall (__closure *TErrorHandler)(const int &code);

#define BITMASK_SSH_DISCONNECTED 0x00000001
#define BITMAKS_SDRUSB_DISCONNECTED 0x00000002
#define BITMASK_DUMP1090_NOT_RUNNING 0x00000004


#endif