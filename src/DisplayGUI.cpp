//---------------------------------------------------------------------------

#include <vcl.h>
#include <new>
#include <math.h>
#include <dir.h>
#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <fileapi.h>
#include <chrono>
#include <vector>
#include <algorithm>

#pragma hdrstop

#include "DisplayGUI.h"
#include "AreaDialog.h"
#include "ntds2d.h"
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
#include "Util/WebDownloadManager.h"

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
    AirportMgr.LoadAirport();
    RouteMgr.LoadRouteFromFile();
    RouteMgr.StartUpdateMonitor();

    AircraftDB = new TAircraftDB();
    AircraftDB->LoadDatabaseAsync(AircraftDBPathFileName);

//    RouteMgr.LoadRouteFromFile();

    std::string tmpsign = "KAL123";

//    RouteMgr.StartUpdateMonitor();
    
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
    printf("OpenGL Version %s\n", glGetString(GL_VERSION));
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
void __fastcall TForm1::DrawObjects(void)
{
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
    if (AreaTemp)
    {
        glPointSize(3.0);
        for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
            LatLon2XY(AreaTemp->Points[i][1], AreaTemp->Points[i][0],
                      AreaTemp->PointsAdj[i][0], AreaTemp->PointsAdj[i][1]);

        glBegin(GL_POINTS);
        for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
        {
            glVertex2f(AreaTemp->PointsAdj[i][0],
                       AreaTemp->PointsAdj[i][1]);
        }
        glEnd();
        glBegin(GL_LINE_STRIP);
        for (DWORD i = 0; i < AreaTemp->NumPoints; i++)
        {
            glVertex2f(AreaTemp->PointsAdj[i][0],
                       AreaTemp->PointsAdj[i][1]);
        }
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

        glColor4f(MC.Red / 255.0, MC.Green / 255.0, MC.Blue / 255.0, 0.4);

        for (j = 0; j < Area->NumPoints; j++)
        {
            LatLon2XY(Area->Points[j][1], Area->Points[j][0],
                      Area->PointsAdj[j][0], Area->PointsAdj[j][1]);
        }
        TTriangles *Tri = Area->Triangles;

        while (Tri)
        {
            glBegin(GL_TRIANGLES);
            glVertex2f(Area->PointsAdj[Tri->indexList[0]][0],
                       Area->PointsAdj[Tri->indexList[0]][1]);
            glVertex2f(Area->PointsAdj[Tri->indexList[1]][0],
                       Area->PointsAdj[Tri->indexList[1]][1]);
            glVertex2f(Area->PointsAdj[Tri->indexList[2]][0],
                       Area->PointsAdj[Tri->indexList[2]][1]);
            glEnd();
            Tri = Tri->next;
        }
    }

    // --- drawing aircrafts ---
    AircraftCountLabel->Caption = IntToStr((int)ght_size(HashTable));
    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if (Data->HaveLatLon)
        {
            if (HideUnregisteredCheckBox->Checked && !AircraftDB->aircraft_is_registered(Data->ICAO)) {
                continue;
            }

            ViewableAircraft++;
            glColor4f(1.0, 1.0, 1.0, 1.0);  // white color - is this necessary?

            LatLon2XY(Data->Latitude, Data->Longitude, ScrX, ScrY);
            // DrawPoint(ScrX,ScrY);
            if (!AircraftDB->aircraft_is_registered(Data->ICAO)) {
                glColor4f(0.5, 0.5, 0.5, 1.0);  // unregistered - gray color
            } else if (Data->HaveSpeedAndHeading) {
                glColor4f(0.0, 1.0, 0.0, 1.0);  // with speed & heading - basic color is Green
                if (Data->IsHelicopter) {
                    glColor4f(1.0, 0.41, 0.71, 1.0) ; // helicopter color is Pink
                }
                if (Data->IsMilitary) {
                    glColor4f(0.0, 0.0, 1.0, 1.0) ; // military color is Blue
                }
            } else {
                Data->Heading = 0.0;
                glColor4f(1.0, 0.0, 0.0, 1.0);  // no speed & heading - red
            }

            DrawAirplaneImage(ScrX, ScrY, 0.8, Data->Heading, Data->SpriteImage);   // Draw airplane image. scale is changed from 1.5 to 0.8

            // ICAO code text besides the aircraft
            glRasterPos2i(ScrX + 60, ScrY - 10);
            ObjectDisplay->Draw2DTextDefault(Data->HexAddr);

            // track age information below the ICAO code
            glColor4f(1.0, 0.0, 0.0, 1.0);  // red
            glRasterPos2i(ScrX + 60, ScrY - 25);    // TODO: location should be adjusted based on font size setting from the dfm file
            TimeDifferenceInSecToChar(Data->LastSeen, Data->TimeElapsedInSec, sizeof(Data->TimeElapsedInSec));
            ObjectDisplay->Draw2DTextAdditional(Data->TimeElapsedInSec + AnsiString(" seconds ago"));

            // heading line
            if ((Data->HaveSpeedAndHeading) && (TimeToGoCheckBox->State == cbChecked))
            {
                double lat, lon, az;
                if (VDirect(Data->Latitude, Data->Longitude,
                            Data->Heading, Data->Speed / 3060.0 * TimeToGoTrackBar->Position, &lat, &lon, &az) == OKNOERROR)
                {
                    double ScrX2, ScrY2;
                    LatLon2XY(lat, lon, ScrX2, ScrY2);
                    glColor4f(1.0, 1.0, 0.0, 1.0);  // yellow color for heading line
                    glBegin(GL_LINE_STRIP);
                    glVertex2f(ScrX, ScrY);
                    glVertex2f(ScrX2, ScrY2);
                    glEnd();
                }
            }
        }
    }

    // --- side bar - contents for text box: Close Control which is for hooked airplane ----
    ViewableAircraftCountLabel->Caption = ViewableAircraft;
    if (TrackHook.Valid_CC)
    {
        Data = (TADS_B_Aircraft *)ght_get(HashTable, sizeof(TrackHook.ICAO_CC), (void *)&TrackHook.ICAO_CC);
        if (Data)
        {
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

    // --- side bar - contents for text box: CPA when two aircraft are hooked ----
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
    int X1, Y1;
    double VLat, VLon;
    int i;
    Y1 = (ObjectDisplay->Height - 1) - Y;
    X1 = X;
    if ((X1 >= Map_v[0].x) && (X1 <= Map_v[1].x) &&
        (Y1 >= Map_v[0].y) && (Y1 <= Map_v[3].y))

    {
        pfVec3 Point;
        VLat = atan(sinh(M_PI * (2 * (Map_w[1].y - (yf * (Map_v[3].y - Y1)))))) * (180.0 / M_PI);
        VLon = (Map_w[1].x - (xf * (Map_v[1].x - X1))) * 360.0;
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
void __fastcall TForm1::HookTrack(int X, int Y, bool CPA_Hook)
{
    double VLat, VLon, dlat, dlon, Range;
    int X1, Y1;
    uint32_t *Key;

    uint32_t Current_ICAO;
    double MinRange;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data;

    Y1 = (ObjectDisplay->Height - 1) - Y;
    X1 = X;

    if ((X1 < Map_v[0].x) || (X1 > Map_v[1].x) ||
        (Y1 < Map_v[0].y) || (Y1 > Map_v[3].y))
        return;

    VLat = atan(sinh(M_PI * (2 * (Map_w[1].y - (yf * (Map_v[3].y - Y1)))))) * (180.0 / M_PI);
    VLon = (Map_w[1].x - (xf * (Map_v[1].x - X1))) * 360.0;

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
    if (MinRange < 0.2)
    {
        TADS_B_Aircraft *ADS_B_Aircraft = (TADS_B_Aircraft *)
            ght_get(HashTable, sizeof(Current_ICAO),
                    &Current_ICAO);
        if (ADS_B_Aircraft)
        {
            if (!CPA_Hook)
            {
                TrackHook.Valid_CC = true;
                TrackHook.ICAO_CC = ADS_B_Aircraft->ICAO;
                const TAircraftData* acData = AircraftDB->GetAircraftDBInfo(ADS_B_Aircraft->ICAO);
                if (acData) {
                    printf("%s\n\n", acData->toString().c_str());
                    RegNumLabel->Caption = acData->Registration.IsEmpty() ? "Unknown" : acData->Registration;
                    ManufactureLabel->Caption = acData->ManufacturerName.IsEmpty() ? "Unknown" : acData->ManufacturerName;
                    ModelLabel->Caption = acData->Model.IsEmpty() ? "Unknown" : acData->Model;
                    OperatorLabel->Caption = acData->OperatorName.IsEmpty() ? "Unknown" : acData->OperatorName;

                    // Get country information using ICAO address
                    const char* country = AircraftDB->GetCountry(ADS_B_Aircraft->ICAO, false);
                    if (country)
                        CountryLabel->Caption = country;
                    else
                        CountryLabel->Caption = "Unknown";
                } else {
                    printf("No AircraftDB info\n\n");
                    RegNumLabel->Caption = "N/A";
                    ManufactureLabel->Caption = "N/A";
                    ModelLabel->Caption = "N/A";
                    OperatorLabel->Caption = "N/A";
                    CountryLabel->Caption = "N/A";
                }
            }
            else
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
    double Lat, Lon, dlat, dlon, Range;
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
void __fastcall TForm1::Purge(void)
{
    uint32_t *Key;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data;
    void *p;
    __int64 CurrentTime = GetCurrentTimeInMsec();
    __int64 StaleTimeInMs = CSpinStaleTime->Value * 1000;

    if (PurgeStale->Checked == false)
        return;

    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {
        if ((CurrentTime - Data->LastSeen) >= StaleTimeInMs)
        {
            p = ght_remove(HashTable, sizeof(*Key), Key);
            ;
            if (!p)
                ShowMessage("Removing the current iterated entry failed! This is a BUG\n");

            delete Data;
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm1::Timer2Timer(TObject *Sender)
{
    Purge();
}
//---------------------------------------------------------------------------
void __fastcall TForm1::PurgeButtonClick(TObject *Sender)
{
    uint32_t *Key;
    ght_iterator_t iterator;
    TADS_B_Aircraft *Data;
    void *p;

    for (Data = (TADS_B_Aircraft *)ght_first(HashTable, &iterator, (const void **)&Key);
         Data; Data = (TADS_B_Aircraft *)ght_next(HashTable, &iterator, (const void **)&Key))
    {

        p = ght_remove(HashTable, sizeof(*Key), Key);
        if (!p)
            ShowMessage("Removing the current iterated entry failed! This is a BUG\n");

        delete Data;
    }
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
            TCPClientRawHandleThread = new TTCPClientRawHandleThread(true);
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
                    TCPClientRawHandleThread = new TTCPClientRawHandleThread(true);
                    TCPClientRawHandleThread->UseFileInsteadOfNetwork = true;
                    TCPClientRawHandleThread->First = true;
                    TCPClientRawHandleThread->FreeOnTerminate = TRUE;
                    TCPClientRawHandleThread->OnTerminate = RawThreadTerminated;
                    TCPClientRawHandleThread->Resume();
                    RawPlaybackButton->Caption = "Stop Raw Playback";
                    RawConnectButton->Enabled = false;
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
    }
}
//---------------------------------------------------------------------------
// Constructor for the thread class
__fastcall TTCPClientRawHandleThread::TTCPClientRawHandleThread(bool value) : TThread(value)
{
	printf("[Thread] TTCPClientRawHandleThread created.\n");
	FreeOnTerminate = true; // Automatically free the thread object after execution
    processorThread = new TMessageProcessorThread(true);
    processorThread->Start();
}
//---------------------------------------------------------------------------
// Destructor for the thread class
__fastcall TTCPClientRawHandleThread::~TTCPClientRawHandleThread()
{
	printf("[Thread] TTCPClientRawHandleThread destroyed.\n");
	// Clean up resources if needed
    if (processorThread) {
        processorThread->Terminate();
        processorThread->WaitFor();
        delete processorThread;
        processorThread = NULL;
    }
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
                if (SleepTime > 0)
                    Sleep(SleepTime);
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
            processorThread->AddMessage(MessageType::RAW, StringMsgBuffer);
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
            TCPClientSBSHandleThread = new TTCPClientSBSHandleThread(true);
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
__fastcall TTCPClientSBSHandleThread::TTCPClientSBSHandleThread(bool value) : TThread(value)
{
	printf("[Thread] TTCPClientSBSHandleThread created.\n");
	FreeOnTerminate = true; // Automatically free the thread object after execution
    processorThread = new TMessageProcessorThread(true);
    processorThread->Start();
}
//---------------------------------------------------------------------------
// Destructor for the thread class
__fastcall TTCPClientSBSHandleThread::~TTCPClientSBSHandleThread()
{
	printf("[Thread] TTCPClientSBSHandleThread destroyed.\n");
	// Clean up resources if needed
    if (processorThread) {
        processorThread->Terminate();
        processorThread->WaitFor();
        delete processorThread;
        processorThread = NULL;
    }
}
//---------------------------------------------------------------------------
// Execute method where the thread's logic resides
void __fastcall TTCPClientSBSHandleThread::Execute(void)
{
    __int64 Time, SleepTime;
    AnsiString SBSMsg;
    while (!Terminated)
    {
        if (!UseFileInsteadOfNetwork)
        {
            try
            {
                if (!Form1->IdTCPClientSBS->Connected())
                    Terminate();

                // Receive message from Hub
                SBSMsg = Form1->IdTCPClientSBS->IOHandler->ReadLn();
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

                // Read timestamp frist
                AnsiString TimestampMsg = Form1->PlayBackSBSStream->ReadLine();
                Time = StrToInt64(TimestampMsg);
                if (First)
                {
                    First = false;
                    LastTime = Time;
                }
                SleepTime = Time - LastTime;
                LastTime = Time;
                if (SleepTime > 0)
                    Sleep(SleepTime);
                if (Form1->PlayBackSBSStream->EndOfStream)
                {
                    printf("End SBS Playback 2\n");
                    TThread::Synchronize(StopPlayback);
                    break;
                }

                // Read SBS message
                SBSMsg = Form1->PlayBackSBSStream->ReadLine();
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
            processorThread->AddMessage(MessageType::SBS, SBSMsg);
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
	messageEvent = new TEvent(nullptr, false, false, "", false);
	isProcessing = false;
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
                    TCPClientSBSHandleThread = new TTCPClientSBSHandleThread(true);
                    TCPClientSBSHandleThread->UseFileInsteadOfNetwork = true;
                    TCPClientSBSHandleThread->First = true;
                    TCPClientSBSHandleThread->FreeOnTerminate = TRUE;
                    TCPClientSBSHandleThread->OnTerminate = SBSThreadTerminated;
                    TCPClientSBSHandleThread->Resume();
                    SBSPlaybackButton->Caption = "Stop SBS Playback";
                    SBSConnectButton->Enabled = false;
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
    UnregisteredCountLabel->Caption = "Unregistered: " + IntToStr(UnregisteredAircraftCount);
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

    int result = MessageDlg("Confirm exit: are you sure you want to close the program?",
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
    glColor4f(0.0, 0.0, 0.0, 1.0);
    
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
        
        if (airportLat >= minLatFromCorners && airportLat <= maxLatFromCorners && 
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
    if (!LoadTowerTexture()) {
        return; 
    }
    
    glPushMatrix();
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glBindTexture(GL_TEXTURE_2D, towerTextureID);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); 
    
    glTranslatef(x, y, 0.0f);
    
    
    float size = 32.0f * scale;
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(-size/2, size/2); 
    glTexCoord2f(1.0f, 1.0f); glVertex2f(size/2, size/2);  
    glTexCoord2f(1.0f, 0.0f); glVertex2f(size/2, -size/2); 
    glTexCoord2f(0.0f, 0.0f); glVertex2f(-size/2, -size/2);
    glEnd();
    
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
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
