//---------------------------------------------------------------------------
#define _USE_MATH_DEFINES
#include <math.h>
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#pragma hdrstop

#include "ntds2d.h"
#define RADPERDEG (asin(1.0f) / 90.0f)
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
//---------------------------------------------------------------------------

#pragma package(smart_init)

#define PI 3.1415926535898
#define RADIUS 20.0
#define NUM_SPRITES 81
#define SPRITE_WIDTH 72
#define SPRITE_HEIGHT 72
#define SPRITE_CHANNELS 4

static GLuint AIR_TRACK_FRIEND;
static GLuint AIR_TRACK_HOSTILE;
static GLuint AIR_TRACK_UNKNOWN;
static GLuint SURFACE_TRACK_FRIEND;
static GLuint TRACKHOOK;

static GLuint TextureSpites[NUM_SPRITES];
static int NumSprites = 0;

//---------------------------------------------------------------------------
int MakeAirplaneImages(void)
{
    bool hasAlpha = false;
    const char filename[] = "..\\..\\Symbols\\sprites-RGBA.png";

    int width = 0;
    int height = 0;
    int nrChannels = 0;
    GLubyte *SpriteImage;
    GLubyte SpriteTexture[SPRITE_WIDTH][SPRITE_HEIGHT][SPRITE_CHANNELS];
    int x, y;

    NumSprites = 0;
    // stbi_set_flip_vertically_on_load(true);
    SpriteImage = stbi_load(filename, &width, &height, &nrChannels, 0);
    if (SpriteImage == NULL)
    {
        ShowMessage("Unable to load png file");
        return (0);
    }

    // ShowMessage("Image loaded "+IntToStr(width)+" "+IntToStr(height) + " channnels " +IntToStr(nrChannels));
    if (nrChannels == 4)
    {
        hasAlpha = true;
    }
    glGenTextures(NUM_SPRITES, TextureSpites);
    for (int row = 0; row < 11; row++)
    {
        for (int col = 0; col < 8; col++)
        {
            for (int x = 0; x < SPRITE_WIDTH; ++x)
            {
                for (int y = 0; y < SPRITE_HEIGHT; ++y)
                {
                    int index = (((y + row * SPRITE_HEIGHT) * width + x + (col * SPRITE_WIDTH))) * SPRITE_CHANNELS;
                    SpriteTexture[x][y][0] = SpriteImage[index];
                    SpriteTexture[x][y][1] = SpriteImage[index + 1];
                    SpriteTexture[x][y][2] = SpriteImage[index + 2];
#if 1           
                    GLubyte r = SpriteImage[index];
                    GLubyte g = SpriteImage[index + 1];
                    GLubyte b = SpriteImage[index + 2];
                    
                    int blackThreshold = 30;
                    
                    if (r <= blackThreshold && g <= blackThreshold && b <= blackThreshold) {
                        SpriteTexture[x][y][3] = 0;
                    } else {
                        if (hasAlpha) {
                            SpriteTexture[x][y][3] = SpriteImage[index + 3];
                        } else {
                            SpriteTexture[x][y][3] = 255;
                        }
                    }
#endif
                }
            }

            glBindTexture(GL_TEXTURE_2D, TextureSpites[NumSprites]);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SPRITE_WIDTH,
                         SPRITE_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         SpriteTexture);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
            NumSprites++;
            if (NumSprites == NUM_SPRITES)
            {
                stbi_image_free(SpriteImage);
                glEnable(GL_TEXTURE_2D);
                glShadeModel(GL_FLAT);
                return (NumSprites);
            }
        }
    }
    stbi_image_free(SpriteImage);
    glEnable(GL_TEXTURE_2D);
    glShadeModel(GL_FLAT);
    return (NumSprites);
}
//---------------------------------------------------------------------------
void MakeAirTrackFriend(void)
{
    GLuint i;
    GLfloat cosine, sine;
    AIR_TRACK_FRIEND = glGenLists(1);
    glNewList(AIR_TRACK_FRIEND, GL_COMPILE);
    glPointSize(3.0);
    glLineWidth(2.0);
#if 1
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

    glBegin(GL_LINE_STRIP);
    for (i = 0; i <= 50; i++)
    {
        cosine = cos(i * PI / 50.0) * RADIUS;
        sine = sin(i * PI / 50.0) * RADIUS;
        glVertex2f(cosine, sine);
    }
    glEnd();

    glBegin(GL_POINTS);
    glVertex2f(0.0, 0.0);
    glEnd();
    glEndList();
}
//---------------------------------------------------------------------------
void MakeAirTrackHostile(void)
{
    AIR_TRACK_HOSTILE = glGenLists(1);
    glNewList(AIR_TRACK_HOSTILE, GL_COMPILE);
    glPointSize(3.0);
    glLineWidth(2.0);

    glBegin(GL_LINE_STRIP);
    glVertex2f(-10.0, 0.0);
    glVertex2f(0.0, 10.0);
    glVertex2f(10.0, 0.0);
    glEnd();

    glBegin(GL_POINTS);
    glVertex2f(0.0, 0.0);
    glEnd();
    glEndList();
}
//---------------------------------------------------------------------------
void MakeAirTrackUnknown(void)
{
    AIR_TRACK_UNKNOWN = glGenLists(1);
    glNewList(AIR_TRACK_UNKNOWN, GL_COMPILE);
    glPointSize(3.0);
    glLineWidth(2.0);

    glBegin(GL_LINE_STRIP);
    glVertex2f(-10.0, 0.0);
    glVertex2f(-10.0, 10.0);
    glVertex2f(10.0, 10.0);
    glVertex2f(10.0, 0.0);
    glEnd();

    glBegin(GL_POINTS);
    glVertex2f(0.0, 0.0);
    glEnd();
    glEndList();
}
//---------------------------------------------------------------------------
void MakePoint(void)
{
    GLuint i;
    GLfloat cosine, sine;
    SURFACE_TRACK_FRIEND = glGenLists(1);
    glNewList(SURFACE_TRACK_FRIEND, GL_COMPILE);
    glPointSize(3.0);
    glLineWidth(3.0);
#if 1
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

    glBegin(GL_LINE_STRIP);
    for (i = 0; i < 100; i++)
    {
        cosine = cos(i * 2 * PI / 100.0) * RADIUS;
        sine = sin(i * 2 * PI / 100.0) * RADIUS;
        glVertex2f(cosine, sine);
    }
    glEnd();

    glBegin(GL_LINE_STRIP);
    for (i = 0; i < 100; i++)
    {
        cosine = cos(i * 2 * PI / 100.0) * 2.0;
        sine = sin(i * 2 * PI / 100.0) * 2.0;
        glVertex2f(cosine, sine);
    }
    glEnd();

    glEndList();
}
//---------------------------------------------------------------------------
void MakeTrackHook(void)     // make circle for hooked trace
{
    GLuint i;
    GLfloat cosine, sine;
    TRACKHOOK = glGenLists(1);
    glNewList(TRACKHOOK, GL_COMPILE);
    glPointSize(8.0);
    glLineWidth(10.0);
#if 1
    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
#endif

    glBegin(GL_LINE_STRIP);
    for (i = 0; i < 100; i++)
    {
        cosine = cos(i * 2 * PI / 100.0) * RADIUS * 3;
        sine = sin(i * 2 * PI / 100.0) * RADIUS * 3;
        glVertex2f(cosine, sine);
    }
    glEnd();
    glEndList();
}
//---------------------------------------------------------------------------
void DrawAirplaneImage(float x, float y, float scale, float heading, int imageNum)
{
    glPushMatrix();
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TextureSpites[imageNum]);
    glShadeModel(GL_FLAT);
    
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.1f);
    
    glTranslated(x, y, 0.0);
    glRotatef(-heading - 90.0, 0, 0, 1);
    
    glBegin(GL_QUADS);

    glTexCoord2f(1.0, 1.0);
    glVertex2f(36.0 * scale, 36.0 * scale);

    glTexCoord2f(0.0, 1.0);
    glVertex2f(-36.0 * scale, 36.0 * scale);

    glTexCoord2f(0.0, 0.0);
    glVertex2f(-36.0 * scale, -36.0 * scale);

    glTexCoord2f(1.0, 0.0);
    glVertex2f(36.0 * scale, -36.0 * scale);

    glEnd();
    
    glDisable(GL_ALPHA_TEST);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    
    glPopMatrix();
}
//---------------------------------------------------------------------------
void DrawAirTrackFriend(float x, float y)
{
    glPushMatrix();
    glTranslated(x, y, 0.0);
    glCallList(AIR_TRACK_FRIEND);
    glPopMatrix();
}
//---------------------------------------------------------------------------
void DrawAirTrackHostile(float x, float y)
{
    glPushMatrix();
    glTranslated(x, y, 0.0);
    glCallList(AIR_TRACK_HOSTILE);
    glPopMatrix();
}
//---------------------------------------------------------------------------
void DrawAirTrackUnknown(float x, float y)
{
    glPushMatrix();
    glTranslated(x, y, 0.0);
    glCallList(AIR_TRACK_UNKNOWN);
    glPopMatrix();
}
//---------------------------------------------------------------------------
void DrawPoint(float x, float y)
{
    glPushMatrix();
    glTranslated(x, y, 0.0);
    glCallList(SURFACE_TRACK_FRIEND);
    glPopMatrix();
}
//---------------------------------------------------------------------------
void DrawTrackHook(float x, float y)    // draw circle made by MakeTrackHook
{
    glPushMatrix();
    glTranslated(x, y, 0.0);
    glCallList(TRACKHOOK);
    glPopMatrix();
}
//---------------------------------------------------------------------------
void DrawRadarCoverage(float xc, float yc, float major, float minor)
{
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(xc, yc); // Center.
    for (float a = 0; a <= 360; a += 5)
    {
        float ang = (float)RADPERDEG * a;
        float x = xc + (float)(major * cos(ang));
        float y = yc + (float)(minor * sin(ang));
        glVertex2f(x, y);
    }
    glEnd();
}
//---------------------------------------------------------------------------
void DrawLeader(float x1, float y1, float x2, float y2)
{
    glBegin(GL_LINE_STRIP);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}
