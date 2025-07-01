//---------------------------------------------------------------------------

#include <vcl.h>
#include <new>
#include <math.h>
#include <dir.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <fileapi.h>
#include <chrono>
#include <vector>
#include <algorithm>

#pragma hdrstop

#include "DisplayGUI.h"
#include "AreaDialog.h"
#include "ntds2d.h"
#include "AircraftFilter/ZoneFilter.h"
#include "AircraftFilter/ScreenFilter.h"
#include "AircraftFilter/MilitaryFilter.h"
#include "LatLonConv.h"
#include "PointInPolygon.h"
#include "DecodeRawADS_B.h"
#include "ght_hash_table.h"
#include "dms.h"
#include "Aircraft.h"
#include "TimeFunctions.h"
#include "SBS_Message.h"
#include "CPA.h"
#include "AircraftDB.h"
#include "csv.h"
#include "Map/Providers/MapProviderFactory.h"
#include "stb_image.h"

#include "MetadataManager/RouteManager.h"
#include "MetadataManager/AirportManager.h"
#include "Util/Logger.h"
#include "Util/WebDownloadManager.h"

/*  소요시간 측정용
    측정 시작:   EXECUTION_TIMER( SomeName );
    측정 종료:   EXECUTION_TIMER_ELAPSED( elapsedTimeInMs, SomeName );
    결과 사용:   printf("Elapsed time: %lld ms\n", elapsedTimeInMs);
*/
#define ELAPSED_TIME_CHK    // NOTE: 사용 시 Enable 할 것
#include "Util/ExecutionTimer.h"


#define AIRCRAFT_DATABASE_URL "https://opensky-network.org/datasets/metadata/aircraftDatabase.zip"
#define AIRCRAFT_DATABASE_FILE "aircraftDatabase.csv"
#define ARTCC_BOUNDARY_FILE "Ground_Level_ARTCC_Boundary_Data_2025-05-15.csv"

#define MAP_CENTER_LAT 40.73612;
#define MAP_CENTER_LON -80.33158;

#define BIG_QUERY_UPLOAD_COUNT 50000
#define BIG_QUERY_RUN_FILENAME "SimpleCSVtoBigQuery.py"
#define LEFT_MOUSE_DOWN 1
#define RIGHT_MOUSE_DOWN 2
#define MIDDLE_MOUSE_DOWN 4

#define BG_INTENSITY 0.37
#define ERROR_HANDLING_ENABLED
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "OpenGLPanel"
#pragma link "Map\libgefetch\Win64\Release\libgefetch.a"
#pragma link "Map\zlib\Win64\Release\zlib.a"
#pragma link "Map\jpeg\Win64\Release\jpeg.a"
#pragma link "Map\png\Win64\Release\png.a"
#pragma link "HashTable\Lib\Win64\Release\HashTableLib.a"
#pragma link "cspin"
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
static void RunPythonScript(AnsiString scriptPath, AnsiString args);
static bool DeleteFilesWithExtension(AnsiString dirPath, AnsiString extension);
static int FinshARTCCBoundary(void);
//---------------------------------------------------------------------------

static char *stristr(const char *String, const char *Pattern);
static const char *strnistr(const char *pszSource, DWORD dwLength, const char *pszFind);

//---------------------------------------------------------------------------
uint32_t createRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
//---------------------------------------------------------------------------
uint32_t PopularColors[] = {
    createRGB(255, 0, 0),     // Red
    createRGB(0, 255, 0),     // Green
    createRGB(0, 0, 255),     // Blue
    createRGB(255, 255, 0),   // Yellow
    createRGB(255, 165, 0),   // Orange
    createRGB(255, 192, 203), // Pink
    createRGB(0, 255, 255),   // Cyan
    createRGB(255, 0, 255),   // Magenta
    createRGB(255, 255, 255), // White
    // createRGB(0, 0, 0),        // Black
    createRGB(128, 128, 128), // Gray
    createRGB(165, 42, 42)    // Brown
};

int NumColors = sizeof(PopularColors) / sizeof(PopularColors[0]);
unsigned int CurrentColor = 0;

//---------------------------------------------------------------------------
typedef struct
{
    union
    {
        struct
        {
            System::Byte Red;
            System::Byte Green;
            System::Byte Blue;
            System::Byte Alpha;
        };
        struct
        {
            TColor Cl;
        };
        struct
        {
            COLORREF Rgb;
        };
    };

} TMultiColor;

