//---------------------------------------------------------------------------

#ifndef DisplayGUIH
#define DisplayGUIH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "Components\OpenGLv0.5BDS2006\Component\OpenGLPanel.h"
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <Graphics.hpp>
#include "FilesystemStorage.h"
#include "KeyholeConnection.h"
#include "GoogleLayer.h"
#include "FlatEarthView.h"
#include "ght_hash_table.h"
#include "TriangulatPoly.h"
#include "TArea.h"
#include <Dialogs.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include "cspin.h"
#include "Map/Providers/MapProviderFactory.h"
#include "MetadataManager/AirportManager.h"
#include "MetadataManager/RouteManager.h"
#include "AircraftFilter/AircraftFilter.h"
#include "AircraftFilter/AirplaneFilterInterface.h"
#include "AircraftFilter/ZoneFilter.h"

#include <queue>
#include "ErrorHandling/PIErrorMonitor.h"

// Define message types for the central processor
enum class MessageType {
    SBS,   // SBS-formatted message ("Type 1,2,3" style)
    RAW    // Raw Mode-S/ADS-B message (hex payload)
};

typedef float T_GL_Color[4];

typedef struct
{
    bool Valid_CC;
    bool Valid_CPA;
    uint32_t ICAO_CC;
    uint32_t ICAO_CPA;
} TTrackHook;

#define MAX_AREA_POINTS 500


//---------------------------------------------------------------------------
class TMessageProcessorThread : public TThread
{
private:
    struct QueuedMessage {
        MessageType type;
        AnsiString  message;
    };
    std::queue<QueuedMessage> messageQueue;
    TCriticalSection* queueLock;
    TEvent* messageEvent;

public:
    __fastcall TMessageProcessorThread(bool value);
    ~TMessageProcessorThread();

    void __fastcall Execute(void);
    void AddMessage(MessageType type, const AnsiString& msg);    // takes a copy to ensure thread safety
    void wakeUp(void);
    void clearQueue(void);
};
//---------------------------------------------------------------------------
class DataRecieverThread : public TThread
{
private:

    virtual void __fastcall StopPlayback(void) = 0;
    virtual void __fastcall StopTCPClient(void) = 0; 

protected:
    AnsiString StringMsgBuffer;
    TMessageProcessorThread* msgProcThread;
virtual void __fastcall Execute(void) = 0;

public:
    bool UseFileInsteadOfNetwork;
    bool First;
    __int64 LastTime;
    double PlaybackSpeed; // 추가: RAW 재생속도 (1.0, 2.0, 3.0)
    __fastcall DataRecieverThread(bool value, TMessageProcessorThread* procThread, double playbackSpeed = 1.0);
    virtual ~DataRecieverThread();
};
//---------------------------------------------------------------------------
class TTCPClientRawHandleThread : public DataRecieverThread
{
private:
    void __fastcall StopPlayback(void);
    void __fastcall StopTCPClient(void);

protected:
    void __fastcall Execute(void);

public:
    __fastcall TTCPClientRawHandleThread(bool value, TMessageProcessorThread* procThread, double playbackSpeed = 1.0);
    ~TTCPClientRawHandleThread();
};
//---------------------------------------------------------------------------
class TTCPClientSBSHandleThread : public DataRecieverThread
{
private:
    AnsiString StringMsgBuffer;
    void __fastcall StopPlayback(void);
    void __fastcall StopTCPClient(void);

protected:
    void __fastcall Execute(void);

public:
    __fastcall TTCPClientSBSHandleThread(bool value, TMessageProcessorThread* procThread, double playbackSpeed = 1.0);
    ~TTCPClientSBSHandleThread();
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
    __published : // IDE-managed Components
                  TMainMenu *MainMenu1;
    TPanel *RightPanel;
    TMenuItem *File1;
    TMenuItem *Exit1;
    TTimer *Timer1;
    TOpenGLPanel *ObjectDisplay;
    TPanel *Panel1;
    TPanel *Panel3;
    TButton *ZoomIn;
    TButton *ZoomOut;
    TCheckBox *DrawMap;
    TCheckBox *PurgeStale;
    TTimer *Timer2;
    TCSpinEdit *CSpinStaleTime;
    TButton *PurgeButton;
    TListView *AreaListView;
    TButton *Insert;
    TButton *Delete;
    TButton *Complete;
    TButton *Cancel;
    TButton *RawConnectButton;
    TLabel *Label16;
    TLabel *Label17;
    TEdit *RawIpAddress;
    TIdTCPClient *IdTCPClientRaw;
    TSaveDialog *RecordRawSaveDialog;
    TOpenDialog *PlaybackRawDialog;
    TCheckBox *CycleImages;
    TPanel *Panel4;
    TLabel *CLatLabel;
    TLabel *CLonLabel;
    TLabel *SpdLabel;
    TLabel *HdgLabel;
    TLabel *AltLabel;
    TLabel *MsgCntLabel;
    TLabel *TrkLastUpdateTimeLabel;
    TLabel *Label14;
    TLabel *Label13;
    TLabel *Label10;
    TLabel *Label9;
    TLabel *Label8;
    TLabel *Label7;
    TLabel *Label6;
    TLabel *Label18;
    TLabel *FlightNumLabel;
    TLabel *ICAOLabel;
    TLabel *Label5;
    TLabel *Label4;
    TPanel *Panel5;
    TLabel *Lon;
    TLabel *Label3;
    TLabel *Lat;
    TLabel *Label2;
    TStaticText *SystemTime;
    TLabel *SystemTimeLabel;
    TLabel *ViewableAircraftCountLabel;
    TLabel *AircraftCountLabel;
    TLabel *Label11;
    TLabel *Label1;
    TButton *RawPlaybackButton;
    TButton *RawRecordButton;
    TIdTCPClient *IdTCPClientSBS;
    TButton *SBSConnectButton;
    TEdit *SBSIpAddress;
    TButton *SBSRecordButton;
    TButton *SBSPlaybackButton;
    TSaveDialog *RecordSBSSaveDialog;
    TOpenDialog *PlaybackSBSDialog;
    TTrackBar *TimeToGoTrackBar;
    TCheckBox *TimeToGoCheckBox;
    TStaticText *TimeToGoText;
    TLabel *Label12;
    TLabel *Label19;
    TLabel *CpaTimeValue;
    TLabel *CpaDistanceValue;
    TPanel *Panel2;
    TComboBox *MapComboBox;
    TCheckBox *CheckBoxUpdateMapTiles;
    TCheckBox *BigQueryCheckBox;
    TLabel *UnregisteredCount;
    TCheckBox *HideUnregisteredCheckBox;
    TMenuItem *UseSBSLocal;
    TMenuItem *UseSBSRemote;
    TMenuItem *LoadARTCCBoundaries1;
    TLabel *Label20;
    TLabel *RegNumLabel;
    TLabel *Label24;
    TLabel *OperatorLabel;
    TLabel *Label26;
    TLabel *CountryLabel;
    TLabel *LabelErrorMessage;
    TLabel *TypeLabel;
    TLabel *Label25;
    TLabel *RouteLabel;
    TLabel *Label27;
    TComboBox *SBSPlaybackSpeedComboBox; // SBS ��� �ӵ� ���� �޺��ڽ� (�����ο� �߰� �ʿ�)
    TButton *SearchAircraft; // Button for searching aircraft by number
    TEdit *AircraftNumber; // Edit for aircraft number input
    TButton *SearchAirport; // New button for searching airports
    TEdit *AirportCode; // New edit for airport code
    TCheckBox *IsMilitary;
    TComboBox *RawPlaybackSpeedComboBox;
    void __fastcall ObjectDisplayInit(TObject *Sender);
    void __fastcall ObjectDisplayResize(TObject *Sender);
    void __fastcall ObjectDisplayPaint(TObject *Sender);
    void __fastcall Timer1Timer(TObject *Sender);
    void __fastcall ResetXYOffset(void);
    void __fastcall ObjectDisplayMouseDown(TObject *Sender, TMouseButton Button,
                                           TShiftState Shift, int X, int Y);
    void __fastcall ObjectDisplayMouseMove(TObject *Sender, TShiftState Shift,
                                           int X, int Y);
    void __fastcall AddPoint(int X, int Y);
    void __fastcall ObjectDisplayMouseUp(TObject *Sender, TMouseButton Button,
                                         TShiftState Shift, int X, int Y);
    void __fastcall Exit1Click(TObject *Sender);
    void __fastcall ZoomInClick(TObject *Sender);
    void __fastcall ZoomOutClick(TObject *Sender);
    void __fastcall Timer2Timer(TObject *Sender);
    