//---------------------------------------------------------------------------
void ComputeTimeToGoPosition(float TimeToGo,
                             float xs, float ys,
                             float xv, float yv,
                             float &xe, float &ye)
{
    xe = xs + (xv / 3600.00) * TimeToGo;
    ye = ys + (yv / 3600.00) * TimeToGo;
}
//---------------------------------------------------------------------------
void DrawLines(DWORD resolution, double xpts[], double ypts[])
{
    DWORD i;
    glBegin(GL_LINES);
    for (i = 0; i < resolution; i++)
    {
        glVertex3f(xpts[i], ypts[i], 0.1f);
        glVertex3f(xpts[(i + 1) % resolution], ypts[(i + 1) % resolution], 0.1f);
    }
    glEnd();
}
#if 0
//---------------------------------------------------------------------------
// SAVE ORIGINAL CODE   ** DO NOT DELETE
//
  void DrawCovarianceEllipse(float c[][2], float cx,float cy, float sf)
  {
   float a,b,term1,term2,term3,term4;
   float major, minor, theta;
   //find direction of maximum variance
   a= 2.0*c[1][0];
   b= (c[0][0]-c[1][1]);
   if ((a==0.0)&& (b==0.0)) return;

   theta=0.5*atan2(a,b);
   //% find major and minor axis amplitudes
   term1=(c[0][0]+c[1][1]);
   term2=(c[0][0]-c[1][1]) *
			  (c[0][0]-c[1][1]) +
			  4.0*c[1][0]*c[1][0];
   if (term2<0.0) return;
   term2=sqrt(term2);
   term3=0.5*(term1+term2);
   term4=0.5*(term1-term2);
   if ((term3<0.0)|| (term4<0.0)) return;
   major=sqrt(term3);
   minor=sqrt(term4);
   theta*=DEGPERRAD;
   _DrawEllipse(theta, major*3, minor*3,cx,cy,sf );
 }
