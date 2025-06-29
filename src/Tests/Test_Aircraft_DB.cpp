#include <vcl.h>
#include <tchar.h>

#pragma hdrstop              // Use precompiled headers
#pragma link "..\HashTable\Lib\Win64\Release\HashTableLib.a"

#include <windows.h>         // For Sleep()
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "AircraftDB.h"


int _tmain(int argc, _TCHAR* argv[])
{
    puts("== AircraftDB simple test ==");

    // 1) Create Instance
    TAircraftDB db;

    // 2) Start Async Loading
    db.LoadDatabaseAsync("..\\..\\data\\test_aircrafts_db.csv");

    // 3) Wait for loading (max 5 sec)
    for (int i = 0; i < 500 && db.IsLoading(); ++i)
        Sleep(10);

    assert(db.IsInitialized());                       // Check Loading success
    const uint32_t ICAO_1 = 0xAA3487;
    const uint32_t ICAO_2 = 0xA4FA61;

    const TAircraftData* rec1 = db.GetAircraftDBInfo(ICAO_1);
    const TAircraftData* rec2 = db.GetAircraftDBInfo(ICAO_2);

    assert(rec1 != nullptr);
    assert(rec2 != nullptr);

    assert(rec1->ICAO24 == ICAO_1);
    assert(rec2->ICAO24 == ICAO_2);

    assert(stricmp(rec1->Registration.c_str(), "N757F") == 0);
    assert(stricmp(rec2->Registration.c_str(), "N42MH") == 0);

    assert(db.GetAircraftDBInfo(0xABCDEF) == nullptr);

    assert(db.GetItemNumAircraftDB() == 2);
    db.DisplayAllAircraftsInfo();

    puts("ALL TESTS PASSED");

    // Wait to check result
    puts("Press Enter to exit...");
    fflush(stdout);
    getchar();

    return 0;
}