    void __fastcall PurgeButtonClick(TObject *Sender);
    void __fastcall InsertClick(TObject *Sender);
    void __fastcall CancelClick(TObject *Sender);
    void __fastcall CompleteClick(TObject *Sender);
    void __fastcall AreaListViewSelectItem(TObject *Sender, TListItem *Item,
                                           bool Selected);
    void __fastcall DeleteClick(TObject *Sender);
    void __fastcall AreaListViewCustomDrawItem(TCustomListView *Sender,
                                               TListItem *Item, TCustomDrawState State, bool &DefaultDraw);
    void __fastcall FormMouseWheel(TObject *Sender, TShiftState Shift,
                                   int WheelDelta, TPoint &MousePos, bool &Handled);
    void __fastcall RawConnectButtonClick(TObject *Sender);
    void __fastcall IdTCPClientRawConnected(TObject *Sender);
    void __fastcall RawRecordButtonClick(TObject *Sender);
    void __fastcall RawPlaybackButtonClick(TObject *Sender);
    void __fastcall IdTCPClientRawDisconnected(TObject *Sender);
    void __fastcall CycleImagesClick(TObject *Sender);
    void __fastcall SBSConnectButtonClick(TObject *Sender);
    void __fastcall SBSRecordButtonClick(TObject *Sender);
    void __fastcall SBSPlaybackButtonClick(TObject *Sender);
    void __fastcall IdTCPClientSBSConnected(TObject *Sender);
    void __fastcall IdTCPClientSBSDisconnected(TObject *Sender);
    void __fastcall TimeToGoTrackBarChange(TObject *Sender);
    void __fastcall MapComboBoxChange(TObject *Sender);
    void __fastcall CheckBoxUpdateMapTilesClick(TObject *Sender);
    void __fastcall BigQueryCheckBoxClick(TObject *Sender);
    void __fastcall HideUnregisteredCheckBoxClick(TObject *Sender);
    void __fastcall UseSBSRemoteClick(TObject *Sender);
    void __fastcall UseSBSLocalClick(TObject *Sender);
    void __fastcall LoadARTCCBoundaries1Click(TObject *Sender);
    void __fastcall UseRawRouterClick(TObject *Sender);
    void __fastcall UseRawCmuSecureClick(TObject *Sender);
    void __fastcall UseRawHyattClick(TObject *Sender);
    void __fastcall SearchAircraftClick(TObject *Sender);
    void __fastcall AircraftNumberChange(TObject *Sender);
    void __fastcall SearchAirportClick(TObject *Sender);
    void __fastcall AirportCodeChange(TObject *Sender);
    void __fastcall MilitaryClick(TObject *Sender);

private: // User declarations
    MapProvider* currentMapProvider;
    bool LoadMapFromInternet;
    void ReloadMapProvider();

    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    AirportManager AirportMgr;
    RouteManager RouteMgr;
    AircraftFilter DefaultFilter;

