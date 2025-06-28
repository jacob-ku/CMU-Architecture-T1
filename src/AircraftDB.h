//---------------------------------------------------------------------------

#ifndef AircraftDBH
#define AircraftDBH

#include <System.hpp> // For AnsiString
#include <System.Classes.hpp>
#include <System.SysUtils.hpp>
#include <atomic>
#include <thread>
#include <mutex>
#include "ght_hash_table.h"
#include "csv.h" // For CSV_context

typedef struct
{
    uint32_t ICAO24;
    AnsiString Registration;
    AnsiString ManufacturerICAO;
    AnsiString ManufacturerName;
    AnsiString Model;
    AnsiString TypeCode;
    AnsiString SerialNumber;
    AnsiString LineNumber;
    AnsiString ICAOAircraftType;
    AnsiString OperatorName;
    AnsiString OperatorCallsign;
    AnsiString OperatorICAO;
    AnsiString OperatorIata;
    AnsiString Owner;
    AnsiString TestReg;
    AnsiString Registered;
    AnsiString RegUntil;
    AnsiString Status;
    AnsiString Built;
    AnsiString FirstFlightDate;
    AnsiString SeatConfiguration;
    AnsiString Engines;
    AnsiString Modes;
    AnsiString ADSB;
    AnsiString ACARS;
    AnsiString Notes;
    AnsiString CategoryDescription;

    AnsiString toString() const {
        AnsiString s;
        s += "ICAO24: " + IntToHex((int)ICAO24, 6) + ", ";
        s += "Registration: " + Registration + ", ";
        s += "ManufacturerICAO: " + ManufacturerICAO + ", ";
        s += "ManufacturerName: " + ManufacturerName + ", ";
        s += "Model: " + Model + ", ";
        s += "TypeCode: " + TypeCode + ", ";
        s += "SerialNumber: " + SerialNumber + ", ";
        s += "LineNumber: " + LineNumber + ", ";
        s += "ICAOAircraftType: " + ICAOAircraftType + ", ";
        s += "OperatorName: " + OperatorName + ", ";
        s += "OperatorCallsign: " + OperatorCallsign + ", ";
        s += "OperatorICAO: " + OperatorICAO + ", ";
        s += "OperatorIata: " + OperatorIata + ", ";
        s += "Owner: " + Owner + ", ";
        s += "TestReg: " + TestReg + ", ";
        s += "Registered: " + Registered + ", ";
        s += "RegUntil: " + RegUntil + ", ";
        s += "Status: " + Status + ", ";
        s += "Built: " + Built + ", ";
        s += "FirstFlightDate: " + FirstFlightDate + ", ";
        s += "SeatConfiguration: " + SeatConfiguration + ", ";
        s += "Engines: " + Engines + ", ";
        s += "Modes: " + Modes + ", ";
        s += "ADSB: " + ADSB + ", ";
        s += "ACARS: " + ACARS + ", ";
        s += "Notes: " + Notes + ", ";
        s += "CategoryDescription: " + CategoryDescription;
        return s;
    }
} TAircraftData;

class TAircraftDB
{
private:
    ght_hash_table_t *FAircraftDBHashTable;     // Hash table to store aircraft database records.
    std::mutex FMutex;                         // Mutex for thread-safe access to shared data.
    bool FLoading;                             // Flag to indicate if the database is currently being loaded.
    bool FInitialized;                         // Flag to indicate if the database has been successfully initialized.
    AnsiString FFileName;                      // The file name of the aircraft database.
    std::thread* FLoadThread = nullptr;
    std::atomic<bool> FCancelLoading = false;

    void LoadDatabase();                       // Loads the aircraft database from the file.

    static int CSV_callback(struct CSV_context *ctx, const char *value);    // Callback function for parsing CSV data.

public:
    TAircraftDB();                            // Constructor for the TAircraftDB class.
    ~TAircraftDB();                           // Destructor for the TAircraftDB class.

    void LoadDatabaseAsync(AnsiString FileName);   // Asynchronously loads the aircraft database.
    void CancelAndJoin();
    bool IsInitialized() {                        // Checks if the database is initialized.
        std::lock_guard<std::mutex> lock(FMutex);
        return FInitialized;
    }
    bool IsLoading() {                            // Checks if the database is currently loading.
        std::lock_guard<std::mutex> lock(FMutex);
        return FLoading;
    }

    const TAircraftData *GetAircraftDBInfo(uint32_t addr);  // Retrieves aircraft database information for a given ICAO address.
    const char *GetCountry(uint32_t addr, bool get_short);        // Retrieves the country name for a given ICAO address.
    bool IsMilitary(uint32_t addr, const char **country);         // Checks if an aircraft is military.
    bool IsHelicopter(uint32_t addr, const char **type_ptr);      // Checks if an aircraft is a helicopter.
    bool IsHelicopterType(const char *type);                     // Checks if the given aircraft type is a helicopter type.
    const char *GetMilitary(uint32_t addr);                      // Retrieves military information for a given ICAO address.

    bool aircraft_is_registered(uint32_t addr);
};

extern TAircraftDB *AircraftDB;

//---------------------------------------------------------------------------
#endif
