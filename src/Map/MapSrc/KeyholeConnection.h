//---------------------------------------------------------------------------

#ifndef KeyholeConnectionH
#define KeyholeConnectionH
#include <gefetch.h>

#include "SimpleTileStorage.h"

#define GoogleMaps               0
#define GoogleMaps_Street        1
#define GoogleMaps_TerrainLabels 2
#define SkyVector_VFR            3
#define SkyVector_IFR_Low        4
#define SkyVector_IFR_High       5
#define Open_Street              6
#define SkyVector                7

/**
 * Connection to Google server.
 *
 * Handles and encapsulates all network stuff, authentification and
 * downloading of tiles from Google servers. Of course, only handles
 * loading of tiles.
 *
 * @deprecated should be restructured and merged to GoogleLayer
 */
class KeyholeConnection: public SimpleTileStorage {
public:
	/**
	 * Constructor.
	 */
	KeyholeConnection(int Type);

	/**
	 * Destructor.
	 */
	virtual ~KeyholeConnection();

protected:
	/**
	 * Download tile from google.
	 */
	void Process(TilePtr tile);

	/**
	 * Cleanup resources after thread termination.
	 */
	virtual void CleanupResources();

private:
	gefetch_t	m_GEFetch;
	int         ServerType;
	int         DelayTime;
	const char  *Key;
	const char  *Chart;
	const char  *Edition;
};

//---------------------------------------------------------------------------
#endif
