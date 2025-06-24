//---------------------------------------------------------------------------

#ifndef AircraftDBH
#define AircraftDBH

#include <System.hpp> // For AnsiString
#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <thread>
#include <mutex>
#include "ght_hash_table.h"
#include "csv.h" // For CSV_context

#define AC_DB_NUM_FIELDS 27
#define AC_DB_ICAO 0
#define AC_DB_Registration 1
#define AC_DB_ManufacturerICAO 2
#define AC_DB_ManufacturerName 3
#define AC_DB_Model 4
#define AC_DB_TypeCode 5
#define AC_DB_SerialNumber 6
#define AC_DB_LineNumber 7
#define AC_DB_ICAOAircraftType 8
#define AC_DB_Operator 9
#define AC_DB_OperatorCallSign 10
#define AC_DB_OperatorICAO 11
#define AC_DB_OperatorIATA 12
#define AC_DB_Owner 13
#define AC_DB_TestReg 14
#define AC_DB_Registered 15
#define AC_DB_RegUntil 16
#define AC_DB_Status 17
#define AC_DB_Built 18
#define AC_DB_FirstFlightDate 19
#define AC_DB_Seatconfiguration 20
#define AC_DB_Engines 21
#define AC_DB_Modes 22
#define AC_DB_ADSB 23
#define AC_DB_ACARS 24
#define AC_DB_Notes 25
#define AC_DB_CategoryDescription 26

typedef struct
{
    uint32_t ICAO24;
    AnsiString Fields[AC_DB_NUM_FIELDS];
} TAircraftData;

class TAircraftDB
{
private:
    ght_hash_table_t *FAircraftDBHashTable;     // Hash table to store aircraft database records.
    std::mutex FMutex;                         // Mutex for thread-safe access to shared data.
    bool FLoading;                             // Flag to indicate if the database is currently being loaded.
    bool FInitialized;                         // Flag to indicate if the database has been successfully initialized.
    AnsiString FFileName;                      // The file name of the aircraft database.

    void LoadDatabase();                       // Loads the aircraft database from the file and logs start/end time.
    const char *GetCountry(uint32_t addr, bool get_short);        // Retrieves the country name for a given ICAO address.
    bool IsMilitary(uint32_t addr, const char **country);         // Checks if an aircraft is military.
    bool IsHelicopter(uint32_t addr, const char **type_ptr);      // Checks if an aircraft is a helicopter.
    bool IsHelicopterType(const char *type);                     // Checks if the given aircraft type is a helicopter type.
    const char *GetMilitary(uint32_t addr);                      // Retrieves military information for a given ICAO address.

    static int CSV_callback(struct CSV_context *ctx, const char *value);    // Callback function for parsing CSV data.

public:
    TAircraftDB();                            // Constructor for the TAircraftDB class.
    ~TAircraftDB();                           // Destructor for the TAircraftDB class.

    void LoadDatabaseAsync(AnsiString FileName);   // Asynchronously loads the aircraft database.
    const char *GetAircraftDBInfo(uint32_t addr);  // Retrieves aircraft database information for a given ICAO address.
    bool IsInitialized() {                        // Checks if the database is initialized.
        std::lock_guard<std::mutex> lock(FMutex);
        return FInitialized;
    }
    bool IsLoading() {                            // Checks if the database is currently loading.
        std::lock_guard<std::mutex> lock(FMutex);
        return FLoading;
    }

    bool aircraft_is_registered(uint32_t addr);
};

extern TAircraftDB *AircraftDB;

//---------------------------------------------------------------------------
#endif