//---------------------------------------------------------------------------
static const char *strnistr(const char *pszSource, DWORD dwLength, const char *pszFind)
{
    DWORD dwIndex = 0;
    DWORD dwStrLen = 0;
    const char *pszSubStr = NULL;

    // check for valid arguments
    if (!pszSource || !pszFind)
    {
        return pszSubStr;
    }

    dwStrLen = strlen(pszFind);

    // can pszSource possibly contain pszFind?
    if (dwStrLen > dwLength)
    {
        return pszSubStr;
    }

    while (dwIndex <= dwLength - dwStrLen)
    {
        if (0 == strnicmp(pszSource + dwIndex, pszFind, dwStrLen))
        {
            pszSubStr = pszSource + dwIndex;
            break;
        }

        dwIndex++;
    }

    return pszSubStr;
}
//---------------------------------------------------------------------------
static char *stristr(const char *String, const char *Pattern)
{
    char *pptr, *sptr, *start;
    size_t slen, plen;

    for (start = (char *)String, pptr = (char *)Pattern, slen = strlen(String), plen = strlen(Pattern);
         slen >= plen; start++, slen--)
    {
        /* find start of pattern in string */
        while (toupper(*start) != toupper(*Pattern))
        {
            start++;
            slen--;

            /* if pattern longer than string */

            if (slen < plen)
                return (NULL);
        }

        sptr = start;
        pptr = (char *)Pattern;

        while (toupper(*sptr) == toupper(*pptr))
        {
            sptr++;
            pptr++;

            /* if end of pattern then pattern was found */

            if ('\0' == *pptr)
                return (start);
        }
    }
    return (NULL);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent *Owner)
    : TForm(Owner)
{
    this->OnClose = FormClose;
    AircraftDBPathFileName = ExtractFilePath(ExtractFileDir(Application->ExeName)) + AnsiString("..\\AircraftDB\\") + AIRCRAFT_DATABASE_FILE;
    ARTCCBoundaryDataPathFileName = ExtractFilePath(ExtractFileDir(Application->ExeName)) + AnsiString("..\\ARTCC_Boundary_Data\\") + ARTCC_BOUNDARY_FILE;
    BigQueryPath = ExtractFilePath(ExtractFileDir(Application->ExeName)) + AnsiString("..\\BigQuery\\");
    BigQueryPythonScript = BigQueryPath + AnsiString(BIG_QUERY_RUN_FILENAME);
    DeleteFilesWithExtension(BigQueryPath, "csv");
    BigQueryLogFileName = BigQueryPath + "BigQuery.log";
    DeleteFileA(BigQueryLogFileName.c_str());
    CurrentSpriteImage = 0;
    InitDecodeRawADS_B();
    RecordRawStream = NULL;
    PlayBackRawStream = NULL;
    TrackHook.Valid_CC = false;
    TrackHook.Valid_CPA = false;

    HashTable = ght_create(50000);

    if (!HashTable)
    {
        throw Sysutils::Exception("Create Hash Failed");
    }
    ght_set_rehash(HashTable, TRUE);

    AreaTemp = NULL;
    Areas = new TList;

    MouseDown = false;

    MapCenterLat = MAP_CENTER_LAT;
    MapCenterLon = MAP_CENTER_LON;

    LoadMapFromInternet = false;
    CheckBoxUpdateMapTiles->Checked = false;
    MapComboBox->ItemIndex = GoogleMaps;
    // MapComboBox->ItemIndex=SkyVector_VFR;
    // MapComboBox->ItemIndex=SkyVector_IFR_Low;
    // MapComboBox->ItemIndex=SkyVector_IFR_High;
    LoadMap(MapComboBox->ItemIndex);

    GetEarthView()->m_Eye.h /= pow(1.3,18);//pow(1.3,43);
    SetMapCenter(GetEarthView()->m_Eye.x, GetEarthView()->m_Eye.y);
    TimeToGoTrackBar->Position = 120;
    BigQueryCSV = NULL;
    BigQueryRowCount = 0;
    BigQueryFileCount = 0;
    UnregisteredAircraftCount = 0;

    // std::string tmpsign = "KAL123";

    // Initialize tower texture
    towerTextureID = 0;
    towerTextureLoaded = false;
    printf("init complete\n");
}
//---------------------------------------------------------------------------
__fastcall TForm1::~TForm1()
{
    printf("[Form] TForm1 destructor called.\n");
    Timer1->Enabled = false;
    Timer2->Enabled = false;
    if (currentMapProvider) {
        delete currentMapProvider;
        currentMapProvider = nullptr;
    }

    if (AircraftDB) {
        printf("[Form] Deleting AircraftDB...\n");
        delete AircraftDB;
        AircraftDB = nullptr;
        printf("[Form] AircraftDB deleted.\n");
    }
    printf("[Form] TForm1 destructor completed.\n");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SetMapCenter(double &x, double &y)
{
    double siny;
    x = (MapCenterLon + 0.0) / 360.0;
    siny = sin((MapCenterLat * M_PI) / 180.0);
    siny = fmin(fmax(siny, -0.9999), 0.9999);
    y = (log((1 + siny) / (1 - siny)) / (4 * M_PI));
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayInit(TObject *Sender)
{
    glViewport(0, 0, (GLsizei)ObjectDisplay->Width, (GLsizei)ObjectDisplay->Height);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_STIPPLE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    NumSpriteImages = MakeAirplaneImages();
    MakeAirTrackFriend();
    MakeAirTrackHostile();
    MakeAirTrackUnknown();
    MakePoint();
    MakeTrackHook();
    currentMapProvider->Resize(ObjectDisplay->Width,ObjectDisplay->Height);
    glPushAttrib(GL_LINE_BIT);
    glPopAttrib();

    LoadTowerTexture();
    
    
    printf("OpenGL Version %s\n", glGetString(GL_VERSION));

    // ----- Metadata database initialization -----
    // using local files to initialize
    AirportMgr.LoadAirportFromFile();
    RouteMgr.LoadRouteFromFile();

    // invoke thread to handle periodic updates
    RouteMgr.startUpdateMonitor();

    // load DB asynchronously in background
    AircraftDB = new TAircraftDB();
    AircraftDB->LoadDatabaseAsync(AircraftDBPathFileName);

    std::unique_ptr<MilitaryFilter> militaryfilter = std::make_unique<MilitaryFilter>();
    DefaultFilter.addFilter("military", std::move(militaryfilter));
    // initialize message processor thread
    if (!msgProcThread)
    {
        msgProcThread = new TMessageProcessorThread(true); // start in suspended state
        msgProcThread->FreeOnTerminate = true; // Automatically free the thread object after execution
        msgProcThread->Start();
    }
    else
    {
        printf("[Form] Message processor thread already exists.\n");
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ObjectDisplayResize(TObject *Sender)
{
    double Value;
    // ObjectDisplay->Width=ObjectDisplay->Height;
    glViewport(0, 0, (GLsizei)ObjectDisplay->Width, (GLsizei)ObjectDisplay->Height);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_LINE_STIPPLE);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    currentMapProvider->Resize(ObjectDisplay->Width,ObjectDisplay->Height);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayPaint(TObject *Sender)
{

    if (DrawMap->Checked)
        glClearColor(0.0, 0.0, 0.0, 0.0);
    else
        glClearColor(BG_INTENSITY, BG_INTENSITY, BG_INTENSITY, 0.0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    GetEarthView()->Animate();
    GetEarthView()->Render(DrawMap->Checked);
    GetTileManager()->Cleanup();
    Mw1 = Map_w[1].x - Map_w[0].x;
    Mw2 = Map_v[1].x - Map_v[0].x;
    Mh1 = Map_w[1].y - Map_w[0].y;
    Mh2 = Map_v[3].y - Map_v[0].y;

    xf = Mw1 / Mw2;
    yf = Mh1 / Mh2;

    DrawObjects();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer1Timer(TObject *Sender)
{
    __int64 CurrentTime;

    CurrentTime = GetCurrentTimeInMsec();
    SystemTime->Caption = TimeToChar(CurrentTime);

    UpdateUnregisteredCount();

    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
static void SetGLColor4f(float r, float g, float b, float a, float colorEps = 1e-3f) {
    static float lastR = -1.0f, lastG = -1.0f, lastB = -1.0f, lastA = -1.0f;
    if (fabs(r - lastR) > colorEps || fabs(g - lastG) > colorEps || fabs(b - lastB) > colorEps || fabs(a - lastA) > colorEps) {
        glColor4f(r, g, b, a);
        lastR = r; lastG = g; lastB = b; lastA = a;
    }
}

void __fastcall TForm1::DrawObjects(void)
{
    EXECUTION_TIMER(drawingTime);

    double ScrX, ScrY;
    int ViewableAircraft = 0;

    // --- drawing center crosshair ---
    // below 5 lines are anti-aliasing options which can be turned off for performance
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // line settings
    glLineWidth(3.0);
    glPointSize(4.0);
    glColor4f(1.0, 1.0, 1.0, 1.0);

    // XY position
    LatLon2XY(MapCenterLat, MapCenterLon, ScrX, ScrY);

    // draw crosshair
    glBegin(GL_LINE_STRIP);
    glVertex2f(ScrX - 20.0, ScrY);
    glVertex2f(ScrX + 20.0, ScrY);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex2f(ScrX, ScrY - 20.0);
    glVertex2f(ScrX, ScrY + 20.0);
    glEnd();

    uint32_t *Key;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data, *DataCPA;

    DWORD i, j, Count;

    // --- drawing area of interest (polygon) ---
    DrawAirportsBatch();
    TArea screenBoundsArea = getScreenBoundsAsArea();
    
    std::unique_ptr<ScreenFilter> screenfilter = std::make_unique<ScreenFilter>();


    screenfilter->updateFilterArea(screenBoundsArea, "screen_bounds");
    screenfilter->setAndFilter(true);

    DefaultFilter.addFilter("screen_bounds", std::move(screenfilter));
    DefaultFilter.activateFilter("screen_bounds");
    // AreaFilter.filterAircraftPosition(40.0, -80.0);
    if (AreaTemp)
    {
        glPointSize(3.0);
        for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
            LatLon2XY(AreaTemp->Points[i][1], AreaTemp->Points[i][0],
                      AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);
        // 점과 선을 각각 한 번의 draw call로 묶음
        glBegin(GL_POINTS);
        for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
            glVertex2f(AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);
        glEnd();
        glBegin(GL_LINE_STRIP);
        for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
            glVertex2f(AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);
        glEnd();
    }
    Count = Areas->Count;
    for (i = 0; i < Count; i++)
    {
        TArea *Area = (TArea *)Areas->Items[i];
        TMultiColor MC;
        MC.Rgb = ColorToRGB(Area->Color);
        if (Area->Selected)
        {
            glLineWidth(4.0);
            glPushAttrib(GL_LINE_BIT);
            glLineStipple(3, 0xAAAA);
        }
        // 폴리곤 라인 전체를 한 번에 그리기
        glColor4f(MC.Red / 255.0, MC.Green / 255.0, MC.Blue / 255.0, 1.0);
        glBegin(GL_LINE_LOOP);
        for (j = 0; j < Area->NumPoints; j++)
        {
            LatLon2XY(Area->Points[j][1], Area->Points[j][0], ScrX, ScrY);
            glVertex2f(ScrX, ScrY);
        }
        glEnd();
        if (Area->Selected)
        {
            glPopAttrib();
            glLineWidth(2.0);
        }
        // 폴리곤 면 전체를 한 번에 그리기
        glColor4f(MC.Red / 255.0, MC.Green / 255.0, MC.Blue / 255.0, 0.4);
        for (j = 0; j < Area->NumPoints; j++)
        {
            LatLon2XY(Area->Points[j][1], Area->Points[j][0],
                      Area->PointsAdj[j][0], Area->PointsAdj[j][1]);
        }
        TTriangles *Tri = Area->Triangles;
        if (Tri) {
            glBegin(GL_TRIANGLES);
            while (Tri)
            {
                glVertex2f(Area->PointsAdj[Tri->indexList[0]][0], Area->PointsAdj[Tri->indexList[0]][1]);
                glVertex2f(Area->PointsAdj[Tri->indexList[1]][0], Area->PointsAdj[Tri->indexList[1]][1]);
                glVertex2f(Area->PointsAdj[Tri->indexList[2]][0], Area->PointsAdj[Tri->indexList[2]][1]);
                Tri = Tri->next;
            }
            glEnd();
        }
    }
    int filtered = 0;
    int inscreen = 0;
    // --- drawing aircrafts ---
    AircraftCountLabel->Caption = IntToStr((int)ght_size(HashTable));
    
    // Performance measurement for aircraft processing loop
    static int loopPerfCounter = 0;
    static double totalLoopTime = 0.0;
    auto loopStart = std::chrono::high_resolution_clock::now();

    // Get zoom level and Define zoom threshold constants for better performance
    float zoomLevel = getCurrentZoomLevel();
    const float ZOOM_THRESHOLD_FOR_MIDDLE = 1.01f;
    const float ZOOM_THRESHOLD_FOR_HIGH = 1.9f;
    const float colorEps = 1e-3f;
	// Measure block execution time
	static double totalBlockTime = 0.0;
	static int blockCounter = 0;
    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if (Data->HaveLatLon)
        {
            if (HideUnregisteredCheckBox->Checked && !AircraftDB->aircraft_is_registered(Data->ICAO)) {
                continue;
            }

            ViewableAircraft++;
            if(DefaultFilter.filterAircraft(*Data))
            {
                inscreen++; // Skip aircraft if it does not match the filter criteria
            } else
            {
                continue; // Reset filtered count if aircraft matches the filter
            }
            
            // Performance measurement for filter block
            auto blockStart = std::chrono::high_resolution_clock::now();
            
            if(AreaFilter.filterAircraft(*Data))
            {
                filtered++; // Skip aircraft if it does not match the filter criteria
            } else
            {
                continue; // Reset filtered count if aircraft matches the filter
            }
			auto blockEnd = std::chrono::high_resolution_clock::now();
			auto blockDuration = std::chrono::duration_cast<std::chrono::microseconds>(blockEnd - blockStart);

			totalBlockTime += blockDuration.count() / 1000.0; // Convert to milliseconds
			blockCounter++;
            LatLon2XY(Data->Latitude, Data->Longitude, ScrX, ScrY);
            // 화면 밖이면 continue (예: -50~Width+50, -50~Height+50 범위만)
            if (ScrX < -50 || ScrX > ObjectDisplay->Width + 50 || ScrY < -50 || ScrY > ObjectDisplay->Height + 50)
                continue;

            float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
            if (!AircraftDB->aircraft_is_registered(Data->ICAO)) {
                r = g = b = 0.5f; // gray
            } else if (Data->HaveSpeedAndHeading) {
                r = 0.0f; g = 1.0f; b = 0.0f; // green
                if (Data->IsHelicopter) {
                    r = 1.0f; g = 0.41f; b = 0.71f; // pink
                }
                if (Data->IsMilitary) {
                    r = 0.0f; g = 0.0f; b = 1.0f; // blue
                }
            } else {
                Data->Heading = 0.0;
                r = 1.0f; g = 0.0f; b = 0.0f; // red
            }
            SetGLColor4f(r, g, b, a, colorEps);

            if (zoomLevel > ZOOM_THRESHOLD_FOR_MIDDLE) {
                DrawAirplaneImage(ScrX, ScrY, 0.8, Data->Heading, Data->SpriteImage);   // Draw airplane image. scale is changed from 1.5 to 0.8
            } else {
                DrawAirplaneImage(ScrX, ScrY, 0.5, Data->Heading, Data->SpriteImage);   // Draw airplane image. scale is changed to smaller
            }

            // heading line
            if ((Data->HaveSpeedAndHeading) && (TimeToGoCheckBox->State == cbChecked))
            {
                double lat, lon, az;
                if (VDirect(Data->Latitude, Data->Longitude,
                            Data->Heading, Data->Speed / 3060.0 * TimeToGoTrackBar->Position, &lat, &lon, &az) == OKNOERROR)
                {
                    double ScrX2, ScrY2;
                    LatLon2XY(lat, lon, ScrX2, ScrY2);
                    SetGLColor4f(1.0f, 1.0f, 0.0f, 1.0f, colorEps); // yellow
                    glBegin(GL_LINE_STRIP);
                    glVertex2f(ScrX, ScrY);
                    glVertex2f(ScrX2, ScrY2);
                    glEnd();
                }
            }

            // Draw ICAO text
            if (zoomLevel > ZOOM_THRESHOLD_FOR_HIGH) {
                // ICAO code text besides the aircraft with yellow color and black outline for better readability
                // Draw black outline for ICAO text
                SetGLColor4f(0.0, 0.0, 0.0, 1.0, colorEps);  // black outline
                glRasterPos2i(ScrX + 39, ScrY - 10);
                ObjectDisplay->Draw2DTextDefault(Data->HexAddr);
                glRasterPos2i(ScrX + 41, ScrY - 10);
                ObjectDisplay->Draw2DTextDefault(Data->HexAddr);
                // Draw main ICAO text in bright yellow
                SetGLColor4f(1.0, 1.0, 0.0, 1.0, colorEps);  // bright yellow color for ICAO code
                glRasterPos2i(ScrX + 40, ScrY - 10);
                ObjectDisplay->Draw2DTextDefault(Data->HexAddr);
            } else if(zoomLevel > ZOOM_THRESHOLD_FOR_MIDDLE && zoomLevel <= ZOOM_THRESHOLD_FOR_HIGH) {
                // Draw main ICAO text in bright yellow
                SetGLColor4f(1.0, 1.0, 0.0, 1.0, colorEps);  // bright yellow color for ICAO code
                glRasterPos2i(ScrX + 40, ScrY - 10);
                ObjectDisplay->Draw2DTextDefault(Data->HexAddr);
            } 
            else {
            }            
            
            // Draw aircraft info text
            if (zoomLevel > ZOOM_THRESHOLD_FOR_HIGH) {
                AnsiString callsignHeadAltSpeedText = "";
                
                // Add callsign if available
                if (Data->HaveFlightNum && strlen(Data->FlightNum) > 0) {
                    AnsiString flightNum = AnsiString(Data->FlightNum);
                    if (!flightNum.IsEmpty()) {
                        callsignHeadAltSpeedText += flightNum;
                    } else {
                        callsignHeadAltSpeedText += "--";
                    }
                } else {
                    callsignHeadAltSpeedText += "--";  // placeholder if no callsign is available
                }
                callsignHeadAltSpeedText += "/";
                
                // Add heading if available
                if (Data->Heading > 0) {
                    callsignHeadAltSpeedText += FloatToStrF((double)Data->Heading, ffFixed, 6, 0);
                } else {
                    callsignHeadAltSpeedText += "--";
                }
                callsignHeadAltSpeedText += "/";
                
                if (Data->Altitude > 0) {
                    int altitudeInThousands = (int)(Data->Altitude / 1000);
                    callsignHeadAltSpeedText += IntToStr(altitudeInThousands);
                } else {
                    callsignHeadAltSpeedText += "--";
                }
                callsignHeadAltSpeedText += "k/";
    
                if (Data->Speed > 0) {
                    callsignHeadAltSpeedText += FloatToStrF((double)Data->Speed, ffFixed, 6, 0);
                } else {
                    callsignHeadAltSpeedText += "--";
                }
                callsignHeadAltSpeedText += "kts";
    
                // Draw the aircraft info text with callsign, heading, altitude and speed 
                // Draw black outline for aircraft info text
                SetGLColor4f(0.0, 0.0, 0.0, 1.0, colorEps);  // black outline
                glRasterPos2i(ScrX + 39, ScrY - 25);
                ObjectDisplay->Draw2DTextAdditional(callsignHeadAltSpeedText);
                glRasterPos2i(ScrX + 41, ScrY - 25);
                ObjectDisplay->Draw2DTextAdditional(callsignHeadAltSpeedText);
                // Draw main aircraft info text
                SetGLColor4f(0.5, 1.0, 0.0, 1.0, colorEps);  // lime green for high visibility
                glRasterPos2i(ScrX + 40, ScrY - 25);
                ObjectDisplay->Draw2DTextAdditional(callsignHeadAltSpeedText);
    
                // track age information below the ICAO code
                SetGLColor4f(1.0, 0.0, 0.0, 1.0, colorEps);  // red
                glRasterPos2i(ScrX + 40, ScrY - 40);    // TODO: location should be adjusted based on font size setting from the dfm file
                TimeDifferenceInSecToChar(Data->LastSeen, Data->TimeElapsedInSec, sizeof(Data->TimeElapsedInSec));
                ObjectDisplay->Draw2DTextAdditional(Data->TimeElapsedInSec + AnsiString(" seconds ago"));
            }

        }
    }
    static int lopcont= 0;
    if (lopcont % 20 == 0) {
        double avgBlockTime = totalBlockTime / blockCounter;
        std::cout << "Filter block avg time: " << avgBlockTime << " ms (total: " << totalBlockTime 
                    << " ms over " << blockCounter << " iterations)" << std::endl;
    }
    lopcont++;

    blockCounter = 0;
    totalBlockTime = 0.0;
    // End performance measurement for aircraft processing loop
    auto loopEnd = std::chrono::high_resolution_clock::now();
    auto loopDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loopEnd - loopStart);
    totalLoopTime += loopDuration.count();
    loopPerfCounter++;
    
    // Output performance data every 100 measurements
    if (loopPerfCounter >= 100) {
        double avgLoopTime = totalLoopTime / loopPerfCounter;
        std::cout << "Aircraft processing loop avg time: " << avgLoopTime << " milliseconds (over " << loopPerfCounter << " iterations)" << std::endl;
        loopPerfCounter = 0;
        totalLoopTime = 0.0;
    }
	    // Output filtered aircraft count every 300 calls
    static int outputCounter = 0;
    outputCounter++;
    if (outputCounter >= 300) {
        std::cout << "Filtered aircraft count: " << filtered << std::endl;
        outputCounter = 0;
	}

    // --- handle hooked aircrafts ----
    ViewableAircraftCountLabel->Caption = ViewableAircraft;
    if (TrackHook.Valid_CC)
    {
        Data = (TADS_B_Aircraft *)ght_get(HashTable, sizeof(TrackHook.ICAO_CC), (void *)&TrackHook.ICAO_CC);
        if (Data)
        {
            // update side bar text for hooked aircraft
            ICAOLabel->Caption = Data->HexAddr;
            if (Data->HaveFlightNum)
                FlightNumLabel->Caption = Data->FlightNum;
            else
                FlightNumLabel->Caption = "N/A";
            if (Data->HaveLatLon)
            {
                CLatLabel->Caption = DMS::DegreesMinutesSecondsLat(Data->Latitude).c_str();
                CLonLabel->Caption = DMS::DegreesMinutesSecondsLon(Data->Longitude).c_str();
            }
            else
            {
                CLatLabel->Caption = "N/A";
                CLonLabel->Caption = "N/A";
            }
            if (Data->HaveSpeedAndHeading)
            {
                SpdLabel->Caption = FloatToStrF(Data->Speed, ffFixed, 12, 2) + " KTS  VRATE:" + FloatToStrF(Data->VerticalRate, ffFixed, 12, 2);
                HdgLabel->Caption = FloatToStrF(Data->Heading, ffFixed, 12, 2) + " DEG";
            }
            else
            {
                SpdLabel->Caption = "N/A";
                HdgLabel->Caption = "N/A";
            }
            if (Data->Altitude)
                AltLabel->Caption = FloatToStrF(Data->Altitude, ffFixed, 12, 2) + " FT";
            else
                AltLabel->Caption = "N/A";

            MsgCntLabel->Caption = "Raw: " + IntToStr((int)Data->NumMessagesRaw) + " SBS: " + IntToStr((int)Data->NumMessagesSBS);
            TrkLastUpdateTimeLabel->Caption = TimeToChar(Data->LastSeen);

            // draw route line
            std::string callSign = Data->FlightNum;   // CallSign
            const Route* route = RouteMgr.getRouteByCallSign(callSign);
            if(route != nullptr)
            {
                const std::vector<std::string>& routes = route->getWaypoints();

                if(!routes.empty())
                {
                    // std::cout << "Route found for call sign: " << route->getWaypointStr() << std::endl;
                    // std::cout << "Route found for call sign: " << routes.size() << std::endl;
                    std::string departureAirportCode = routes.front();
                    std::string arrivalAirportCode = routes.back();
        
                    const Airport* departureAirport = AirportMgr.getAirportByCode(departureAirportCode);
                    // std::cout << "Departure Airport: " << departureAirport->getName() << std::endl;
                    // std::cout << "Departure Airport Code: " << departureAirportCode << std::endl;
                    // std::cout << "Departure Airport Latitude: " << departureAirport->getLatitude() << std::endl;
                    // std::cout << "Departure Airport Longitude: " << departureAirport->getLongitude() << std::endl;
                    const Airport* arrivalAirport = AirportMgr.getAirportByCode(arrivalAirportCode);
                    // std::cout << "Arrival Airport: " << arrivalAirport->getName() << std::endl;
                    // std::cout << "Arrival Airport Code: " << arrivalAirportCode << std::endl;
                    // std::cout << "Arrival Airport Latitude: " << arrivalAirport->getLatitude() << std::endl;
                    // std::cout << "Arrival Airport Longitude: " << arrivalAirport->getLongitude() << std::endl;

                    // latlon to XY
                    LatLon2XY(departureAirport->getLatitude(), departureAirport->getLongitude(), ScrX, ScrY);
                    double arrX, arrY;
                    LatLon2XY(arrivalAirport->getLatitude(), arrivalAirport->getLongitude(), arrX, arrY);

                    // std::cout << "Departure Airport XY: (" << ScrX << ", " << ScrY << ")" << std::endl;
                    // std::cout << "Arrival Airport XY: (" << arrX << ", " << arrY << ")" << std::endl;
                    // std::cout << (GLsizei)ObjectDisplay->Width << ", " << (GLsizei)ObjectDisplay->Height << std::endl;

                    // TODO: whether both points are within the screen bounds


                    std::vector<std::pair<double, double>> intersections = cohenSutherlandClip(ScrX, ScrY, arrX, arrY, 0, 0, (GLsizei)ObjectDisplay->Width, (GLsizei)ObjectDisplay->Height);

                    // 결과 출력
                    // std::cout << "Intersection points:\n";
                    // for (const auto& point : intersections) {
                    //     std::cout << "(" << point.first << ", " << point.second << ")\n";
                    // }

                    glColor4f(0.0, 1.0, 1.0, 1.0); // Cyan color for route line
                    int numIntersections = intersections.size();
                    if(numIntersections == 2) {
                        // Draw the route line between the two intersection points
                        DrawLeader(intersections[0].first, intersections[0].second, intersections[1].first, intersections[1].second);
                    } else if(numIntersections >= 1) {
                        if(computeRegionCode(ScrX, ScrY, 0, 0, (GLsizei)ObjectDisplay->Width, ObjectDisplay->Height) == 0) {
                            DrawLeader(intersections[0].first, intersections[0].second, ScrX, ScrY);
                        } else {
                            DrawLeader(intersections[0].first, intersections[0].second, arrX, arrY);
                        }
                    } else {
                        DrawLeader(ScrX, ScrY, arrX, arrY);
                    }
                } else {
                    std::cout << "No route found for call sign: " << callSign << std::endl;
                }
            }

            // draw circle for hooked aircraft
            glColor4f(1.0, 0.0, 0.0, 1.0);
            LatLon2XY(Data->Latitude, Data->Longitude, ScrX, ScrY);
            DrawTrackHook(ScrX, ScrY);
        }

        else
        {
            TrackHook.Valid_CC = false;
            ICAOLabel->Caption = "N/A";
            FlightNumLabel->Caption = "N/A";
            CLatLabel->Caption = "N/A";
            CLonLabel->Caption = "N/A";
            SpdLabel->Caption = "N/A";
            HdgLabel->Caption = "N/A";
            AltLabel->Caption = "N/A";
            MsgCntLabel->Caption = "N/A";
            TrkLastUpdateTimeLabel->Caption = "N/A";
        }
    }

    // --- draw CPA lines ----
    if (TrackHook.Valid_CPA)
    {
        bool CpaDataIsValid = false;
        DataCPA = (TADS_B_Aircraft *)ght_get(HashTable, sizeof(TrackHook.ICAO_CPA), (void *)&TrackHook.ICAO_CPA);
        if ((DataCPA) && (TrackHook.Valid_CC))
        {

            double tcpa, cpa_distance_nm, vertical_cpa;
            double lat1, lon1, lat2, lon2, junk;
            if (computeCPA(Data->Latitude, Data->Longitude, Data->Altitude,
                           Data->Speed, Data->Heading,
                           DataCPA->Latitude, DataCPA->Longitude, DataCPA->Altitude,
                           DataCPA->Speed, DataCPA->Heading,
                           tcpa, cpa_distance_nm, vertical_cpa))
            {
                if (VDirect(Data->Latitude, Data->Longitude,
                            Data->Heading, Data->Speed / 3600.0 * tcpa, &lat1, &lon1, &junk) == OKNOERROR)
                {
                    if (VDirect(DataCPA->Latitude, DataCPA->Longitude,
                                DataCPA->Heading, DataCPA->Speed / 3600.0 * tcpa, &lat2, &lon2, &junk) == OKNOERROR)
                    {
                        glColor4f(0.0, 1.0, 0.0, 1.0);
                        glBegin(GL_LINE_STRIP);
                        LatLon2XY(Data->Latitude, Data->Longitude, ScrX, ScrY);
                        glVertex2f(ScrX, ScrY);
                        LatLon2XY(lat1, lon1, ScrX, ScrY);
                        glVertex2f(ScrX, ScrY);
                        glEnd();
                        glBegin(GL_LINE_STRIP);
                        LatLon2XY(DataCPA->Latitude, DataCPA->Longitude, ScrX, ScrY);
                        glVertex2f(ScrX, ScrY);
                        LatLon2XY(lat2, lon2, ScrX, ScrY);
                        glVertex2f(ScrX, ScrY);
                        glEnd();
                        glColor4f(1.0, 0.0, 0.0, 1.0);
                        glBegin(GL_LINE_STRIP);
                        LatLon2XY(lat1, lon1, ScrX, ScrY);
                        glVertex2f(ScrX, ScrY);
                        LatLon2XY(lat2, lon2, ScrX, ScrY);
                        glVertex2f(ScrX, ScrY);
                        glEnd();
                        CpaTimeValue->Caption = TimeToChar(tcpa * 1000);
                        CpaDistanceValue->Caption = FloatToStrF(cpa_distance_nm, ffFixed, 10, 2) + " NM VDIST: " + IntToStr((int)vertical_cpa) + " FT";
                        CpaDataIsValid = true;
                    }
                }
            }
        }
        if (!CpaDataIsValid)
        {
            TrackHook.Valid_CPA = false;
            CpaTimeValue->Caption = "None";
            CpaDistanceValue->Caption = "None";
        }
    }
    EXECUTION_TIMER_ELAPSED(elapsed, drawingTime);
    LOG("Viewable aircraft: " + std::to_string(ViewableAircraft) + " Elapsed: " + std::to_string(elapsed) + "ms");
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayMouseDown(TObject *Sender,
                                               TMouseButton Button, TShiftState Shift, int X, int Y)
{

    if (Button == mbLeft)
    {
        if (Shift.Contains(ssCtrl))
        {
        }
        else
        {
            g_MouseLeftDownX = X;
            g_MouseLeftDownY = Y;
            g_MouseDownMask |= LEFT_MOUSE_DOWN;
            GetEarthView()->StartDrag(X, Y, NAV_DRAG_PAN);
        }
    }
    else if (Button == mbRight)
    {
        if (AreaTemp)
        {
            if (AreaTemp->NumPoints < MAX_AREA_POINTS)
            {
                AddPoint(X, Y);
            }
            else
                ShowMessage("Max Area Points Reached");
        }
        else
        {
            if (Shift.Contains(ssCtrl))
                HookTrack(X, Y, true);
            else
                HookTrack(X, Y, false);
        }
    }

    else if (Button == mbMiddle)
        ResetXYOffset();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ObjectDisplayMouseUp(TObject *Sender,
                                             TMouseButton Button, TShiftState Shift, int X, int Y)
{
    if (Button == mbLeft)
        g_MouseDownMask &= ~LEFT_MOUSE_DOWN;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ObjectDisplayMouseMove(TObject *Sender,
                                               TShiftState Shift, int X, int Y)
{
    double VLat, VLon;
    int i;
    if (XY2LatLon2(X, Y, VLat, VLon) == 0)
    {
        pfVec3 Point;
        Lat->Caption = DMS::DegreesMinutesSecondsLat(VLat).c_str();
        Lon->Caption = DMS::DegreesMinutesSecondsLon(VLon).c_str();
        Point[0] = VLon;
        Point[1] = VLat;
        Point[2] = 0.0;

        for (i = 0; i < Areas->Count; i++)
        {
            TArea *Area = (TArea *)Areas->Items[i];
            if (PointInPolygon(Area->Points, Area->NumPoints, Point))
            {
#if 0
		  MsgLog->Lines->Add("In Polygon "+ Area->Name);
#endif
            }
        }
    }

    if (g_MouseDownMask & LEFT_MOUSE_DOWN)
    {
        GetEarthView()->Drag(g_MouseLeftDownX, g_MouseLeftDownY, X,Y, NAV_DRAG_PAN);
        ObjectDisplay->Repaint();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ResetXYOffset(void)
{
    SetMapCenter(GetEarthView()->m_Eye.x, GetEarthView()->m_Eye.y);
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Exit1Click(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AddPoint(int X, int Y)
{
    double Lat, Lon;

    if (XY2LatLon2(X, Y, Lat, Lon) == 0)
    {

        AreaTemp->Points[AreaTemp->NumPoints][1] = Lat;
        AreaTemp->Points[AreaTemp->NumPoints][0] = Lon;
        AreaTemp->Points[AreaTemp->NumPoints][2] = 0.0;
        AreaTemp->NumPoints++;
        ObjectDisplay->Repaint();
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::HookTrack(int X, int Y, bool CPA_Hook)  // handle track hooking
{
    double VLat, VLon, dlat, dlon, Range;
    uint32_t *Key;

    uint32_t Current_ICAO;
    double MinRange;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data;

    if (XY2LatLon2(X, Y, VLat, VLon) == -1) {
        return;
    }

    MinRange = 16.0;

    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if (Data->HaveLatLon)
        {
            dlat = VLat - Data->Latitude;
            dlon = VLon - Data->Longitude;
            Range = sqrt(dlat * dlat + dlon * dlon);
            if (Range < MinRange)
            {
                Current_ICAO = Data->ICAO;
                MinRange = Range;
            }
        }
    }
    if (MinRange < 0.2) // close enough to consider it is hooked
    {
        TADS_B_Aircraft *ADS_B_Aircraft = (TADS_B_Aircraft *)
            ght_get(HashTable, sizeof(Current_ICAO),
                    &Current_ICAO);
        if (ADS_B_Aircraft) // aircraft data is in hash table
        {
            if (!CPA_Hook)  // selection of the first aircraft
            {
                TrackHook.Valid_CC = true;  // valid track hook
                TrackHook.ICAO_CC = ADS_B_Aircraft->ICAO;
                const TAircraftData* acData = AircraftDB->GetAircraftDBInfo(ADS_B_Aircraft->ICAO);
                if (acData) {
                    printf("%s\n\n", acData->toString().c_str());
                    RegNumLabel->Caption = acData->Registration.IsEmpty() ? "Unknown" : acData->Registration;
                    OperatorLabel->Caption = acData->OperatorName.IsEmpty() ? "Unknown" : acData->OperatorName;

                    // Get country information using ICAO address
                    const char* country = AircraftDB->GetCountry(ADS_B_Aircraft->ICAO, false);
                    if (country)
                        CountryLabel->Caption = country;
                    else
                        CountryLabel->Caption = "Unknown";

                    const TAircraftTypeInfo type = AircraftDB->GetAircraftType(ADS_B_Aircraft->ICAO);
                    TypeLabel->Caption = type.categoryName;

                    // Check if we have a valid flight number
                    if (ADS_B_Aircraft->HaveFlightNum && strlen(ADS_B_Aircraft->FlightNum) > 0) {
                        std::string callsign(ADS_B_Aircraft->FlightNum);
                        const Route* route = RouteMgr.getRouteByCallSign(callsign);

                        // Get waypoints and join them with "->" separator
                        std::vector<std::string> waypoints = route->getWaypoints();
                        if (!waypoints.empty()) {
                            std::string routeStr = "";
                            for (size_t i = 0; i < waypoints.size(); ++i) {
                                if (i > 0) {
                                    routeStr += "->";
                                }
                                routeStr += waypoints[i];
                            }
                            RouteLabel->Caption = routeStr.c_str();
                        } else {
                            RouteLabel->Caption = "Unknown";
                        }
                    } else {
                        RouteLabel->Caption = "Unknown";
                    }
                } else {
                    printf("No AircraftDB info\n\n");
                    RegNumLabel->Caption = "N/A";
                    OperatorLabel->Caption = "N/A";
                    CountryLabel->Caption = "N/A";
                    TypeLabel->Caption = "N/A";
                    RouteLabel->Caption = "N/A";
                }
            }
            else    // selection of the second aircarft
            {
                TrackHook.Valid_CPA = true;
                TrackHook.ICAO_CPA = ADS_B_Aircraft->ICAO;
            };
        }
    }
    else
    {
        if (!CPA_Hook)
        {
            TrackHook.Valid_CC = false;
            ICAOLabel->Caption = "N/A";
            FlightNumLabel->Caption = "N/A";
            CLatLabel->Caption = "N/A";
            CLonLabel->Caption = "N/A";
            SpdLabel->Caption = "N/A";
            HdgLabel->Caption = "N/A";
            AltLabel->Caption = "N/A";
            MsgCntLabel->Caption = "N/A";
            TrkLastUpdateTimeLabel->Caption = "N/A";
        }
        else
        {
            TrackHook.Valid_CPA = false;
            CpaTimeValue->Caption = "None";
            CpaDistanceValue->Caption = "None";
        }
    }
}
//---------------------------------------------------------------------------
// convert lat/lon to x/y
void __fastcall TForm1::LatLon2XY(double lat, double lon, double &x, double &y)
{
    x = (Map_v[1].x - ((Map_w[1].x - (lon / 360.0)) / xf));
    y = Map_v[3].y - (Map_w[1].y / yf) + (asinh(tan(lat * M_PI / 180.0)) / (2 * M_PI * yf));
}
//---------------------------------------------------------------------------
// convert x/y to lat/lon
int __fastcall TForm1::XY2LatLon2(int x, int y, double &lat, double &lon)
{
    // double Lat, Lon, dlat, dlon, Range;
    int X1, Y1;

    Y1 = (ObjectDisplay->Height - 1) - y;
    X1 = x;

    if ((X1 < Map_v[0].x) || (X1 > Map_v[1].x) ||
        (Y1 < Map_v[0].y) || (Y1 > Map_v[3].y))
        return -1;

    lat = atan(sinh(M_PI * (2 * (Map_w[1].y - (yf * (Map_v[3].y - Y1)))))) * (180.0 / M_PI);
    lon = (Map_w[1].x - (xf * (Map_v[1].x - X1))) * 360.0;

    return 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::ZoomInClick(TObject *Sender)
{
    GetEarthView()->SingleMovement(NAV_ZOOM_IN);
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ZoomOutClick(TObject *Sender)
{
    GetEarthView()->SingleMovement(NAV_ZOOM_OUT);

    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
// Internal helper to remove aircrafts matching predicate
void TForm1::PurgeInternal(std::function<bool(TADS_B_Aircraft*)> shouldPurge)
{
    uint32_t *Key;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data;
    void *p;

    // Collect keys to remove to avoid iterator invalidation
    std::vector<uint32_t> keysToRemove;

    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if (shouldPurge(Data))
            keysToRemove.push_back(*Key);
    }

    for (auto icao : keysToRemove)
    {
        Data = (TADS_B_Aircraft *)ght_get(HashTable, sizeof(icao), &icao);
        p = ght_remove(HashTable, sizeof(icao), &icao);
        if (!p)
            ShowMessage("Removing the current iterated entry failed! This is a BUG\n");
        delete Data;
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Purge(void)
{
    if (PurgeStale->Checked == false)
        return;

    __int64 CurrentTime = GetCurrentTimeInMsec();
    __int64 StaleTimeInMs = CSpinStaleTime->Value * 1000;

    auto shouldPurge = [CurrentTime, StaleTimeInMs](TADS_B_Aircraft* Data) {
        return (CurrentTime - Data->LastSeen) >= StaleTimeInMs;
    };

    PurgeInternal(shouldPurge);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PurgeButtonClick(TObject *Sender)
{
    auto alwaysPurge = [](TADS_B_Aircraft*) { return true; };
    PurgeInternal(alwaysPurge);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer2Timer(TObject *Sender)
{
    Purge();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::InsertClick(TObject *Sender)
{
    Insert->Enabled = false;
    LoadARTCCBoundaries1->Enabled = false;
    Complete->Enabled = true;
    Cancel->Enabled = true;
    // Delete->Enabled=false;

    AreaTemp = new TArea;
    AreaTemp->NumPoints = 0;
    AreaTemp->Name = "";
    AreaTemp->Selected = false;
    AreaTemp->Triangles = NULL;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CancelClick(TObject *Sender)
{
    TArea *Temp;
    Temp = AreaTemp;
    AreaTemp = NULL;
    delete Temp;
    Insert->Enabled = true;
    Complete->Enabled = false;
    Cancel->Enabled = false;
    LoadARTCCBoundaries1->Enabled = true;
    // if (Areas->Count>0)  Delete->Enabled=true;
    // else   Delete->Enabled=false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CompleteClick(TObject *Sender)
{

    int or1 = orientation2D_Polygon(AreaTemp->Points, AreaTemp->NumPoints);
    if (or1 == 0)
    {
        ShowMessage("Degenerate Polygon");
        CancelClick(NULL);
        return;
    }
    if (or1 == CLOCKWISE)
    {
        DWORD i;

        memcpy(AreaTemp->PointsAdj, AreaTemp->Points, sizeof(AreaTemp->Points));
        for (i = 0; i < AreaTemp->NumPoints; i++)
        {
            memcpy(AreaTemp->Points[i],
                   AreaTemp->PointsAdj[AreaTemp->NumPoints - 1 - i], sizeof(pfVec3));
        }
    }
    if (checkComplex(AreaTemp->Points, AreaTemp->NumPoints))
    {
        ShowMessage("Polygon is Complex");
        CancelClick(NULL);
        return;
    }

    AreaConfirm->ShowDialog();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AreaListViewSelectItem(TObject *Sender, TListItem *Item,
                                               bool Selected)
{
    DWORD Count;
    TArea *AreaS = (TArea *)Item->Data;
    bool HaveSelected = false;
    Count = Areas->Count;
    for (unsigned int i = 0; i < Count; i++)
    {
        TArea *Area = (TArea *)Areas->Items[i];
        if (Area == AreaS)
        {
            if (Item->Selected)
            {
                Area->Selected = true;
                HaveSelected = true;
            }
            else
                Area->Selected = false;
        }
        else
            Area->Selected = false;
    }
    if (HaveSelected)
        Delete->Enabled = true;
    else
        Delete->Enabled = false;
    
    if (HaveSelected)
    {
        AreaFilter.allFiltersDeactivate();
        for (unsigned int i = 0; i < Count; i++)
        {
            TArea *Area = (TArea *)Areas->Items[i];
            if (Area->Selected)
            {   
                AreaFilter.activateFilter(AnsiStringToStdString(Area->Name));
            }
        }
    }
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DeleteClick(TObject *Sender)
{
    int i = 0;

    while (i < AreaListView->Items->Count)
    {
        if (AreaListView->Items->Item[i]->Selected)
        {
            TArea *Area;
            int Index;

            Area = (TArea *)AreaListView->Items->Item[i]->Data;
            std::cout << "Deleting area: " << Area->Name.c_str() << std::endl;
            AreaFilter.removeFilter(AnsiStringToStdString(Area->Name));
            AreaFilter.allFiltersActivate();
            for (Index = 0; Index < Areas->Count; Index++)
            {
                if (Area == Areas->Items[Index])
                {
                    Areas->Delete(Index);
                    AreaListView->Items->Item[i]->Delete();
                    TTriangles *Tri = Area->Triangles;
                    while (Tri)
                    {
                        TTriangles *temp = Tri;
                        Tri = Tri->next;
                        free(temp->indexList);
                        free(temp);
                    }
                    delete Area;
                    break;
                }
            }
        }
        else
        {
            ++i;
        }
    }
    // if (Areas->Count>0)  Delete->Enabled=true;
    // else   Delete->Enabled=false;

    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AreaListViewCustomDrawItem(TCustomListView *Sender,
                                                   TListItem *Item, TCustomDrawState State, bool &DefaultDraw)
{
    TRect R;
    int Left;
    AreaListView->Canvas->Brush->Color = AreaListView->Color;
    AreaListView->Canvas->Font->Color = AreaListView->Font->Color;
    R = Item->DisplayRect(drBounds);
    AreaListView->Canvas->FillRect(R);

    AreaListView->Canvas->TextWidth(Item->Caption);

    AreaListView->Canvas->TextOut(2, R.Top, Item->Caption);

    Left = AreaListView->Column[0]->Width;

    for (int i = 0; i < Item->SubItems->Count; i++)
    {
        R = Item->DisplayRect(drBounds);
        R.Left = R.Left + Left;
        TArea *Area = (TArea *)Item->Data;
        AreaListView->Canvas->Brush->Color = Area->Color;
        AreaListView->Canvas->FillRect(R);
    }

    if (Item->Selected)
    {
        R = Item->DisplayRect(drBounds);
        AreaListView->Canvas->DrawFocusRect(R);
    }
    DefaultDraw = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::DeleteAllAreas(void)
{
    int i = 0;

    while (AreaListView->Items->Count)
    {

        TArea *Area;
        int Index;

        Area = (TArea *)AreaListView->Items->Item[i]->Data;
        for (Index = 0; Index < Areas->Count; Index++)
        {
            if (Area == Areas->Items[Index])
            {
                Areas->Delete(Index);
                AreaListView->Items->Item[i]->Delete();
                TTriangles *Tri = Area->Triangles;
                while (Tri)
                {
                    TTriangles *temp = Tri;
                    Tri = Tri->next;
                    free(temp->indexList);
                    free(temp);
                }
                delete Area;
                break;
            }
        }
    }

    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormMouseWheel(TObject *Sender, TShiftState Shift,
                                       int WheelDelta, TPoint &MousePos, bool &Handled)
{
    if (WheelDelta > 0)
        GetEarthView()->SingleMovement(NAV_ZOOM_IN);
    else
        GetEarthView()->SingleMovement(NAV_ZOOM_OUT);
    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall TForm1::RawConnectButtonClick(TObject *Sender)
{
    IdTCPClientRaw->Host = RawIpAddress->Text;
    IdTCPClientRaw->Port = 30002;

    if ((RawConnectButton->Caption == "Raw Connect") && (Sender != NULL))
    {
        #ifdef ERROR_HANDLING_ENABLED
        try {
            if (mPIErrorMonitorThread != NULL) {
                mPIErrorMonitorThread->Terminate();
                // delete mPIErrorMonitorThread;
                // mPIErrorMonitorThread = NULL;
            }
        }
        catch (const EIdException &e) {
            std::cout << "Error while setting up ssh connection: " << e.Message.c_str() << std::endl;
        }
        try {

            mPIErrorMonitorThread = new PIErrorMonitor(true);
            mPIErrorMonitorThread->registerErrorHandler(HandlePIErrorState);
            AnsiString hostAnsi = RawIpAddress->Text;
            mPIErrorMonitorThread->initSshConnection(hostAnsi.c_str(), "lg", "lg");  // FIXME: what if I forget this invokation?
            mPIErrorMonitorThread->Resume();
        }
        catch (const EIdException &e)
        {
            ShowMessage("Error while setting up ssh connection: " + e.Message);
        }
        #endif
        try
        {
            IdTCPClientRaw->Connect();
            TCPClientRawHandleThread = new TTCPClientRawHandleThread(true, msgProcThread);
            TCPClientRawHandleThread->UseFileInsteadOfNetwork = false;
            TCPClientRawHandleThread->FreeOnTerminate = TRUE;
            TCPClientRawHandleThread->OnTerminate = RawThreadTerminated;
            TCPClientRawHandleThread->Resume();
        }
        catch (const EIdException &e)
        {
            ShowMessage("Error while connecting: " + e.Message);
        }
    }
    else
    {
        TCPClientRawHandleThread->Terminate();
        IdTCPClientRaw->Disconnect();
        IdTCPClientRaw->IOHandler->InputBuffer->Clear();
        RawConnectButton->Caption = "Raw Connect";
        RawPlaybackButton->Enabled = true;
        // #ifdef ERROR_HANDLING_ENABLED
        // try {
        //     mPIErrorMonitorThread->Terminate();
        //     // delete mPIErrorMonitorThread;
        //     // mPIErrorMonitorThread = NULL;
        // } catch (const EIdException &e) {
        //     ShowMessage("Error while connecting: " + e.Message);
        // }
        // #endif
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::IdTCPClientRawConnected(TObject *Sender)
{
    // SetKeepAliveValues(const AEnabled: Boolean; const ATimeMS, AInterval: Integer);
    IdTCPClientRaw->Socket->Binding->SetKeepAliveValues(true, 60 * 1000, 15 * 1000);
    RawConnectButton->Caption = "Raw Disconnect";
    RawPlaybackButton->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::IdTCPClientRawDisconnected(TObject *Sender)
{
    TCPClientRawHandleThread->Terminate();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RawRecordButtonClick(TObject *Sender)
{
    if (RawRecordButton->Caption == "Raw Record")
    {
        if (RecordRawSaveDialog->Execute())
        {
            // First, check if the file exists.
            if (FileExists(RecordRawSaveDialog->FileName))
                ShowMessage("File " + RecordRawSaveDialog->FileName + "already exists. Cannot overwrite.");
            else
            {
                // Open a file for writing. Creates the file if it doesn't exist, or overwrites it if it does.
                RecordRawStream = new TStreamWriter(RecordRawSaveDialog->FileName, false);
                if (RecordRawStream == NULL)
                {
                    ShowMessage("Cannot Open File " + RecordRawSaveDialog->FileName);
                }
                else
                    RawRecordButton->Caption = "Stop Raw Recording";
            }
        }
    }
    else
    {
        delete RecordRawStream;
        RecordRawStream = NULL;
        RawRecordButton->Caption = "Raw Record";
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::RawPlaybackButtonClick(TObject *Sender)
{
    if ((RawPlaybackButton->Caption == "Raw Playback") && (Sender != NULL))
    {
        if (PlaybackRawDialog->Execute())
        {
            // First, check if the file exists.
            if (!FileExists(PlaybackRawDialog->FileName))
                ShowMessage("File " + PlaybackRawDialog->FileName + " does not exist");
            else
            {
                // Open a file for writing. Creates the file if it doesn't exist, or overwrites it if it does.
                PlayBackRawStream = new TStreamReader(PlaybackRawDialog->FileName);
                if (PlayBackRawStream == NULL)
                {
                    ShowMessage("Cannot Open File " + PlaybackRawDialog->FileName);
                }
                else
                {
                    // 콤보박스에서 속도 읽기
                    double playbackSpeed = 1.0;
                    if (RawPlaybackSpeedComboBox)
                    {
                        int idx = RawPlaybackSpeedComboBox->ItemIndex;
                        if (idx == 1) playbackSpeed = 2.0;
                        else if (idx == 2) playbackSpeed = 3.0;
                        else playbackSpeed = 1.0;
                    }
                    TCPClientRawHandleThread = new TTCPClientRawHandleThread(true, msgProcThread);
                    TCPClientRawHandleThread->UseFileInsteadOfNetwork = true;
                    TCPClientRawHandleThread->First = true;
                    TCPClientRawHandleThread->FreeOnTerminate = TRUE;
                    TCPClientRawHandleThread->OnTerminate = RawThreadTerminated;
                    TCPClientRawHandleThread->Resume();
                    RawPlaybackButton->Caption = "Stop Raw Playback";
                    RawConnectButton->Enabled = false;
                    RawPlaybackSpeedComboBox->Enabled = false;
                }
            }
        }
    }
    else
    {
        TCPClientRawHandleThread->Terminate();
        delete PlayBackRawStream;
        PlayBackRawStream = NULL;
        RawPlaybackButton->Caption = "Raw Playback";
        RawConnectButton->Enabled = true;
        RawPlaybackSpeedComboBox->Enabled = true;
    }
}
//---------------------------------------------------------------------------
// Constructor for the thread class
__fastcall TTCPClientRawHandleThread::TTCPClientRawHandleThread(bool value,
    TMessageProcessorThread* procThread, double playbackSpeed) : TThread(value), msgProcThread(procThread), PlaybackSpeed(playbackSpeed)
{
	printf("[Thread] TTCPClientRawHandleThread created.\n");
	FreeOnTerminate = true; // Automatically free the thread object after execution
}
//---------------------------------------------------------------------------
// Destructor for the thread class
__fastcall TTCPClientRawHandleThread::~TTCPClientRawHandleThread()
{
	printf("[Thread] TTCPClientRawHandleThread destroyed.\n");
	// Clean up resources if needed
}
//---------------------------------------------------------------------------
// Execute method where the thread's logic resides
void __fastcall TTCPClientRawHandleThread::Execute(void)
{
    __int64 Time, SleepTime;
    while (!Terminated)
    {
        if (!UseFileInsteadOfNetwork)
        {
            try
            {
                if (!Form1->IdTCPClientRaw->Connected())
                    Terminate();
                StringMsgBuffer = Form1->IdTCPClientRaw->IOHandler->ReadLn();
            }
            catch (...)
            {
                TThread::Synchronize(StopTCPClient);
                break;
            }
        }
        else
        {
            try
            {
                if (Form1->PlayBackRawStream->EndOfStream)
                {
                    printf("End Raw Playback 1\n");
                    TThread::Synchronize(StopPlayback);
                    break;
                }
                StringMsgBuffer = Form1->PlayBackRawStream->ReadLine();
                Time = StrToInt64(StringMsgBuffer);
                if (First)
                {
                    First = false;
                    LastTime = Time;
                }
                SleepTime = Time - LastTime;
                LastTime = Time;
                if (SleepTime > 0 && PlaybackSpeed > 0.0)
                {
                    EXECUTION_TIMER(sleepTime);
                    Sleep((int)(SleepTime / PlaybackSpeed));
                    EXECUTION_TIMER_ELAPSED(elapsed, sleepTime);
                    LOG("Raw playback sleep time: " + to_string(elapsed) + "ms");
                }
                if (Form1->PlayBackRawStream->EndOfStream)
                {
                    printf("End Raw Playback 2\n");
                    TThread::Synchronize(StopPlayback);
                    break;
                }
                StringMsgBuffer = Form1->PlayBackRawStream->ReadLine();
            }
            catch (...)
            {
                printf("Raw Playback Exception\n");
                TThread::Synchronize(StopPlayback);
                break;
            }
        }
        try
        {
            // Push RAW message into shared processor thread
            msgProcThread->AddMessage(MessageType::RAW, StringMsgBuffer);
        }
        catch (...)
        {
            ShowMessage("TTCPClientRawHandleThread::Execute Exception 3");
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TTCPClientRawHandleThread::StopPlayback(void)
{
    Form1->RawPlaybackButtonClick(NULL);
}
//---------------------------------------------------------------------------
void __fastcall TTCPClientRawHandleThread::StopTCPClient(void)
{
    Form1->RawConnectButtonClick(NULL);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CycleImagesClick(TObject *Sender)
{
    CurrentSpriteImage = 0;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SBSConnectButtonClick(TObject *Sender)
{
    IdTCPClientSBS->Host = SBSIpAddress->Text;
    IdTCPClientSBS->Port = 5002;

    if ((SBSConnectButton->Caption == "SBS Connect") && (Sender != NULL))
    {
        try
        {
            IdTCPClientSBS->Connect();
            TCPClientSBSHandleThread = new TTCPClientSBSHandleThread(true, msgProcThread);
            TCPClientSBSHandleThread->UseFileInsteadOfNetwork = false;
            TCPClientSBSHandleThread->FreeOnTerminate = TRUE;
            TCPClientSBSHandleThread->OnTerminate = SBSThreadTerminated;
            TCPClientSBSHandleThread->Resume();
        }
        catch (const EIdException &e)
        {
            ShowMessage("Error while connecting: " + e.Message);
        }
    }
    else
    {
        TCPClientSBSHandleThread->Terminate();
        IdTCPClientSBS->Disconnect();
        IdTCPClientSBS->IOHandler->InputBuffer->Clear();
        SBSConnectButton->Caption = "SBS Connect";
        SBSPlaybackButton->Enabled = true;
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// Constructor for the thread class
__fastcall TTCPClientSBSHandleThread::TTCPClientSBSHandleThread(bool value, 
    TMessageProcessorThread* procThread, double playbackSpeed) : TThread(value), msgProcThread(procThread), PlaybackSpeed(playbackSpeed)
{
	printf("[Thread] TTCPClientSBSHandleThread created.\n");
	FreeOnTerminate = true; // Automatically free the thread object after execution
    UseFileInsteadOfNetwork = false;
    First = true;
    LastTime = 0;
}
//---------------------------------------------------------------------------
// Destructor for the thread class
__fastcall TTCPClientSBSHandleThread::~TTCPClientSBSHandleThread()
{
	printf("[Thread] TTCPClientSBSHandleThread destroyed.\n");
	// Clean up resources if needed
}
//---------------------------------------------------------------------------
// Execute method where the thread's logic resides
void __fastcall TTCPClientSBSHandleThread::Execute(void)
{
    __int64 Time, SleepTime;
    while (!Terminated)
    {
        if (!UseFileInsteadOfNetwork)
        {
            try
            {
                if (!Form1->IdTCPClientSBS->Connected())
                    Terminate();

                // Receive message from Hub
                StringMsgBuffer = Form1->IdTCPClientSBS->IOHandler->ReadLn();
            }
            catch (...)
            {
                TThread::Synchronize(StopTCPClient);
                break;
            }
        }
        else
        {
            try
            {
                if (Form1->PlayBackSBSStream->EndOfStream)
                {
                    printf("End SBS Playback 1\n");
                    TThread::Synchronize(StopPlayback);
                    break;
                }

                // Read timestamp first
                AnsiString TimestampMsg = Form1->PlayBackSBSStream->ReadLine();
                Time = StrToInt64(TimestampMsg);
                if (First)
                {
                    First = false;
                    LastTime = Time;
                }
                SleepTime = Time - LastTime;
                LastTime = Time;
                if (SleepTime > 0 && PlaybackSpeed > 0.0)
                {
                    EXECUTION_TIMER(sleepTime);
                    Sleep((int)(SleepTime / PlaybackSpeed));
                    EXECUTION_TIMER_ELAPSED(elapsed, sleepTime);
                    LOG("Sleep time: " + std::to_string(elapsed) + "ms");
                }
                if (Form1->PlayBackSBSStream->EndOfStream)
                {
                    printf("End SBS Playback 2\n");
                    TThread::Synchronize(StopPlayback);
                    break;
                }

                // Read SBS message
                StringMsgBuffer = Form1->PlayBackSBSStream->ReadLine();
            }
            catch (...)
            {
                printf("SBS Playback Exception\n");
                TThread::Synchronize(StopPlayback);
                break;
            }
        }
        try
        {
            msgProcThread->AddMessage(MessageType::SBS, StringMsgBuffer);
        }
        catch (...)
        {
            ShowMessage("Failed to add SBS message");
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TTCPClientSBSHandleThread::StopPlayback(void)
{
    Form1->SBSPlaybackButtonClick(NULL);
}
//---------------------------------------------------------------------------
void __fastcall TTCPClientSBSHandleThread::StopTCPClient(void)
{
    Form1->SBSConnectButtonClick(NULL);
}
//---------------------------------------------------------------------------
// [START] TMessageProcessorThread
//---------------------------------------------------------------------------
__fastcall TMessageProcessorThread::TMessageProcessorThread(bool value) : TThread(value)
{
	printf("[Thread] TMessageProcessorThread created.\n");
	queueLock = new TCriticalSection();
	messageEvent = new TEvent(
        nullptr, // security attributes - none
        false,   // automatic reset after read by WaitFor()
        false,   // Initial state - not signaled
        "",      // no name
        false);  // Use COMWait - false
}
//---------------------------------------------------------------------------
TMessageProcessorThread::~TMessageProcessorThread()
{
	printf("[Thread] TMessageProcessorThread destroyed.\n");
    if (messageEvent) {
        delete messageEvent;
        messageEvent = NULL;
    }
    if (queueLock) {
        delete queueLock;
        queueLock = NULL;
    }
}
//---------------------------------------------------------------------------
void TMessageProcessorThread::AddMessage(MessageType type, const AnsiString& msg)
{
    queueLock->Enter();
    messageQueue.push({type, msg});
    queueLock->Leave();

    messageEvent->SetEvent();
}
//---------------------------------------------------------------------------
void TMessageProcessorThread::wakeUp()
{
    // Signal the event to wake up the thread
    messageEvent->SetEvent();
}
//---------------------------------------------------------------------------
void TMessageProcessorThread::clearQueue(void) {
    queueLock->Enter();
    while (!messageQueue.empty()) {
        messageQueue.pop();
    }
    queueLock->Leave();
}
//---------------------------------------------------------------------------
void __fastcall TMessageProcessorThread::Execute(void)
{
    const char *helicopter_type = NULL;
    const char *cntry;
    while (!Terminated) {
        messageEvent->WaitFor(INFINITE);

        while (true) {
            QueuedMessage qMsg;
            {
                queueLock->Enter();
                if (messageQueue.empty()) {
                    queueLock->Leave();
                    break;
                }
                qMsg = messageQueue.front();
                messageQueue.pop();
                queueLock->Leave();
            }

            switch (qMsg.type) {
                case MessageType::SBS:
                    if (Form1->RecordSBSStream)
                    {
                        __int64 CurrentTime;
                        CurrentTime = GetCurrentTimeInMsec();
                        Form1->RecordSBSStream->WriteLine(IntToStr(CurrentTime));
                        Form1->RecordSBSStream->WriteLine(qMsg.message);
                    }

                    if (Form1->BigQueryCSV)
                    {
                        Form1->BigQueryCSV->WriteLine(qMsg.message);
                        Form1->BigQueryRowCount++;
                        if (Form1->BigQueryRowCount >= BIG_QUERY_UPLOAD_COUNT)
                        {
                            Form1->CloseBigQueryCSV();
                            // printf("string is:%s\n", Form1->BigQueryPythonScript.c_str());
                            RunPythonScript(Form1->BigQueryPythonScript, Form1->BigQueryPath + " " + Form1->BigQueryCSVFileName);
                            Form1->CreateBigQueryCSV();
                        }
                    }

                    // Handle SBS message including hashmap update
                    SBS_Message_Decode(qMsg.message.c_str());
                    break;

                case MessageType::RAW: {
                    modeS_message mm;
                    TDecodeStatus Status;

                    if (Form1->RecordRawStream)
                    {
                        __int64 CurrentTime;
                        CurrentTime = GetCurrentTimeInMsec();
                        Form1->RecordRawStream->WriteLine(IntToStr(CurrentTime));
                        Form1->RecordRawStream->WriteLine(qMsg.message);
                    }

                    Status = decode_RAW_message(qMsg.message, &mm);
                    if (Status == HaveMsg)
                    {
                        uint32_t addr = (mm.AA[0] << 16) | (mm.AA[1] << 8) | mm.AA[2];

                        // Lookup or create aircraft entry
                        TADS_B_Aircraft *ADS_B_Aircraft = (TADS_B_Aircraft *)ght_get(Form1->HashTable, sizeof(addr), &addr);
                        if (ADS_B_Aircraft)
                        {
                            // Form1->MsgLog->Lines->Add("Retrived");
                        }
                        else
                        {
                            ADS_B_Aircraft = new TADS_B_Aircraft;
                            ADS_B_Aircraft->ICAO = addr;
                            snprintf(ADS_B_Aircraft->HexAddr, sizeof(ADS_B_Aircraft->HexAddr), "%06X", (int)addr);
                            ADS_B_Aircraft->NumMessagesSBS = 0;
                            ADS_B_Aircraft->NumMessagesRaw = 0;
                            ADS_B_Aircraft->VerticalRate = 0;
                            ADS_B_Aircraft->HaveAltitude = false;
                            ADS_B_Aircraft->HaveLatLon = false;
                            ADS_B_Aircraft->HaveSpeedAndHeading = false;
                            ADS_B_Aircraft->HaveFlightNum = false;
                            ADS_B_Aircraft->IsHelicopter=false;
                            ADS_B_Aircraft->IsMilitary=false;
                            ADS_B_Aircraft->SpriteImage = Form1->CurrentSpriteImage;
                            if (AircraftDB->IsHelicopter(ADS_B_Aircraft->ICAO, &helicopter_type)) {
                                ADS_B_Aircraft->IsHelicopter=true;
                                ADS_B_Aircraft->SpriteImage = Form1->CurrentSpriteImage + 52;
                            }
                            if (AircraftDB->IsMilitary(ADS_B_Aircraft->ICAO, &cntry)) {
                                ADS_B_Aircraft->IsMilitary = true;
                                ADS_B_Aircraft->SpriteImage = Form1->CurrentSpriteImage + 76;
                            }

                            if (Form1->CycleImages->Checked)
                                Form1->CurrentSpriteImage = (Form1->CurrentSpriteImage + 1) % Form1->NumSpriteImages;

                            if (ght_insert(Form1->HashTable, ADS_B_Aircraft, sizeof(addr), &addr) < 0)
                            {
                                printf("ght_insert Error - Should Not Happen\n");
                            }
                        }

                        RawToAircraft(&mm, ADS_B_Aircraft);
                    }
                    else {
                        printf("Raw Decode Error:%d\n", Status);
                    }
                    break;
                }
            }
        }
    }
}
//---------------------------------------------------------------------------
// [END] TMessageProcessorThread
//---------------------------------------------------------------------------
void __fastcall TForm1::SBSRecordButtonClick(TObject *Sender)
{
    if (SBSRecordButton->Caption == "SBS Record")
    {
        if (RecordSBSSaveDialog->Execute())
        {
            // First, check if the file exists.
            if (FileExists(RecordSBSSaveDialog->FileName))
                ShowMessage("File " + RecordSBSSaveDialog->FileName + "already exists. Cannot overwrite.");
            else
            {
                // Open a file for writing. Creates the file if it doesn't exist, or overwrites it if it does.
                RecordSBSStream = new TStreamWriter(RecordSBSSaveDialog->FileName, false);
                if (RecordSBSStream == NULL)
                {
                    ShowMessage("Cannot Open File " + RecordSBSSaveDialog->FileName);
                }
                else
                    SBSRecordButton->Caption = "Stop SBS Recording";
            }
        }
    }
    else
    {
        delete RecordSBSStream;
        RecordSBSStream = NULL;
        SBSRecordButton->Caption = "SBS Record";
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SBSPlaybackButtonClick(TObject *Sender)
{
    if ((SBSPlaybackButton->Caption == "SBS Playback") && (Sender != NULL))
    {
        if (PlaybackSBSDialog->Execute())
        {
            // First, check if the file exists.
            if (!FileExists(PlaybackSBSDialog->FileName))
                ShowMessage("File " + PlaybackSBSDialog->FileName + " does not exist");
            else
            {
                // Open a file for writing. Creates the file if it doesn't exist, or overwrites it if it does.
                PlayBackSBSStream = new TStreamReader(PlaybackSBSDialog->FileName);
                if (PlayBackSBSStream == NULL)
                {
                    ShowMessage("Cannot Open File " + PlaybackSBSDialog->FileName);
                }
                else
                {
                    // 콤보박스에서 속도 읽기
                    double playbackSpeed = 1.0;
                    if (SBSPlaybackSpeedComboBox)
                    {
                        int idx = SBSPlaybackSpeedComboBox->ItemIndex;
                        if (idx == 1) playbackSpeed = 2.0;
                        else if (idx == 2) playbackSpeed = 3.0;
                        else playbackSpeed = 1.0;
                    }
                    TCPClientSBSHandleThread = new TTCPClientSBSHandleThread(true, msgProcThread, playbackSpeed);
                    TCPClientSBSHandleThread->UseFileInsteadOfNetwork = true;
                    TCPClientSBSHandleThread->First = true;
                    TCPClientSBSHandleThread->FreeOnTerminate = TRUE;
                    TCPClientSBSHandleThread->OnTerminate = SBSThreadTerminated;
                    TCPClientSBSHandleThread->Resume();
                    SBSPlaybackButton->Caption = "Stop SBS Playback";
                    SBSConnectButton->Enabled = false;
                    SBSPlaybackSpeedComboBox->Enabled = false;
                }
            }
        }
    }
    else
    {
        TCPClientSBSHandleThread->Terminate();
        delete PlayBackSBSStream;
        PlayBackSBSStream = NULL;
        SBSPlaybackButton->Caption = "SBS Playback";
        SBSConnectButton->Enabled = true;
        SBSPlaybackSpeedComboBox->Enabled = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::IdTCPClientSBSConnected(TObject *Sender)
{
    // SetKeepAliveValues(const AEnabled: Boolean; const ATimeMS, AInterval: Integer);
    IdTCPClientSBS->Socket->Binding->SetKeepAliveValues(true, 60 * 1000, 15 * 1000);
    SBSConnectButton->Caption = "SBS Disconnect";
    SBSPlaybackButton->Enabled = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::IdTCPClientSBSDisconnected(TObject *Sender)
{
    TCPClientSBSHandleThread->Terminate();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::TimeToGoTrackBarChange(TObject *Sender)
{
    _int64 hmsm;
    hmsm = TimeToGoTrackBar->Position * 1000;
    TimeToGoText->Caption = TimeToChar(hmsm);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LoadMap(int Type)
{
    // Clean up previous provider if any
    if (currentMapProvider) {
        delete currentMapProvider;
        currentMapProvider = nullptr;
    }
    // Create new provider using the factory
    currentMapProvider = MapProviderFactory::Create(Type);
    if (currentMapProvider) {
        currentMapProvider->Initialize(LoadMapFromInternet);
        currentMapProvider->Resize(ObjectDisplay->Width, ObjectDisplay->Height);
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::MapComboBoxChange(TObject *Sender)
{
    ReloadMapProvider();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ReloadMapProvider()
{
    double m_Eyeh = GetEarthView()->m_Eye.h;
    double m_Eyex = GetEarthView()->m_Eye.x;
    double m_Eyey = GetEarthView()->m_Eye.y;

    Timer1->Enabled = false;
    Timer2->Enabled = false;
    LoadMap(MapComboBox->ItemIndex);
    GetEarthView()->m_Eye.h = m_Eyeh;
    GetEarthView()->m_Eye.x = m_Eyex;
    GetEarthView()->m_Eye.y = m_Eyey;
    Timer1->Enabled = true;
    Timer2->Enabled = true;
}

void __fastcall TForm1::CheckBoxUpdateMapTilesClick(TObject *Sender)
{
    LoadMapFromInternet = CheckBoxUpdateMapTiles->Checked;
    ReloadMapProvider();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::BigQueryCheckBoxClick(TObject *Sender)
{
    if (BigQueryCheckBox->State == cbChecked)
        CreateBigQueryCSV();
    else
    {
        CloseBigQueryCSV();
        RunPythonScript(BigQueryPythonScript, BigQueryPath + " " + BigQueryCSVFileName);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::CreateBigQueryCSV(void)
{
    AnsiString HomeDir = ExtractFilePath(ExtractFileDir(Application->ExeName));
    BigQueryCSVFileName = "BigQuery" + UIntToStr(BigQueryFileCount) + ".csv";
    BigQueryRowCount = 0;
    BigQueryFileCount++;
    BigQueryCSV = new TStreamWriter(HomeDir + "..\\BigQuery\\" + BigQueryCSVFileName, false);
    if (BigQueryCSV == NULL)
    {
        ShowMessage("Cannot Open BigQuery CSV File " + HomeDir + "..\\BigQuery\\" + BigQueryCSVFileName);
        BigQueryCheckBox->State = cbUnchecked;
    }
    AnsiString Header = AnsiString("Message Type,Transmission Type,SessionID,AircraftID,HexIdent,FlightID,Date_MSG_Generated,Time_MSG_Generated,Date_MSG_Logged,Time_MSG_Logged,Callsign,Altitude,GroundSpeed,Track,Latitude,Longitude,VerticalRate,Squawk,Alert,Emergency,SPI,IsOnGround");
    BigQueryCSV->WriteLine(Header);
}
//--------------------------------------------------------------------------
void __fastcall TForm1::CloseBigQueryCSV(void)
{
    if (BigQueryCSV)
    {
        delete BigQueryCSV;
        BigQueryCSV = NULL;
    }
}
//--------------------------------------------------------------------------
static void RunPythonScript(AnsiString scriptPath, AnsiString args)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    AnsiString pythonExecutableName = ExtractFilePath(ExtractFileDir(Application->ExeName)) + AnsiString("..\\Python\\python.exe");
    AnsiString commandLine = pythonExecutableName + " " + scriptPath + " " + args;
    char *cmdLineCharArray = new char[strlen(commandLine.c_str()) + 1];
    strcpy(cmdLineCharArray, commandLine.c_str());
#define LOG_PYTHON 1
#if LOG_PYTHON
    // printf("%s\n", cmdLineCharArray);
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    HANDLE h = CreateFileA(Form1->BigQueryLogFileName.c_str(),
                           FILE_APPEND_DATA,
                           FILE_SHARE_WRITE | FILE_SHARE_READ,
                           &sa,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    si.hStdInput = NULL;
    si.hStdOutput = h;
    si.hStdError = h; // Redirect standard error as well, if needed
    si.dwFlags |= STARTF_USESTDHANDLES;
#endif
    if (!CreateProcessA(
            nullptr,          // No module name (use command line)
            cmdLineCharArray, // Command line
            nullptr,          // Process handle not inheritable
            nullptr,          // Thread handle not inheritable
#if LOG_PYTHON
            TRUE,
#else
            FALSE, // Set handle inheritance to FALSE
#endif
            CREATE_NO_WINDOW, // Don't create a console window
            nullptr,          // Use parent's environment block
            nullptr,          // Use parent's starting directory
            &si,              // Pointer to STARTUPINFO structure
            &pi))             // Pointer to PROCESS_INFORMATION structure
    {
        std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
        delete[] cmdLineCharArray;
        return;
    }

    // Optionally, detach from the process
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    delete[] cmdLineCharArray;
}

//--------------------------------------------------------------------------
void __fastcall TForm1::UseSBSRemoteClick(TObject *Sender)
{
    SBSIpAddress->Text = "data.adsbhub.org";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UseSBSLocalClick(TObject *Sender)
{
    SBSIpAddress->Text = "128.237.96.41";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UseRawRouterClick(TObject *Sender)
{
    RawIpAddress->Text = "192.168.0.223";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UseRawCmuSecureClick(TObject *Sender)
{
    RawIpAddress->Text = "172.26.43.119";
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UseRawHyattClick(TObject *Sender)
{
    RawIpAddress->Text = "172.20.2.97";
}
//---------------------------------------------------------------------------

static bool DeleteFilesWithExtension(AnsiString dirPath, AnsiString extension)
{
    AnsiString searchPattern = dirPath + "\\*." + extension;
    WIN32_FIND_DATAA findData;

    HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &findData);

    if (hFind == INVALID_HANDLE_VALUE)
    {
        return false; // No files found or error
    }

    do
    {
        AnsiString filePath = dirPath + "\\" + findData.cFileName;
        if (DeleteFileA(filePath.c_str()) == 0)
        {
            FindClose(hFind);
            return false; // Failed to delete a file
        }
    } while (FindNextFileA(hFind, &findData) != 0);

    FindClose(hFind);
    return true;
}
static bool IsFirstRow = true;
static bool CallBackInit = false;
//---------------------------------------------------------------------------
static int CSV_callback_ARTCCBoundaries(struct CSV_context *ctx, const char *value)
{
    int rc = 1;
    static char LastArea[512];
    static char Area[512];
    static char Lat[512];
    static char Lon[512];
    int Deg, Min, Sec, Hsec;
    char Dir;

    if (ctx->field_num == 0)
    {
        strcpy(Area, value);
    }
    else if (ctx->field_num == 3)
    {
        strcpy(Lat, value);
    }
    else if (ctx->field_num == 4)
    {
        strcpy(Lon, value);
    }

    if (ctx->field_num == (ctx->num_fields - 1))
    {

        float fLat, fLon;
        if (!IsFirstRow)
        {
            if (!CallBackInit)
            {
                strcpy(LastArea, Area);
                CallBackInit = true;
            }
            if (strcmp(LastArea, Area) != 0)
            {

                if (FinshARTCCBoundary())
                {
                    printf("Load ERROR ID %s\n", LastArea);
                }
                else
                    printf("Loaded ID %s\n", LastArea);
                strcpy(LastArea, Area);
            }
            if (Form1->AreaTemp == NULL)
            {
                Form1->AreaTemp = new TArea;
                Form1->AreaTemp->NumPoints = 0;
                Form1->AreaTemp->Name = Area;
                Form1->AreaTemp->Selected = false;
                Form1->AreaTemp->Triangles = NULL;
                printf("Loading ID %s\n", Area);
            }
            if (sscanf(Lat, "%2d%2d%2d%2d%c", &Deg, &Min, &Sec, &Hsec, &Dir) != 5)
                printf("Latitude Parse Error\n");
            fLat = Deg + Min / 60.0 + Sec / 3600.0 + Hsec / 360000.00;
            if (Dir == 'S')
                fLat = -fLat;

            if (sscanf(Lon, "%3d%2d%2d%2d%c", &Deg, &Min, &Sec, &Hsec, &Dir) != 5)
                printf("Longitude Parse Error\n");
            fLon = Deg + Min / 60.0 + Sec / 3600.0 + Hsec / 360000.00;
            if (Dir == 'W')
                fLon = -fLon;
            // printf("%f, %f\n",fLat,fLon);
            if (Form1->AreaTemp->NumPoints < MAX_AREA_POINTS)
            {
                Form1->AreaTemp->Points[Form1->AreaTemp->NumPoints][1] = fLat;
                Form1->AreaTemp->Points[Form1->AreaTemp->NumPoints][0] = fLon;
                Form1->AreaTemp->Points[Form1->AreaTemp->NumPoints][2] = 0.0;
                Form1->AreaTemp->NumPoints++;
            }
            else
                printf("Max Area Points Reached\n");
        }
        if (IsFirstRow)
            IsFirstRow = false;
    }
    return (rc);
}
//---------------------------------------------------------------------------
bool __fastcall TForm1::LoadARTCCBoundaries(AnsiString FileName)
{
    CSV_context csv_ctx;
    memset(&csv_ctx, 0, sizeof(csv_ctx));
    csv_ctx.file_name = FileName.c_str();
    csv_ctx.delimiter = ',';
    csv_ctx.callback = CSV_callback_ARTCCBoundaries;
    csv_ctx.line_size = 2000;
    IsFirstRow = true;
    CallBackInit = false;
    if (!CSV_open_and_parse_file(&csv_ctx))
    {
        printf("Parsing of \"%s\" failed: %s\n", FileName.c_str(), strerror(errno));
        return (false);
    }
    if ((Form1->AreaTemp != NULL) && (Form1->AreaTemp->NumPoints > 0))
    {
        char Area[512];
        strcpy(Area, Form1->AreaTemp->Name.c_str());
        if (FinshARTCCBoundary())
        {
            printf("Loaded ERROR ID %s\n", Area);
        }
        else
            printf("Loaded ID %s\n", Area);
    }
    printf("Done\n");
    return (true);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::LoadARTCCBoundaries1Click(TObject *Sender)
{
    LoadARTCCBoundaries(ARTCCBoundaryDataPathFileName);
}
//---------------------------------------------------------------------------
static int FinshARTCCBoundary(void)
{
    int or1 = orientation2D_Polygon(Form1->AreaTemp->Points, Form1->AreaTemp->NumPoints);
    if (or1 == 0)
    {
        TArea *Temp;
        Temp = Form1->AreaTemp;
        Form1->AreaTemp = NULL;
        delete Temp;
        printf("Degenerate Polygon\n");
        return (-1);
    }
    if (or1 == CLOCKWISE)
    {
        DWORD i;

        memcpy(Form1->AreaTemp->PointsAdj, Form1->AreaTemp->Points, sizeof(Form1->AreaTemp->Points));
        for (i = 0; i < Form1->AreaTemp->NumPoints; i++)
        {
            memcpy(Form1->AreaTemp->Points[i],
                   Form1->AreaTemp->PointsAdj[Form1->AreaTemp->NumPoints - 1 - i], sizeof(pfVec3));
        }
    }
    if (checkComplex(Form1->AreaTemp->Points, Form1->AreaTemp->NumPoints))
    {
        TArea *Temp;
        Temp = Form1->AreaTemp;
        Form1->AreaTemp = NULL;
        delete Temp;
        printf("Polygon is Complex\n");
        return (-2);
    }
    DWORD Row, Count, i;

    Count = Form1->Areas->Count;
    for (i = 0; i < Count; i++)
    {
        TArea *Area = (TArea *)Form1->Areas->Items[i];
        if (Area->Name == Form1->AreaTemp->Name)
        {

            TArea *Temp;
            Temp = Form1->AreaTemp;
            printf("Duplicate Area Name %s\n", Form1->AreaTemp->Name.c_str());
            ;
            Form1->AreaTemp = NULL;
            delete Temp;
            return (-3);
        }
    }

    triangulatePoly(Form1->AreaTemp->Points, Form1->AreaTemp->NumPoints,
                    &Form1->AreaTemp->Triangles);

    Form1->AreaTemp->Color = TColor(PopularColors[CurrentColor]);
    CurrentColor++;
    CurrentColor = CurrentColor % NumColors;
    Form1->Areas->Add(Form1->AreaTemp);
    Form1->AreaListView->Items->BeginUpdate();
    Form1->AreaListView->Items->Add();
    Row = Form1->AreaListView->Items->Count - 1;
    Form1->AreaListView->Items->Item[Row]->Caption = Form1->AreaTemp->Name;
    Form1->AreaListView->Items->Item[Row]->Data = Form1->AreaTemp;
    Form1->AreaListView->Items->Item[Row]->SubItems->Add("");
    Form1->AreaListView->Items->EndUpdate();
    
    // Original code (commented out for backup)
    /*
    std::unique_ptr<AirplaneFilterInterface> artccBoundaryFilter(new ZoneFilter());
    TArea *currentArea;
    currentArea = Form1->AreaTemp;
    std::string areaName(Form1->AnsiStringToStdString(currentArea->Name));
    std::cout << "Adding filter area: " << areaName << std::endl;
    artccBoundaryFilter->updateFilterArea(*currentArea, Form1->AnsiStringToStdString(currentArea->Name));
    Form1->AreaFilter.addFilter(Form1->AnsiStringToStdString(currentArea->Name), std::move(artccBoundaryFilter));
    Form1->AreaFilter.activateFilter(Form1->AnsiStringToStdString(currentArea->Name));
    printf("Area %s added to AreaFilter\n", currentArea->Name.c_str());
    */
    
    // Add area to filter using extracted function
    Form1->AddAreaToFilter(Form1->AreaTemp);
    
    Form1->AreaTemp = NULL;
    return 0;
}
//---------------------------------------------------------------------------

FlatEarthView* TForm1::GetEarthView() const {
    return currentMapProvider ? currentMapProvider->GetEarthView() : nullptr;
}

TileManager* TForm1::GetTileManager() const {
    return currentMapProvider ? currentMapProvider->GetTileManager() : nullptr;
}

int TForm1::GetUnregisteredAircraftCount(void)
{
    int count = 0;
    ght_iterator_t iterator;
    uint32_t *Key;
    TADS_B_Aircraft *Data;
    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if (!AircraftDB->aircraft_is_registered(Data->ICAO))
            count++;
    }
    return count;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::UpdateUnregisteredCount(void)
{
    UnregisteredAircraftCount = GetUnregisteredAircraftCount();
    UnregisteredCount->Caption = "Unregistered: " + IntToStr(UnregisteredAircraftCount);
}
//---------------------------------------------------------------------------
void __fastcall TForm1::HideUnregisteredCheckBoxClick(TObject *Sender)
{
    if (HideUnregisteredCheckBox->Checked && TrackHook.Valid_CC)
    {
        TADS_B_Aircraft *Data = (TADS_B_Aircraft *)ght_get(HashTable, sizeof(TrackHook.ICAO_CC), (void *)&TrackHook.ICAO_CC);
        if (Data && !AircraftDB->aircraft_is_registered(Data->ICAO))
        {
            TrackHook.Valid_CC = false;
            TrackHook.Valid_CPA = false;

            ICAOLabel->Caption = "N/A";
            FlightNumLabel->Caption = "N/A";
            CLatLabel->Caption = "N/A";
            CLonLabel->Caption = "N/A";
            SpdLabel->Caption = "N/A";
            HdgLabel->Caption = "N/A";
            AltLabel->Caption = "N/A";
            MsgCntLabel->Caption = "N/A";
            TrkLastUpdateTimeLabel->Caption = "N/A";
        }
    }

    ObjectDisplay->Repaint();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
    printf("[FormClose] Starting application shutdown...\n");

    int result = MessageDlg("Confirm exit: are you sure to close the program?",
                        mtConfirmation,
                        TMsgDlgButtons() << mbYes << mbNo, 0);
    if (result == mrNo)
    {
        Action = caNone;
        return;
    }

    // Also ensure the aircraft database loading thread is terminated.
    if (AircraftDB)
    {
        AircraftDB->CancelAndJoin();
    }

    if (TCPClientRawHandleThread)
    {
        TCPClientRawHandleThread->Terminate();
    }
    if (TCPClientSBSHandleThread)
    {
        TCPClientSBSHandleThread->Terminate();
    }

    if (msgProcThread) {
        msgProcThread->clearQueue(); // Clear any remaining messages
        msgProcThread->Terminate(); // change Terminated flag to true
        msgProcThread->wakeUp(); // trigger natual exit
        msgProcThread = NULL;
    }

    // Wait for threads to terminate
    while (TCPClientRawHandleThread || TCPClientSBSHandleThread)
    {
        Application->ProcessMessages(); // Allow OnTerminate to be processed
        Sleep(50);
    }
    printf("[FormClose] All threads terminated.\n");
}

void __fastcall TForm1::RawThreadTerminated(TObject *Sender)
{
    TCPClientRawHandleThread = NULL;
}

void __fastcall TForm1::SBSThreadTerminated(TObject *Sender)
{
    TCPClientSBSHandleThread = NULL;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::DrawBlackDot(double lat, double lot)
{
    double ScrX, ScrY;

    LatLon2XY(lat, lot, ScrX, ScrY);

    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

    glPointSize(20.0);
    // glColor4f(0.0, 0.0, 0.0, 1.0);

    glBegin(GL_POINTS);
    glVertex2f(ScrX, ScrY);
    glEnd();

    glDisable(GL_POINT_SMOOTH);
    glDisable(GL_BLEND);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::DrawAirportsBatch(void)
{
    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<std::pair<double, double>> airportPositions;

    std::unordered_map<std::string, Airport> &airportCodeMap = AirportMgr.getAirportCodeMap();
    airportPositions.reserve(airportCodeMap.size());

    double maxLatFromCorners, minLatFromCorners, maxLonFromCorners, minLonFromCorners;
    getScreenLatLonBounds(minLatFromCorners, maxLatFromCorners, minLonFromCorners, maxLonFromCorners);

    static int boundaryLogCount = 0;
    boundaryLogCount++;
    if (boundaryLogCount % 500 == 0) {
        std::cout << "Screen Boundary Log (#" << boundaryLogCount << "): "
                  << "Lat[" << minLatFromCorners << ", " << maxLatFromCorners << "] "
                  << "Lon[" << minLonFromCorners << ", " << maxLonFromCorners << "]"
                  << std::endl;
    }

    int loopCount = 0;
    for(const auto &airportPair : airportCodeMap) {
        const Airport &airport = airportPair.second;
        double airportLat = airport.getLatitude();
        double airportLon = airport.getLongitude();

        // Only draw tower if airport has IATA code
        std::string iataCode = airport.getIATA();
        if (!iataCode.empty() &&
            airportLat >= minLatFromCorners && airportLat <= maxLatFromCorners &&
            airportLon >= minLonFromCorners && airportLon <= maxLonFromCorners) {
            double ScrX, ScrY;
            LatLon2XY(airportLat, airportLon, ScrX, ScrY);
            airportPositions.push_back({ScrX, ScrY});
        }
    }

    int i = 0;

    for(const auto& pos : airportPositions) {
        DrawTowerImage(pos.first, pos.second, getCurrentZoomLevel());
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);


    static int call_count = 0;
    call_count++;
    if (call_count % 100 == 0) {
        char time_str[256];
        sprintf(time_str, "Airport tower drawing took %lld milliseconds for %zu airports (call #%d)",
                duration.count(), airportPositions.size(), call_count);
        std::cout << time_str << std::endl;
    }
}
//---------------------------------------------------------------------------

float __fastcall TForm1::getCurrentZoomLevel(void)
{

    double currentZoom = GetEarthView() ? GetEarthView()->m_Eye.h : 1.0;
    double baseZoom = pow(1.3, 18);
    double zoomRatio = baseZoom / currentZoom;


    float scale = static_cast<float>(std::min(2.0, zoomRatio * 0.00008));

    static int scaleLogCount = 0;
    scaleLogCount++;
    // if (scaleLogCount % 5000 == 0) {
    //     std::cout << "Scale Log (#" << scaleLogCount << "): "
    //               <<  scale
    //               << " (currentZoom: "  << GetEarthView()->m_Eye.h
    //               << ", zoomRatio: " <<  zoomRatio << ")"
    //               << std::endl;
    // }

    return scale;
}
//---------------------------------------------------------------------------

bool __fastcall TForm1::LoadTowerTexture(void)
{
    if (towerTextureLoaded) {
        return true;
    }

    const char *filename = "..\\..\\Symbols\\tower-64.png";
    int width, height, nrChannels;

    unsigned char *imageData = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (!imageData) {
        std::cout << "Failed to load tower texture: " << filename << std::endl;
        return false;
    }

    glGenTextures(1, &towerTextureID);
    glBindTexture(GL_TEXTURE_2D, towerTextureID);

    GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(imageData);

    towerTextureLoaded = true;
    std::cout << "Tower texture loaded successfully: " << width << "x" << height << " (" << nrChannels << " channels)" << std::endl;

    return true;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::DrawTowerImage(float x, float y, float scale)
{


    // Save current OpenGL state
    GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean textureEnabled = glIsEnabled(GL_TEXTURE_2D);
    GLint blendSrc, blendDst;
    glGetIntegerv(GL_BLEND_SRC, &blendSrc);
    glGetIntegerv(GL_BLEND_DST, &blendDst);

    glPushMatrix();
    if (!blendEnabled)
    {
        glEnable(GL_BLEND);
    }
    if (!textureEnabled)
    {
        glEnable(GL_TEXTURE_2D);
    }
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindTexture(GL_TEXTURE_2D, towerTextureID);

    // Set sky blue color to make tower more visible
    glColor4f(0.5f, 0.8f, 1.0f, 1.0f); // Sky blue with full opacity

    glTranslatef(x, y, 0.0f);


    float size = 32.0f * scale;

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-size/2, size/2);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(size/2, size/2);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(size/2, -size/2);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-size/2, -size/2);
    glEnd();

    // Reset color to white to not affect other drawings
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBindTexture(GL_TEXTURE_2D, 0);
    
    // Restore previous OpenGL state
    // if (!textureEnabled) glDisable(GL_TEXTURE_2D);
    // if (!blendEnabled) glDisable(GL_BLEND);
    // else glBlendFunc(blendSrc, blendDst);
    glPopMatrix();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::getScreenLatLonBounds(double &minLat, double &maxLat, double &minLon, double &maxLon)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int screenX = viewport[0];
    int screenY = viewport[1];
    int screenWidth = viewport[2];
    int screenHeight = viewport[3];

    double topLeftLat, topLeftLon, topRightLat, topRightLon;
    double bottomLeftLat, bottomLeftLon, bottomRightLat, bottomRightLon;

    XY2LatLon2(screenX, (ObjectDisplay->Height - 1) - screenY, bottomLeftLat, bottomLeftLon);                    // ����
    XY2LatLon2(screenWidth, (ObjectDisplay->Height - 1) - screenY, bottomRightLat, bottomRightLon);             // ����
    XY2LatLon2(screenX, (ObjectDisplay->Height - 1) - screenHeight, topLeftLat, topLeftLon);                    // �»�
    XY2LatLon2(screenWidth, (ObjectDisplay->Height - 1) - screenHeight, topRightLat, topRightLon);              // ���

    maxLat = std::max({topLeftLat, topRightLat, bottomLeftLat, bottomRightLat});
    minLat = std::min({topLeftLat, topRightLat, bottomLeftLat, bottomRightLat});
    maxLon = std::max({topLeftLon, topRightLon, bottomLeftLon, bottomRightLon});
    minLon = std::min({topLeftLon, topRightLon, bottomLeftLon, bottomRightLon});
}
//---------------------------------------------------------------------------
void __fastcall TForm1::HandlePIErrorState(const int &code) {
    static bool needReconnectRaw = false;
    std::cout << "Error to UI: " << code << std::endl;
    AnsiString errorMessages;

    // Check each error bit and append the corresponding message
    if (code & BITMASK_SSH_DISCONNECTED) {
        errorMessages += "Disconnected RPI\n";
        LabelErrorMessage->Caption = errorMessages;
        LabelErrorMessage->Font->Color = clRed;
        std::cout << "Disconnected RPI" << std::endl;
        RawConnectButtonClick(RawConnectButton); // disconnect via the actual button
        needReconnectRaw = true; // set flag to reconnect raw
    }
    if (code & BITMAKS_SDRUSB_DISCONNECTED) {
        errorMessages += "SDR Disconnected. Exited dump1090\n";
        LabelErrorMessage->Caption = errorMessages;
        LabelErrorMessage->Font->Color = clRed;
        needReconnectRaw = true; // set flag to reconnect raw
    }
    if (code & BITMASK_DUMP1090_NOT_RUNNING) {
        errorMessages += "dump1090 is N/A.\n";
        LabelErrorMessage->Caption = errorMessages;
        LabelErrorMessage->Font->Color = clRed;
        needReconnectRaw = true; // set flag to reconnect raw
    }
    if (code == 0) {
        if (needReconnectRaw == true) {
            needReconnectRaw = false; // reset flag
            std::cout << "Auto reconnect RPI" << std::endl;
            RawConnectButtonClick(RawConnectButton); // reconnect via the actual button
        }
        LabelErrorMessage->Caption = "";
        LabelErrorMessage->Font->Color = clGreen;
    }
}
TArea __fastcall TForm1::getScreenBoundsAsArea()
{
    TArea screenArea;
    
    // Initialize the area structure
    screenArea.Name = "Screen Bounds";
    screenArea.Color = clRed;  // Default color
    screenArea.NumPoints = 4;  // Rectangle has 4 corners
    screenArea.Selected = false;
    screenArea.Triangles = nullptr;
    
    // Get the current screen bounds
    double minLat, maxLat, minLon, maxLon;
    getScreenLatLonBounds(minLat, maxLat, minLon, maxLon);
    
    // Create rectangle points in clockwise order
    // Bottom-left
    screenArea.Points[0][0] = minLon;  // Longitude
    screenArea.Points[0][1] = minLat;  // Latitude
    screenArea.Points[0][2] = 0.0;     // Altitude
    
    // Bottom-right
    screenArea.Points[1][0] = maxLon;  // Longitude
    screenArea.Points[1][1] = minLat;  // Latitude
    screenArea.Points[1][2] = 0.0;     // Altitude
    
    // Top-right
    screenArea.Points[2][0] = maxLon;  // Longitude
    screenArea.Points[2][1] = maxLat;  // Latitude
    screenArea.Points[2][2] = 0.0;     // Altitude
    
    // Top-left
    screenArea.Points[3][0] = minLon;  // Longitude
    screenArea.Points[3][1] = maxLat;  // Latitude
    screenArea.Points[3][2] = 0.0;     // Altitude
    
    // Copy Points to PointsAdj (screen coordinates will be calculated when needed)
    for (int i = 0; i < 4; i++) {
        screenArea.PointsAdj[i][0] = screenArea.Points[i][0];
        screenArea.PointsAdj[i][1] = screenArea.Points[i][1];
        screenArea.PointsAdj[i][2] = screenArea.Points[i][2];
    }
    
    return screenArea;
}
//---------------------------------------------------------------------------

std::string TForm1::AnsiStringToStdString(const AnsiString& ansiStr)
{
    if (ansiStr.IsEmpty()) {
        return std::string();
    }
    
    // Convert AnsiString to std::string using c_str()
    return std::string(ansiStr.c_str());
}
//---------------------------------------------------------------------------

void __fastcall TForm1::AddAreaToFilter(TArea* area)
{
    if (!area) {
        std::cerr << "Error: Cannot add null area to filter" << std::endl;
        return;
    }
    //std::unique_ptr<AirplaneFilterInterface> artccBoundaryFilter(new ZoneFilter());
    std::unique_ptr<ZoneFilter> artccBoundaryFilter(new ZoneFilter());
    std::string areaName(AnsiStringToStdString(area->Name));
    std::cout << "Adding filter area: " << areaName << std::endl;
    artccBoundaryFilter->updateFilterArea(*area, AnsiStringToStdString(area->Name));
    AreaFilter.addFilter(AnsiStringToStdString(area->Name), std::move(artccBoundaryFilter));
    AreaFilter.activateFilter(AnsiStringToStdString(area->Name));
    printf("Area %s added to AreaFilter\n", area->Name.c_str());
}
//---------------------------------------------------------------------------
void __fastcall TForm1::SearchAircraftClick(TObject *Sender)
{
    AnsiString searchInput = AircraftNumber->Text.Trim().UpperCase();
    
    if (searchInput.IsEmpty()) {
        ShowMessage("Please enter an ICAO code or callsign to search.");
        return;
    }
    
    // Search through all aircraft in the hash table
    uint32_t *Key;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data;
    TADS_B_Aircraft *foundAircraft = nullptr;
    
    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if (Data->HaveLatLon) {
            // Check ICAO code (HexAddr)
            AnsiString icaoCode = AnsiString(Data->HexAddr).UpperCase();
            
            // Check callsign (FlightNum)
            AnsiString callsign = "";
            if (Data->HaveFlightNum && strlen(Data->FlightNum) > 0) {
                callsign = AnsiString(Data->FlightNum).Trim().UpperCase();
            }
            
            // Match either ICAO or callsign
            if (icaoCode == searchInput || callsign == searchInput) {
                foundAircraft = Data;
                break;
            }
        }
    }
    
    if (foundAircraft) {
        // Aircraft found - center map on aircraft and highlight it
        if (GetEarthView()) {
            // Method 1: Use proper coordinate transformation
            // The EarthView coordinate system: -0.5 to +0.5 for both x and y
            // Longitude: -180° to +180° maps to -0.5 to +0.5
            double normalizedLon = foundAircraft->Longitude / 360.0;
            
            // Latitude: -90° to +90°, but need to account for Mercator projection
            // Use the inverse of what LatLon2XY does
            double latRad = foundAircraft->Latitude * M_PI / 180.0;
            double mercatorY = asinh(tan(latRad)) / (2 * M_PI);
            
            // Set the eye position to center on the aircraft
            GetEarthView()->m_Eye.x = normalizedLon;
            GetEarthView()->m_Eye.y = mercatorY;
            
            // Set TrackHook to highlight the aircraft
            TrackHook.ICAO_CC = foundAircraft->ICAO;
            TrackHook.Valid_CC = true;
            
            // Refresh the display
            ObjectDisplay->Repaint();
            
            // Show success message with coordinates for debugging
            AnsiString message = "Aircraft found: " + AnsiString(foundAircraft->HexAddr);
            if (foundAircraft->HaveFlightNum && strlen(foundAircraft->FlightNum) > 0) {
                message += " (" + AnsiString(foundAircraft->FlightNum).Trim() + ")";
            }
            message += "\nLat: " + FloatToStrF(foundAircraft->Latitude, ffFixed, 8, 4) + 
                      " Lon: " + FloatToStrF(foundAircraft->Longitude, ffFixed, 8, 4);
            message += "\nEye.x: " + FloatToStrF(GetEarthView()->m_Eye.x, ffFixed, 8, 6) +
                      " Eye.y: " + FloatToStrF(GetEarthView()->m_Eye.y, ffFixed, 8, 6);
            ShowMessage(message);
        }
    } else {
        // Aircraft not found
        ShowMessage("Aircraft with ICAO '" + searchInput + "' or callsign '" + searchInput + "' not found.");
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::AircraftNumberChange(TObject *Sender)
{
    // Optional: Real-time validation or formatting of input
    AnsiString inputText = AircraftNumber->Text.Trim();
    
    // Convert to uppercase for consistency (ICAO codes are typically uppercase)
    if (inputText != inputText.UpperCase()) {
        int cursorPos = AircraftNumber->SelStart;
        AircraftNumber->Text = inputText.UpperCase();
        AircraftNumber->SelStart = cursorPos;
    }
    
    // Optional: Enable/disable search button based on input
    // You can add validation logic here if needed
}
//---------------------------------------------------------------------------



void __fastcall TForm1::MilitaryClick(TObject *Sender)
{
    if (IsMilitary->Checked)
    {
        // Enable the search button when checkbox is checked
        DefaultFilter.activateFilter("military");
    }
    else
    {
        // Disable the search button when checkbox is unchecked
        DefaultFilter.deactivateFilter("military");
    }
}
//---------------------------------------------------------------------------

