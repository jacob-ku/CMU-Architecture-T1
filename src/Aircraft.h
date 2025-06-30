//---------------------------------------------------------------------------

#ifndef AircraftH
#define AircraftH
#include "DecodeRawADS_B.h"

#define MODES_NON_ICAO_ADDRESS (1 << 24) // Set on addresses to indicate they are not ICAO addresses

typedef struct
{
    uint32_t ICAO;
    char HexAddr[7];     /* Printable ICAO address */
    __int64 LastSeen;    /* Time at which the last packet was received. */
    char TimeElapsedInSec[3]; /* Time elapsed in seconds since the last packet was received. */
    long NumMessagesRaw; /* Number of Mode S messages received. */
    long NumMessagesSBS;
    int odd_cprlat; /* Encoded latitude and longitude as extracted by odd and even */
    int odd_cprlon; /* CPR encoded messages. */
    int even_cprlat;
    int even_cprlon;
    __int64 odd_cprtime;
    __int64 even_cprtime;
    char FlightNum[9]; /* Flight number */
    bool HaveFlightNum;
    bool HaveAltitude;
    double Altitude;
    bool HaveLatLon;
    double Latitude;  /* Coordinated obtained from CPR encoded data. */
    double Longitude; /* Coordinated obtained from CPR encoded data. */
    bool HaveSpeedAndHeading;
    double Heading; /* Heading in degrees, 0-360 */
    double Speed;   /* Speed in knots */
    double VerticalRate;
    int SpriteImage;
    bool IsHelicopter;
    bool IsMilitary;
} TADS_B_Aircraft;

void RawToAircraft(modeS_message *mm, TADS_B_Aircraft *ADS_B_Aircraft);
//---------------------------------------------------------------------------
#endif