//---------------------------------------------------------------------------
// SAVE ORIGINAL CODE   ** DO NOT DELETE
//
 //---------------------------------------------------------------------------
 void _DrawEllipse(float major_deg, float major_len, float minor_len, float cx,
				   float cy, float sf)
{
const int resolution = 100;
float xpts[resolution], ypts[resolution], newy[resolution], newx[resolution];
float theta, theta_step, major_rad;
int i;

	major_rad = major_deg * 3.1416f / 180.0f;
	theta_step = 2.0f * 3.1416f / (float)resolution;
	for (theta=0.0, i=0; i<resolution; i++, theta += theta_step)
	{
		xpts[i] = major_len * (float)cos(theta);
		ypts[i] = minor_len * (float)sin(theta);
	}
	for (i=0; i<resolution; i++)
	{
		newx[i] = (float)(cx + (xpts[i] * (float)cos(major_rad) - ypts[i] * (float)sin(major_rad)))*sf;
		newy[i] = (float)(cy + (xpts[i] * (float)sin(major_rad) + ypts[i] * (float)cos(major_rad)))*sf;
	}

	glBegin(GL_LINES);
	for (i=0; i<resolution; i++)
	{
		glVertex3f(newx[i], newy[i], 0.1f);
		glVertex3f(newx[(i+1)%resolution], newy[(i+1)%resolution], 0.1f);
	}
	//glVertex3f(newx[0], newy[0], 0.1f);
	//glVertex3f(newx[resolution/2], newy[resolution/2], 0.1f);
	//glVertex3f(newx[resolution/4], newy[resolution/4], 0.1f);
	//glVertex3f(newx[3*resolution/4], newy[3*resolution/4], 0.1f);
	glEnd();
}
 //---------------------------------------------------------------------------
