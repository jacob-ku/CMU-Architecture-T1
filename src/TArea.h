#ifndef TAREA_H
#define TAREA_H

#include "TriangulatPoly.h"
typedef struct
{
    double lat;
    double lon;
    double hae;
} TPolyLine;

#define MAX_AREA_POINTS 500

typedef struct
{
    AnsiString Name;
    TColor Color;
    DWORD NumPoints;
    pfVec3 Points[MAX_AREA_POINTS];
    pfVec3 PointsAdj[MAX_AREA_POINTS];
    TTriangles *Triangles;
    bool Selected;
} TArea;    // Target Area - area of interest

#endif // TAREA_H