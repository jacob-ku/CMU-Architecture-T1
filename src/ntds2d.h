//---------------------------------------------------------------------------

#ifndef NTDS2DH
#define NTDS2DH

#include <vector>

// Define region codes
const int RESION_INSIDE = 0; // 0000
const int RESION_LEFT = 1;   // 0001
const int RESION_RIGHT = 2;  // 0010
const int RESION_BOTTOM = 4; // 0100
const int RESION_TOP = 8;    // 1000

int MakeAirplaneImages(void);
void MakeAirTrackFriend(void);
void MakeAirTrackHostile(void);
void MakeAirTrackUnknown(void);
void MakePoint(void);
void MakeTrackHook(void);    // make circle for hooked trace
void DrawAirTrackFriend(float x, float y);
void DrawAirTrackHostile(float x, float y);
void DrawAirTrackUnknown(float x, float y);
void DrawPoint(float x, float y);
void DrawAirplaneImage(float x, float y, float scale, float heading, int imageNum);
void DrawTrackHook(float x, float y);   // draw circle made by MakeTrackHook
void DrawRadarCoverage(float xc, float yc, float major, float minor);
void DrawLeader(float x1, float y1, float x2, float y2);
void ComputeTimeToGoPosition(float TimeToGo,
                             float xs, float ys,
                             float xv, float yv,
                             float &xe, float &ye);
void DrawLines(DWORD resolution, double xpts[], double ypts[]);

// Function to compute the region code for a point
int computeRegionCode(double x, double y, double minX, double minY, double maxX, double maxY);

// Function to clip a line using the Cohen-Sutherland algorithm
std::vector<std::pair<double, double>> cohenSutherlandClip(
    double x1, double y1, double x2, double y2,
    double minX, double minY, double maxX, double maxY);

//---------------------------------------------------------------------------
#endif