#endif

// Region Code 계산 함수
int computeRegionCode(double x, double y, double minX, double minY, double maxX, double maxY) {
    int code = RESION_INSIDE;

    if (x < minX) code |= RESION_LEFT;
    else if (x > maxX) code |= RESION_RIGHT;

    if (y < minY) code |= RESION_BOTTOM;
    else if (y > maxY) code |= RESION_TOP;

    return code;
}

// Cohen-Sutherland 알고리즘
std::vector<std::pair<double, double>> cohenSutherlandClip(
    double x1, double y1, double x2, double y2,
    double minX, double minY, double maxX, double maxY) {
    
    int code1 = computeRegionCode(x1, y1, minX, minY, maxX, maxY);
    int code2 = computeRegionCode(x2, y2, minX, minY, maxX, maxY);

    std::vector<std::pair<double, double>> intersectionPoints;

    while (true) {
        if ((code1 | code2) == 0) {
            // 두 점이 모두 뷰포인트 내부에 있음
            break;
        } else if (code1 & code2) {
            // 두 점이 같은 외부 영역에 있음 (교차 없음)
            break;
        } else {
            // 직선이 뷰포인트와 교차함
            double x, y;

            // 뷰포인트 외부에 있는 점 선택
            int codeOut = code1 ? code1 : code2;

            // 교차점 계산
            if (codeOut & RESION_TOP) {
                x = x1 + (x2 - x1) * (maxY - y1) / (y2 - y1);
                y = maxY;
            } else if (codeOut & RESION_BOTTOM) {
                x = x1 + (x2 - x1) * (minY - y1) / (y2 - y1);
                y = minY;
            } else if (codeOut & RESION_RIGHT) {
                y = y1 + (y2 - y1) * (maxX - x1) / (x2 - x1);
                x = maxX;
            } else if (codeOut & RESION_LEFT) {
                y = y1 + (y2 - y1) * (minX - x1) / (x2 - x1);
                x = minX;
            }

            // 교차점이 뷰포인트 내부에 있는지 확인
            if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
                // 중복 교차점 제거
                if (intersectionPoints.empty() || 
                    (intersectionPoints.back().first != x || intersectionPoints.back().second != y)) {
                    intersectionPoints.emplace_back(x, y);
                }
            }

            // 교차점이 2개 계산되면 종료
            if (intersectionPoints.size() == 2) {
                break;
            }

            // 외부 점을 교차점으로 대체
            if (codeOut == code1) {
                x1 = x;
                y1 = y;
                code1 = computeRegionCode(x1, y1, minX, minY, maxX, maxY);
            } else {
                x2 = x;
                y2 = y;
                code2 = computeRegionCode(x2, y2, minX, minY, maxX, maxY);
            }
        }
    }

    return intersectionPoints;
}