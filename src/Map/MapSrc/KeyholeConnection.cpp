//---------------------------------------------------------------------------


#pragma hdrstop

#include "KeyholeConnection.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
#ifdef WIN32
#include <windows.h>
#define sleep(sec) Sleep(1000*(sec))
#endif

// https://skyvector.com/api/chartDataFPL


#define GOOGLE_URL               "http://mt1.google.com"
#define OPENSTREET_URL           "http://b.tile.openstreetmap.org"
#define SKYVECTOR_URL            "http://t.skyvector.com"
#define SKYVECTOR_CHART_VPS      "301"
#define SKYVECTOR_CHART_IFR_LOW  "302"
#define SKYVECTOR_CHART_IFR_HIGH "304"
#define SKYVECTOR_KEY             "V7pMh4xRihf1nr61"
#define SKYVECTOR_EDITION         "2504"

#define MAX_DELAY_TIME (30*1000) // ms
#define MIN_DELAY_TIME 10 // ms
#define BACKOFF_FACTOR 10

KeyholeConnection::KeyholeConnection(int type)
{
  const char * url;
	if (type== GoogleMaps)
	{
	 ServerType= GoogleMaps;
	 url=GOOGLE_URL;
    }
    else if (type == GoogleMaps_Street)
    {
        ServerType = GoogleMaps_Street;
        url=GOOGLE_URL;
    }
    else if (type == GoogleMaps_TerrainLabels)
    {
        ServerType = GoogleMaps_TerrainLabels;
        url=GOOGLE_URL;
    }
    else if (type == Open_Street)
    {
        ServerType = Open_Street;
        url = OPENSTREET_URL;
    }
	else if (type== SkyVector_VFR)
	{
	  ServerType= SkyVector;
	  url=SKYVECTOR_URL;
	  Key=SKYVECTOR_KEY;
	  Chart=SKYVECTOR_CHART_VPS;
	  Edition=SKYVECTOR_EDITION;
	}
	else if (type== SkyVector_IFR_Low)
	{
	  ServerType= SkyVector;
	  url=SKYVECTOR_URL;
	  Key=SKYVECTOR_KEY;
	  Chart=SKYVECTOR_CHART_IFR_LOW;
	  Edition=SKYVECTOR_EDITION;
	}
	else if (type== SkyVector_IFR_High)
	{
	  ServerType= SkyVector;
	  url=SKYVECTOR_URL;
	  Key=SKYVECTOR_KEY;
	  Chart=SKYVECTOR_CHART_IFR_HIGH;
	  Edition=SKYVECTOR_EDITION;
	}
	if ((m_GEFetch = gefetch_init(url)) == 0)
		throw Exception("gefetch_init() failed");
	DelayTime = MIN_DELAY_TIME;
}

KeyholeConnection::~KeyholeConnection() {
}

void KeyholeConnection::CleanupResources() {
	if (m_GEFetch) {
		printf("KeyholeConnection: cleaning up gefetch\n");
		gefetch_cleanup(m_GEFetch);
		m_GEFetch = 0;
	}
}

void KeyholeConnection::Process(TilePtr tile) {
	gefetch_error res;
    if (ServerType== GoogleMaps)
    {
      res = gefetch_fetch_image_googlemaps(m_GEFetch, tile->GetX(), tile->GetY(), tile->GetLevel());
    }
    else if (ServerType == GoogleMaps_Street)
    {
        res = gefetch_fetch_image_googlemaps_with_type(m_GEFetch, tile->GetX(), tile->GetY(), tile->GetLevel(), "m");
    }
    else if (ServerType == GoogleMaps_TerrainLabels)
    {
        res = gefetch_fetch_image_googlemaps_with_type(m_GEFetch, tile->GetX(), tile->GetY(), tile->GetLevel(), "p");
    }
	else if (ServerType == Open_Street)
	{
		res = gefetch_fetch_image_openstreetmap(m_GEFetch, tile->GetX(), tile->GetY(), tile->GetLevel());
	}
    else if (ServerType== SkyVector)
	{
	  res = gefetch_fetch_image_skyvector(m_GEFetch,Key,Chart,Edition, tile->GetX(), tile->GetY(), tile->GetLevel());
    }

	if ((res == GEFETCH_NOT_FOUND) ||  (res == GEFETCH_INVALID_ZOOM))
	{
		tile->Null();
		return;
	}
	else if (res != GEFETCH_OK) {
		printf("map tile fetch error, sleep %d\n", DelayTime);
		Sleep(DelayTime);	/* don't do a DOS in case of any problems */
		DelayTime = DelayTime * BACKOFF_FACTOR;
		if (DelayTime > MAX_DELAY_TIME)
			DelayTime = MAX_DELAY_TIME;
		//throw Exception("gefetch_fetch_image() failed");
	} else {
		DelayTime = MIN_DELAY_TIME;
	}

	RawBuffer *buf = new RawBuffer(gefetch_get_data_ptr(m_GEFetch), gefetch_get_data_size(m_GEFetch));

	try {
		tile->Load(buf, m_pSaveStorage != 0);
	} catch (...) {
		delete buf;
		throw;
	}
}