    TMessageProcessorThread* msgProcThread; // single thread which has same lifecycle as the application
public:  // User declarations
    __fastcall TForm1(TComponent *Owner);
    __fastcall ~TForm1();
    void __fastcall LatLon2XY(double lat, double lon, double &x, double &y);    // convert lat/lon to x/y
    int __fastcall XY2LatLon2(int x, int y, double &lat, double &lon);          // convert x/y to lat/lon
    void __fastcall HookTrack(int X, int Y, bool CPA_Hoo);
    void __fastcall DrawObjects(void);
    void __fastcall DeleteAllAreas(void);
    void __fastcall Purge(void);
    void __fastcall SendCotMessage(AnsiString IP_address, unsigned short Port, char *Buffer, DWORD Length);
    void __fastcall RegisterWithCoTRouter(void);
    void __fastcall SetMapCenter(double &x, double &y);
    void __fastcall LoadMap(int Type);
    void __fastcall CreateBigQueryCSV(void);
    void __fastcall CloseBigQueryCSV(void);
    bool __fastcall LoadARTCCBoundaries(AnsiString FileName);
    void __fastcall UpdateUnregisteredCount(void);
    void __fastcall RawThreadTerminated(TObject *Sender);
    void __fastcall SBSThreadTerminated(TObject *Sender);

    void __fastcall DrawBlackDot(double lat, double lot);
    void __fastcall DrawAirportsBatch(void);
    bool LoadTowerTexture(void);
    void DrawTowerImage(float x, float y, float scale);
    float getCurrentZoomLevel(void);
    void getScreenLatLonBounds(double &minLat, double &maxLat, double &minLon, double &maxLon);
    void __fastcall HandlePIErrorState(const int &code);

    TArea getScreenBoundsAsArea();
    std::string AnsiStringToStdString(const AnsiString& ansiStr);
    void __fastcall AddAreaToFilter(TArea* area);
    int MouseDownX, MouseDownY;
    bool MouseDown;
    TTrackHook TrackHook;
    Vector3d Map_v[4], Map_p[4];
    Vector2d Map_w[2];
    double Mw1, Mw2, Mh1, Mh2, xf, yf;
    double MapCenterLat, MapCenterLon;
    int g_MouseLeftDownX;
    int g_MouseLeftDownY;
    int g_MouseDownMask;
    TList *Areas;
    TArea *AreaTemp;    // target area - area of interest
    ght_hash_table_t *HashTable;
    TTCPClientRawHandleThread *TCPClientRawHandleThread;
    TTCPClientSBSHandleThread *TCPClientSBSHandleThread;
    PIErrorMonitor *mPIErrorMonitorThread;
    TStreamWriter *RecordRawStream;
    TStreamReader *PlayBackRawStream;
    TStreamWriter *RecordSBSStream;
    TStreamReader *PlayBackSBSStream;
    TStreamWriter *BigQueryCSV;
    AnsiString BigQueryCSVFileName;
    unsigned int BigQueryRowCount;
    unsigned int BigQueryFileCount;
    AnsiString BigQueryPythonScript;
    AnsiString BigQueryPath;
    AnsiString BigQueryLogFileName;
    int NumSpriteImages;
    int CurrentSpriteImage;
    AnsiString AircraftDBPathFileName;
    AnsiString ARTCCBoundaryDataPathFileName;
    int UnregisteredAircraftCount;
    
    // Airport highlighting variables
    double HighlightedAirportLat;
    double HighlightedAirportLon;
    bool HighlightedAirportValid;
    FlatEarthView* GetEarthView() const;
    TileManager* GetTileManager() const;
    int GetUnregisteredAircraftCount(void);
    GLuint towerTextureID;
    bool towerTextureLoaded;
    AircraftFilter AreaFilter;

    void PurgeInternal(std::function<bool(TADS_B_Aircraft*)> shouldPurge);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------

#endif

