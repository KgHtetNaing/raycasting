//|____________________________________________________________________
//|
//| Simple Ray Caster 
//|
//| Copyright 2026 - Mores Prachyabrued
//|
//| This source code implements the techniques described in the book
//| "Tricks of the Game Programming Gurus", Chapter 6. It also incorporates
//| my own additions such as texture mapping and in-program setting of
//| screen resolutions and player's FOV.
//| 
//| NOTE: No permission is given for distribution beyond
//|       the class, of this file or any derivative works.
//|____________________________________________________________________

//|___________________
//|
//| Defines
//|___________________

// Suppress Visual Studio compiler warnings for older CRT functions (such as strcpy)
#if _MSC_VER >= 1400
#define _CRT_SECURE_NO_DEPRECATE				
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#define USE_ADVANCED_PLAYER_TRANSLATION           // Enables advanced movement mechanics
                                                  // Player can slide along walls
#define USE_DOUBLE_BUFFERING                      // Enables double buffering

//#define SHOW_DEBUG_INFO                           // Shows debug information

//|___________________
//|
//| Includes
//|___________________

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include <GL/glut.h>

//|___________________
//|
//| Type Definitions
//|___________________

// Supported screen resolutions
enum ScreenResolution {
    SRES_320x200 = 0, SRES_320x240, SRES_640x480, SRES_800x600, SRES_1024x768, SRES_NB
};

// Supported FOVs
enum PlayerFOV { PFOV_45 = 0, PFOV_60, PFOV_90, PFOV_NB };

// Supported cell type
enum CellType { CT_EMPTY = 0, CT_NONEMPTY };

//|___________________
//|
//| Constants
//|___________________

// Small and large values
const double PI = 3.141592654;  // PI
const double SHIFTRAD = 3.272e-4;     // Very small angle (radian) TODO: May not need!
const double LONG_DISTANCE = 1e8;          // Very long distance

// There are 16 * 16 cells, with each occupying 64 * 64 units
const int CELLX = 16;           // Number of cells in the x direction
const int CELLY = 16;           // Number of cells in the y direction
const int CELL_WIDTH = 64;           // Cell width
const int CELL_HEIGHT = CELL_WIDTH;   // Cell height (square cell)
const int CALCY = CELLY - 1;
const int CELLW_MOD = 63;
const int CELLH_MOD = 63;
const int CELLW_MASK = ~63;          // for optimizations
const int CELLH_MASK = ~63;
const int SAFE_DIST_FROM_WALL = 4;            // Safe player's distance from walls

// Insanity Mechanic
float fInsanity = 100.0f;       // Starts at 100
float fMaxInsanity = 100.0f;    // The maximum size of the bar

const int NUM_MAPS = 3;

// Game map: 0 = empty cell, >= 1 cell with textured walls
// All map variations
const int aMaps[NUM_MAPS][CELLY][CELLX] = {
    // Variation 1
    { {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
      {3,0,0,0,1,0,0,0,0,0,0,0,0,0,0,1},
      {1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,3},
      {1,0,0,1,0,0,0,0,0,0,0,1,0,1,1,1},
      {1,1,1,1,0,0,0,0,0,1,0,1,0,1,0,1},
      {1,0,0,0,0,0,0,0,0,1,0,1,1,1,0,1},
      {1,0,0,0,1,1,1,0,0,1,0,0,0,0,0,1},
      {1,0,0,0,1,0,0,0,0,1,0,1,1,1,0,1},
      {1,0,0,0,1,0,0,0,0,1,0,0,0,1,1,1},
      {1,0,1,1,1,0,1,1,1,1,0,0,0,1,0,2},
      {1,0,0,0,1,0,0,0,0,1,0,0,0,1,0,1},
      {1,0,0,0,1,0,0,0,0,1,0,1,1,1,0,1},
      {1,1,1,1,1,0,0,1,0,0,0,0,0,0,0,1},
      {1,0,0,0,1,0,0,1,1,1,0,1,1,0,0,1},
      {3,0,0,0,0,0,0,1,0,0,0,0,1,0,0,1},
      {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} },

      // Variation 2
      { {1,3,1,1,1,1,1,1,1,1,1,1,3,1,1,1},
      {1,0,0,1,0,0,0,0,0,0,1,0,0,0,0,1},
      {1,0,1,1,0,1,1,1,1,0,1,1,1,1,0,1},
      {1,0,1,0,0,0,0,1,0,0,0,0,0,1,0,1},
      {1,0,1,0,0,0,0,1,0,0,0,2,1,1,0,1},
      {1,0,1,1,1,1,0,1,1,1,1,1,0,0,0,1},
      {1,0,0,0,0,0,0,0,0,0,0,1,1,1,0,1},
      {1,0,0,1,1,1,1,1,1,0,0,1,0,0,0,1},
      {1,0,0,0,1,0,0,0,1,0,0,1,0,1,1,1},
      {1,1,1,0,1,0,0,0,1,0,0,1,0,1,0,1},
      {1,0,0,0,1,0,1,0,1,0,0,0,0,1,0,1},
      {1,0,1,0,1,0,1,0,1,1,1,1,1,1,0,1},
      {1,0,1,0,0,0,1,0,0,1,0,0,0,0,0,1},
      {1,0,1,1,1,0,1,0,0,1,0,1,0,1,1,1},
      {1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1},
      {1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1} },

      // Variation 3
      { {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
          {1,0,1,0,0,0,0,1,0,0,1,0,0,0,0,1},
          {1,0,1,0,0,1,1,1,0,0,2,0,1,1,1,1},
          {1,0,1,0,0,0,0,1,0,0,0,0,0,0,0,1},
          {1,0,1,0,0,0,0,0,1,1,1,1,1,0,0,1},
          {1,0,1,1,1,1,0,0,0,1,0,0,1,0,1,1},
          {1,0,0,0,0,0,0,0,0,1,1,0,0,0,0,1},
          {1,0,0,1,1,1,1,1,0,0,0,0,1,0,0,1},
          {3,0,0,1,0,0,0,0,0,1,0,0,1,1,1,1},
          {1,1,1,1,0,1,0,0,0,1,0,0,0,0,0,1},
          {1,0,0,0,0,1,0,0,0,1,1,0,1,1,0,1},
          {1,0,0,0,1,1,1,0,0,1,0,0,1,0,0,1},
          {1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,3},
          {1,1,1,1,1,0,1,0,1,1,0,1,0,0,0,1},
          {1,0,0,0,0,0,1,0,0,0,0,1,0,0,0,1},
          {1,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1} }
};

// The active game map used by the raycaster & collision detection
int aMap[CELLY][CELLX];

const int aSliverScale[SRES_NB] =               // Scale slivers to appropriate size
{ 12500, 12500, 20000, 22500, 25000 };
const int aScrSize[SRES_NB][2] =               // Supported screen dimensions
{ {320,200}, {320,240}, {640,480}, {800, 600}, { 1024,768 } };
const int aFov[PFOV_NB] =               // Supported FOV (degrees)
{ 45,60,90 };

const int TURN_ANGLE = 5;           // Angle for left and right turns (degrees)
const int FORWARD_TRAN = 25;           // Translation for movement (world units)

// Colors defined using RGB tuples
const unsigned char WHITE[3] = { 255, 255, 255 };
const unsigned char GREEN[3] = { 0, 255,   0 };
const unsigned char BLUE[3] = { 0,   0, 255 };
const unsigned char GRAY[3] = { 128, 128, 128 };
const unsigned char DYELLOW[3] = { 150, 142, 59 };     // Dark Yellow
const unsigned char YELLOW[3] = { 182, 180, 97 };      // Yellow
const unsigned char DGRAY[3] = { 64,  64,  64 };      // Dark gray

// Wall textures
const char WALLTEXTURES_IMGFILE[] = "wall_textures.ppm"; // Wall texture image file
const int WALLTEXTURES_NB = 4;                   // Number of distinct wall textures
const int WALLTEX_WIDTH = CELL_WIDTH;          // Square wall textures
const int WALLTEX_HEIGHT = CELL_WIDTH;
const int WALLTEX_IMGWIDTH = WALLTEXTURES_NB * WALLTEX_WIDTH;

//|___________________
//|
//| Global Variables
//|___________________

int FOV;                  // Field of view (currently selected)
int SCREENX;              // Screen width (currently selected)
int SCREENY;              // Screen height
int SCRX_1;               // Screen dimension - 1
int SCRY_1;
int SCR_CENTERY;          // Y coordinate of the screen center 
double DEG_PER_RAY;       // Angle between two adjacent rays (degrees)
int g_damageFlashFrames = 0; // Number of frames to keep flashing

bool keys[256] = { false };

const int ANGLE0 = 0;
int ANGLE90;
int ANGLE180;
int ANGLE270;
int ANGLE360;
int FOVANGLE;
int ANGLETURN;

// Math Tables
double* aSlope = NULL;
double* aInvSlope = NULL;
double* aCos = NULL;
double* aInvCos = NULL;
double* aSin = NULL;
double* aInvSin = NULL;
double* aXStep = NULL;
double* aYStep = NULL;
double* aRectify = NULL;

GLubyte* pFrameBuffer = NULL;
double* ZBuffer = NULL;

const int NUM_ALMOND_WATER = 3;

struct Sprite {
    double x, y;
    bool active;
};

// Array of 3 Almond Water bottles
Sprite almondWaters[NUM_ALMOND_WATER];

enum MonsterState { LURK, STALK, CHASE, VANISH };

struct Monster {
    double x, y;
    bool active;
    MonsterState state;
};

Monster stalker = { 0.0, 0.0, true, LURK };

// Player pose     
double dXp;
double dYp;
int iAngle;

int iScrIndex = SRES_1024x768;
int iFovIndex = PFOV_60;

bool bDoTexture = true;
unsigned char* walltex_imgdata = NULL;

unsigned char* almondwater_imgdata = NULL;
unsigned int aw_w = 0;
unsigned int aw_h = 0;

// Monster textures
unsigned char* monster_imgdata = NULL;
unsigned int monster_w = 0;
unsigned int monster_h = 0;

struct PointLight {
    double x, y;
};
const PointLight g_Lights[] = {
    { 2 * CELL_WIDTH + 32.0, 3 * CELL_HEIGHT + 32.0 },
    { 6 * CELL_WIDTH + 32.0, 3 * CELL_HEIGHT + 32.0 },
    { 10 * CELL_WIDTH + 32.0, 3 * CELL_HEIGHT + 32.0 },
    { 14 * CELL_WIDTH + 32.0, 3 * CELL_HEIGHT + 32.0 },

    { 2 * CELL_WIDTH + 32.0, 7 * CELL_HEIGHT + 32.0 },
    { 6 * CELL_WIDTH + 32.0, 7 * CELL_HEIGHT + 32.0 },
    { 10 * CELL_WIDTH + 32.0, 7 * CELL_HEIGHT + 32.0 },
    { 14 * CELL_WIDTH + 32.0, 7 * CELL_HEIGHT + 32.0 },

    { 2 * CELL_WIDTH + 32.0, 11 * CELL_HEIGHT + 32.0 },
    { 6 * CELL_WIDTH + 32.0, 11 * CELL_HEIGHT + 32.0 },
    { 10 * CELL_WIDTH + 32.0, 11 * CELL_HEIGHT + 32.0 },
    { 14 * CELL_WIDTH + 32.0, 11 * CELL_HEIGHT + 32.0 }
};
const int g_NumLights = sizeof(g_Lights) / sizeof(g_Lights[0]);

bool g_started = false;

//|___________________
//|
//| Function Prototypes
//|___________________

void ResetGame();
void InitProgram();
void CalcRCParams(const int iFov, const int iScrX, const int iScrY, const int iSliverScale);
void SetFrameBuffer(const int iScrX, const int iScrY);
void Error(const char* szMsg);
void RayCaster(const int iXp, const int iYp, const int iAngle);
void DrawSolidSliver(const int x, const int yt, const int yb,
    const unsigned char r, const unsigned char g, const unsigned char b, double intensity);
void DrawTexturedSliver(const int x, const int yt, const int yb, const int tid, const int tc, double intensity);
void RenderString(float x, float y, const char* str);
void RenderStringUI(int x, int y, const char* str);
void DisplayFunc();
void KeyboardFunc(unsigned char key, int x, int y);
void KeyboardUpFunc(unsigned char key, int x, int y);
void LoadPPM(const char* fname, unsigned int* w, unsigned int* h, unsigned char** data, const int mallocflag);
void ReshapeFunc(int w, int h);
void TimerFunc(int value);
double CalcLightIntensity(double worldX, double worldY);
bool IsLight(int cx, int cy);
void DrawCeilingSliver(const int x, const int yt, const int yb, const int iRayID, const int iRay);
void DrawFloorSliver(const int x, const int yt, const int yb, const int iRayID, const int iRay);
void DrawSprite(Sprite& sprite);
void DrawTexturedSpriteSliver(const int x, const int yt, const int yb, const int texCol, unsigned char* imgdata, int imgW, int imgH, double intensity);
void DrawMonster(Monster& m);

// Helpers
void RenderString(float x, float y, const char* str) {
    glRasterPos2f(x, y);
    for (const char* c = str; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

void RenderStringUI(int x, int y, const char* str) {
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2i(x, y);
    for (const char* c = str; *c != '\0'; ++c) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }
}

// Entry Point
int main(int argc, char** argv)
{
    glutInit(&argc, argv);
#ifdef USE_DOUBLE_BUFFERING
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
#else
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
#endif  
    glutInitWindowSize(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);
    glutCreateWindow("Guten Tag! Ray Caster");

    InitProgram();

    glutDisplayFunc(DisplayFunc);
    glutKeyboardFunc(KeyboardFunc);
    glutReshapeFunc(ReshapeFunc);
    glutKeyboardUpFunc(KeyboardUpFunc);

    glutPostRedisplay();
    glutTimerFunc(50, TimerFunc, 0);

    glutMainLoop();
    return 0;
}

//|____________________________________________________________________
//|
//| Function: ResetGame
//| Resets the map, player position, monster, pickups, and sanity bar.
//|____________________________________________________________________

void ResetGame()
{
    int selectedMap = rand() % NUM_MAPS;
    memcpy(aMap, aMaps[selectedMap], sizeof(aMap));
    printf("Loaded Map Variation #%d\n", selectedMap + 1);

    fInsanity = fMaxInsanity;

    if (selectedMap == 0) {
        dXp = (2 * CELL_WIDTH) + 32;
        dYp = (4 * CELL_HEIGHT) + 32;
        stalker.x = (14 * CELL_WIDTH) + 32.0;
        stalker.y = (4 * CELL_HEIGHT) + 32.0;

        almondWaters[0] = { (6 * CELL_WIDTH) + 32.0,  (10 * CELL_HEIGHT) + 32.0, true };
        almondWaters[1] = { (13 * CELL_WIDTH) + 32.0, (1 * CELL_HEIGHT) + 32.0, true };
        almondWaters[2] = { (1 * CELL_WIDTH) + 32.0,  (14 * CELL_HEIGHT) + 32.0, true };
    }
    else if (selectedMap == 1) {
        dXp = (14 * CELL_WIDTH) + 32;
        dYp = (1 * CELL_HEIGHT) + 32;
        stalker.x = (10 * CELL_WIDTH) + 32.0;
        stalker.y = (7 * CELL_HEIGHT) + 32.0;

        almondWaters[0] = { (4 * CELL_WIDTH) + 32.0,  (3 * CELL_HEIGHT) + 32.0, true };
        almondWaters[1] = { (11 * CELL_WIDTH) + 32.0, (12 * CELL_HEIGHT) + 32.0, true };
        almondWaters[2] = { (2 * CELL_WIDTH) + 32.0,  (14 * CELL_HEIGHT) + 32.0, true };
    }
    else {
        dXp = (6 * CELL_WIDTH) + 32;
        dYp = (12 * CELL_HEIGHT) + 32;
        stalker.x = (10 * CELL_WIDTH) + 32.0;
        stalker.y = (2 * CELL_HEIGHT) + 32.0;

        almondWaters[0] = { (8 * CELL_WIDTH) + 32.0,  (7 * CELL_HEIGHT) + 32.0, true };
        almondWaters[1] = { (3 * CELL_WIDTH) + 32.0,  (1 * CELL_HEIGHT) + 32.0, true };
        almondWaters[2] = { (13 * CELL_WIDTH) + 32.0, (12 * CELL_HEIGHT) + 32.0, true };
    }
    stalker.active = true;
    iAngle = ANGLE90;
    g_damageFlashFrames = 0;
}

void InitProgram()
{
    unsigned int w, h;

    srand((unsigned int)time(NULL));

    CalcRCParams(aFov[iFovIndex], aScrSize[iScrIndex][0], aScrSize[iScrIndex][1], aSliverScale[iScrIndex]);
    SetFrameBuffer(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);

    LoadPPM(WALLTEXTURES_IMGFILE, &w, &h, &walltex_imgdata, 1);
    if ((w != WALLTEX_IMGWIDTH) || (h != WALLTEX_HEIGHT)) {
        Error("InitProgram: Invalid texture image dimensions");
    }

    LoadPPM("almondwater.ppm", &aw_w, &aw_h, &almondwater_imgdata, 1);
    LoadPPM("captClark.ppm", &monster_w, &monster_h, &monster_imgdata, 1);

    ResetGame();
}

void CalcRCParams(const int iFov, const int iScrX, const int iScrY, const int iSliverScale)
{
    int iRay;
    double dRadian;

    FOV = iFov;
    SCREENX = iScrX;
    SCREENY = iScrY;
    SCRX_1 = SCREENX - 1;
    SCRY_1 = SCREENY - 1;
    SCR_CENTERY = SCREENY / 2;
    DEG_PER_RAY = ((double)FOV) / SCREENX;

    ANGLE90 = (int)(90 / DEG_PER_RAY);
    ANGLE180 = (int)(180 / DEG_PER_RAY);
    ANGLE270 = (int)(270 / DEG_PER_RAY);
    ANGLE360 = (int)(360 / DEG_PER_RAY);
    FOVANGLE = (int)(FOV / DEG_PER_RAY);
    ANGLETURN = (int)(TURN_ANGLE / DEG_PER_RAY);

    if (aSlope) { free(aSlope); }
    if (aInvSlope) { free(aInvSlope); }
    if (aCos) { free(aCos); }
    if (aInvCos) { free(aInvCos); }
    if (aSin) { free(aSin); }
    if (aInvSin) { free(aInvSin); }
    if (aXStep) { free(aXStep); }
    if (aYStep) { free(aYStep); }
    if (aRectify) { free(aRectify); }

    if (!(aSlope = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aSlope"); }
    if (!(aInvSlope = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aInvSlope"); }
    if (!(aCos = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aCos"); }
    if (!(aInvCos = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aInvCos"); }
    if (!(aSin = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aSin"); }
    if (!(aInvSin = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aInvSin"); }
    if (!(aXStep = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aXStep"); }
    if (!(aYStep = (double*)malloc(ANGLE360 * sizeof(double)))) { Error("CalcRCParams: aYStep"); }
    if (!(aRectify = (double*)malloc(FOVANGLE * sizeof(double)))) { Error("CalcRCParams: aRectify"); }

    for (iRay = 0; iRay < ANGLE360; iRay++) {
        dRadian = SHIFTRAD + (iRay * 2 * PI / ANGLE360);

        aSlope[iRay] = tan(dRadian);
        aInvSlope[iRay] = 1 / aSlope[iRay];
        aCos[iRay] = cos(dRadian);
        aInvCos[iRay] = 1 / aCos[iRay];
        aSin[iRay] = sin(dRadian);
        aInvSin[iRay] = 1 / aSin[iRay];

        if ((iRay >= ANGLE0) && (iRay < ANGLE180)) {
            aYStep[iRay] = fabs(aSlope[iRay] * CELL_WIDTH);
        }
        else {
            aYStep[iRay] = -1 * fabs(aSlope[iRay] * CELL_WIDTH);
        }

        if ((iRay < ANGLE90) || (iRay >= ANGLE270)) {
            aXStep[iRay] = fabs(aInvSlope[iRay] * CELL_HEIGHT);
        }
        else {
            aXStep[iRay] = -1 * fabs(aInvSlope[iRay] * CELL_HEIGHT);
        }
    }

    for (iRay = -(FOVANGLE / 2); iRay < FOVANGLE / 2; iRay++) {
        dRadian = SHIFTRAD + (iRay * 2 * PI / ANGLE360);
        aRectify[iRay + (FOVANGLE / 2)] = (1 / cos(dRadian)) * iSliverScale;
    }
}

void SetFrameBuffer(const int iScrX, const int iScrY)
{
    const int FRAMEBUFFER_SIZE = iScrX * iScrY * 3;

    if (pFrameBuffer) { free(pFrameBuffer); }
    if (ZBuffer) { free(ZBuffer); }

    pFrameBuffer = (GLubyte*)malloc(FRAMEBUFFER_SIZE);
    ZBuffer = (double*)malloc(iScrX * sizeof(double));
    if (!pFrameBuffer) {
        Error("SetFrameBuffer: Not enough memory to allocate the framebuffer");
    }
    memset(pFrameBuffer, 0, FRAMEBUFFER_SIZE);
}

void Error(const char* szMsg)
{
    printf("Error: %s\n", szMsg);
    _getch();
    exit(1);
}

void DrawTexturedSpriteSliver(const int x, const int yt, const int yb, const int texCol, unsigned char* imgdata, int imgW, int imgH, double intensity)
{
    if (yt < yb) return;

    int sliver_height = yt - yb + 1;
    float tx_step = (float)imgH / sliver_height;
    float tx_y = 0;

    for (int y = yt; y >= yb; y--) {
        int texY = (int)tx_y;
        if (texY >= imgH) texY = imgH - 1;

        unsigned char* c = imgdata + ((texY * imgW) + texCol) * 3;

        unsigned char r = *c;
        unsigned char g = *(c + 1);
        unsigned char b = *(c + 2);

        if (r == 255 && g == 255 && b == 255) {
            tx_y += tx_step;
            continue;
        }

        pFrameBuffer[(((y * SCREENX) + x) * 3)] = (unsigned char)(r * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = (unsigned char)(g * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = (unsigned char)(b * intensity);

        tx_y += tx_step;
    }
}

void DrawSprite(Sprite& sprite)
{
    if (!sprite.active) return;
    if (almondwater_imgdata == NULL || aw_w == 0 || aw_h == 0) return;

    double dx = sprite.x - dXp;
    double dy = sprite.y - dYp;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist < 10.0) return;

    double spriteAngleRad = atan2(dy, dx);
    if (spriteAngleRad < 0) spriteAngleRad += 2 * PI;
    int spriteAngleRay = (int)(spriteAngleRad / (2 * PI) * ANGLE360);

    int angleDiff = spriteAngleRay - iAngle;
    if (angleDiff < -ANGLE360 / 2) angleDiff += ANGLE360;
    if (angleDiff > ANGLE360 / 2) angleDiff -= ANGLE360;

    if (abs(angleDiff) > (FOVANGLE / 2) + 100) return;

    int screenRayID = angleDiff + (FOVANGLE / 2);
    int centerCol = SCRX_1 - screenRayID;

    double scale = aSliverScale[iScrIndex] / dist;
    int spriteSize = (int)scale;

    int top = SCR_CENTERY + (spriteSize / 2);
    int bottom = SCR_CENTERY - (spriteSize / 2);

    int floorOffset = spriteSize / 3;
    top -= floorOffset;
    bottom -= floorOffset;

    if (bottom < 0) bottom = 0;
    if (top > SCRY_1) top = SCRY_1;

    int startX = centerCol - (spriteSize / 4);
    int endX = centerCol + (spriteSize / 4);

    int spriteWidth = endX - startX;
    if (spriteWidth <= 0) spriteWidth = 1;

    for (int x = startX; x <= endX; x++) {
        if (x >= 0 && x < SCREENX) {
            if (dist < ZBuffer[x]) {
                double lightIntensity = CalcLightIntensity(sprite.x, sprite.y);
                int texCol = (int)(((float)(x - startX) / spriteWidth) * aw_w);
                if (texCol >= (int)aw_w) texCol = aw_w - 1;

                DrawTexturedSpriteSliver(x, top, bottom, texCol, almondwater_imgdata, aw_w, aw_h, lightIntensity);
            }
        }
    }
}

void DrawMonster(Monster& m)
{
    if (!m.active) return;

    if (monster_imgdata == NULL || monster_w == 0 || monster_h == 0) return;

    double dx = m.x - dXp;
    double dy = m.y - dYp;
    double dist = sqrt(dx * dx + dy * dy);

    if (dist < 10.0) return;

    double mAngleRad = atan2(dy, dx);
    if (mAngleRad < 0) mAngleRad += 2 * PI;
    int mAngleRay = (int)(mAngleRad / (2 * PI) * ANGLE360);

    int angleDiff = mAngleRay - iAngle;
    if (angleDiff < -ANGLE360 / 2) angleDiff += ANGLE360;
    if (angleDiff > ANGLE360 / 2) angleDiff -= ANGLE360;

    if (abs(angleDiff) > (FOVANGLE / 2) + 100) return;

    int screenRayID = angleDiff + (FOVANGLE / 2);
    int centerCol = SCRX_1 - screenRayID;

    double scale = aSliverScale[iScrIndex] / dist;
    int mSize = (int)scale;

    int top = SCR_CENTERY + (mSize / 2);
    int bottom = SCR_CENTERY - (mSize / 2);

    int floorOffset = mSize / 3;
    top -= floorOffset;
    bottom -= floorOffset;

    if (bottom < 0) bottom = 0;
    if (top > SCRY_1) top = SCRY_1;

    int startX = centerCol - (mSize / 3);
    int endX = centerCol + (mSize / 3);

    int spriteWidth = endX - startX;
    if (spriteWidth <= 0) spriteWidth = 1;

    for (int x = startX; x <= endX; x++) {
        if (x >= 0 && x < SCREENX) {
            if (dist < ZBuffer[x]) {
                double light = CalcLightIntensity(m.x, m.y);
                int texCol = (int)(((float)(x - startX) / spriteWidth) * monster_w);
                if (texCol >= (int)monster_w) texCol = monster_w - 1;
                DrawTexturedSpriteSliver(x, top, bottom, texCol, monster_imgdata, monster_w, monster_h, light);
            }
        }
    }
}

void RayCaster(const int iXp, const int iYp, const int iAngle)
{
    int iRay;
    int iRayID;
    int iXBound;
    int iNextX;
    int iShiftX;
    int iYBound;
    int iNextY;
    int iShiftY;
    double dY;
    double dYDist = LONG_DISTANCE;
    double dX;
    double dXDist = LONG_DISTANCE;
    bool bCast;
    int iYCell;
    int iXCell;
    int iYCellY;
    int iXCellY;
    int iYCellX;
    int iXCellX;
    double dScale;
    int iTop;
    int iBottom;
    int iCol;
    int iCellTypeX = CT_EMPTY;
    int iCellTypeY = CT_EMPTY;

    iRay = iAngle - (FOVANGLE / 2);
    if (iRay < ANGLE0)
        iRay = ANGLE360 + iRay;

    for (iRayID = 0; iRayID < SCREENX; iRayID++) {
        if ((iRay >= ANGLE270) || (iRay < ANGLE90)) {
            iXBound = ((iXp / CELL_WIDTH) * CELL_WIDTH) + CELL_WIDTH;
            iNextX = CELL_WIDTH;
            iShiftX = 0;
        }
        else {
            iXBound = (iXp / CELL_WIDTH) * CELL_WIDTH;
            iNextX = -CELL_WIDTH;
            iShiftX = -1;
        }

        if ((iRay >= ANGLE0) && (iRay < ANGLE180)) {
            iYBound = ((iYp / CELL_HEIGHT) * CELL_HEIGHT) + CELL_HEIGHT;
            iNextY = CELL_HEIGHT;
            iShiftY = 0;
        }
        else {
            iYBound = (iYp / CELL_HEIGHT) * CELL_HEIGHT;
            iNextY = -CELL_HEIGHT;
            iShiftY = -1;
        }

        if ((iRay == ANGLE90) || (iRay == ANGLE270)) {
            bCast = false;
        }
        else {
            dY = (aSlope[iRay] * (iXBound - iXp)) + iYp;
            bCast = true;
        }

        while (bCast) {
            iYCell = (int)(dY / CELL_HEIGHT);
            iXCell = (iXBound + iShiftX) / CELL_WIDTH;

            if ((iYCell < 0) || (iYCell >= CELLY)) {
                dYDist = LONG_DISTANCE;
                bCast = false;
            }
            else if ((iCellTypeY = aMap[CALCY - iYCell][iXCell]) != CT_EMPTY) {
                iYCellY = iYCell;
                iXCellY = iXCell;
                dYDist = (dY - iYp) * aInvSin[iRay];
                bCast = false;
            }
            else {
                dY = aYStep[iRay] + dY;
                iXBound = iXBound + iNextX;
            }
        }

        if ((iRay == ANGLE0) || (iRay == ANGLE180)) {
            bCast = false;
        }
        else {
            dX = (aInvSlope[iRay] * (iYBound - iYp)) + iXp;
            bCast = true;
        }

        while (bCast) {
            iXCell = (int)(dX / CELL_WIDTH);
            iYCell = (iYBound + iShiftY) / CELL_HEIGHT;

            if ((iXCell < 0) || (iXCell >= CELLX)) {
                dXDist = LONG_DISTANCE;
                bCast = false;
            }
            else if ((iCellTypeX = aMap[CALCY - iYCell][iXCell]) != CT_EMPTY) {
                iXCellX = iXCell;
                iYCellX = iYCell;
                dXDist = (dX - iXp) * aInvCos[iRay];
                bCast = false;
            }
            else {
                dX = aXStep[iRay] + dX;
                iYBound = iYBound + iNextY;
            }
        }

        if (dYDist <= dXDist) {
            dScale = aRectify[iRayID] / ((1e-10) + dYDist);
            iBottom = (int)(SCR_CENTERY - (dScale / 2));
            iTop = (int)(SCR_CENTERY + (dScale / 2));

            if ((iBottom < 0) || (iBottom > SCRY_1)) iBottom = 0;
            if ((iTop > SCRY_1) || (iTop < 0)) iTop = SCRY_1;

            iCol = SCRX_1 - iRayID;
            ZBuffer[iCol] = dYDist;

            double intensity = CalcLightIntensity((double)iXBound, dY);

            if (bDoTexture) {
                DrawTexturedSliver(iCol, iTop, iBottom, iCellTypeY - 1, ((int)dY) % CELL_HEIGHT, intensity);
            }
            else {
                if (((int)dY) % CELL_HEIGHT == 0) {
                    DrawSolidSliver(iCol, iTop, iBottom, WHITE[0], WHITE[1], WHITE[2], intensity);
                }
                else {
                    DrawSolidSliver(iCol, iTop, iBottom, BLUE[0], BLUE[1], BLUE[2], intensity);
                }
            }

            DrawCeilingSliver(iCol, SCRY_1, iTop + 1, iRayID, iRay);
            DrawFloorSliver(iCol, iBottom - 1, 0, iRayID, iRay);
        }
        else {
            dScale = aRectify[iRayID] / ((1e-10) + dXDist);
            iBottom = (int)(SCR_CENTERY - (dScale / 2));
            iTop = (int)(SCR_CENTERY + (dScale / 2));

            if ((iBottom < 0) || (iBottom > SCRY_1)) iBottom = 0;
            if ((iTop > SCRY_1) || (iTop < 0)) iTop = SCRY_1;

            iCol = SCRX_1 - iRayID;
            ZBuffer[iCol] = dXDist;

            double intensity = CalcLightIntensity(dX, (double)iYBound);

            if (bDoTexture) {
                DrawTexturedSliver(iCol, iTop, iBottom, iCellTypeX - 1, ((int)dX) % CELL_WIDTH, intensity);
            }
            else {
                if (((int)dX) % CELL_WIDTH == 0) {
                    DrawSolidSliver(iCol, iTop, iBottom, WHITE[0], WHITE[1], WHITE[2], intensity);
                }
                else {
                    DrawSolidSliver(iCol, iTop, iBottom, GREEN[0], GREEN[1], GREEN[2], intensity);
                }
            }

            DrawCeilingSliver(iCol, SCRY_1, iTop + 1, iRayID, iRay);
            DrawFloorSliver(iCol, iBottom - 1, 0, iRayID, iRay);
        }

        iRay++;
        if (iRay == ANGLE360)
            iRay = ANGLE0;
    }

    for (int i = 0; i < NUM_ALMOND_WATER; i++) {
        DrawSprite(almondWaters[i]);
    }
    DrawMonster(stalker);

    // --- POST-PROCESSING: Hallucination Effect ---
    if (fInsanity < 40.0f) {
        for (int y = 0; y < SCREENY; y++) {
            for (int x = 0; x < SCREENX; x++) {
                int index = ((y * SCREENX) + x) * 3;
                int red = pFrameBuffer[index];
                pFrameBuffer[index] = (red + 70 > 255) ? 255 : red + 70;

                if (y % 4 == 0) {
                    pFrameBuffer[index] /= 2;
                    pFrameBuffer[index + 1] /= 2;
                    pFrameBuffer[index + 2] /= 2;
                }
            }
        }
    }

    // --- POST-PROCESSING: Damage Blink Effect ---
    if (g_damageFlashFrames > 0) {
        for (int y = 0; y < SCREENY; y++) {
            for (int x = 0; x < SCREENX; x++) {
                int index = ((y * SCREENX) + x) * 3;
                int red = pFrameBuffer[index] + 160;
                pFrameBuffer[index] = (red > 255) ? 255 : red;
                pFrameBuffer[index + 1] /= 3;
                pFrameBuffer[index + 2] /= 3;
            }
        }
    }

    int barMaxWidth = SCREENX;
    int barWidth = (int)((fInsanity / fMaxInsanity) * barMaxWidth);
    int barHeight = 20;
    int startX = 0;
    int startY = SCREENY - 40;

    for (int y = startY; y < startY + barHeight; y++) {
        for (int x = startX; x < startX + barWidth; x++) {
            pFrameBuffer[(((y * SCREENX) + x) * 3)] = 0;
            pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = 255;
            pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = 0;
        }
    }

#ifdef SHOW_DEBUG_INFO
    for (int x = 0; x < SCREENX; x++) {
        pFrameBuffer[((((SCRY_1 - 1) * SCREENX) + x) * 3)] = 255;
        pFrameBuffer[((((SCRY_1 - 1) * SCREENX) + x) * 3) + 1] = 0;
        pFrameBuffer[((((SCRY_1 - 1) * SCREENX) + x) * 3) + 2] = 0;

        pFrameBuffer[(((SCR_CENTERY * SCREENX) + x) * 3)] = 255;
        pFrameBuffer[(((SCR_CENTERY * SCREENX) + x) * 3) + 1] = 0;
        pFrameBuffer[(((SCR_CENTERY * SCREENX) + x) * 3) + 2] = 0;

        pFrameBuffer[((SCREENX + x) * 3)] = 255;
        pFrameBuffer[((SCREENX + x) * 3) + 1] = 0;
        pFrameBuffer[((SCREENX + x) * 3) + 2] = 0;
    }
#endif
}

void DrawSolidSliver(const int x, const int yt, const int yb,
    const unsigned char r, const unsigned char g, const unsigned char b, double intensity)
{
    if (yt < yb) return;

    for (int y = yb; y <= yt; y++) {
        pFrameBuffer[(((y * SCREENX) + x) * 3)] = (unsigned char)(r * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = (unsigned char)(g * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = (unsigned char)(b * intensity);
    }
}

void DrawTexturedSliver(const int x, const int yt, const int yb, const int tid, const int tc, double intensity)
{
    if (yt < yb) return;

    int sliver_height = yt - yb + 1;
    float tx_step = (float)WALLTEX_HEIGHT / sliver_height;
    float tx_y = 0;
    int y;
    unsigned char* c;

    for (y = yt; y >= yb; y--) {
        c = walltex_imgdata + ((tid * WALLTEX_WIDTH) + tc + ((int)tx_y * WALLTEX_IMGWIDTH)) * 3;

        pFrameBuffer[(((y * SCREENX) + x) * 3)] = (unsigned char)(*c * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = (unsigned char)(*(c + 1) * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = (unsigned char)(*(c + 2) * intensity);

        tx_y += tx_step;
    }
}

void DisplayFunc()
{
    if (!g_started) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, aScrSize[iScrIndex][0], 0, aScrSize[iScrIndex][1]);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        int screenW = aScrSize[iScrIndex][0];
        int screenH = aScrSize[iScrIndex][1];
        int cx = (screenW / 2) - 150;
        int cy = (screenH / 2);

        RenderStringUI(cx - 50, cy + 50, "Welcome to the Backrooms,Don't lose your sanity!");
        RenderStringUI(cx + 35, cy + 20, "and DON'T GET CAUGHT!");
        RenderStringUI(cx + 50, cy - 20, "Press ENTER to start.");

#ifdef USE_DOUBLE_BUFFERING
        glutSwapBuffers();
#else
        glFlush();
#endif
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glRasterPos2i(-1, -1);
    glDrawPixels(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1], GL_RGB, GL_UNSIGNED_BYTE, pFrameBuffer);

#ifdef USE_DOUBLE_BUFFERING
    glutSwapBuffers();
#else
    glFlush();
#endif
}

void KeyboardFunc(unsigned char key, int x, int y)
{
    if (!g_started) {
        if (key == 13 || key == '\r' || key == '\n') {
            g_started = true;
            RayCaster((int)dXp, (int)dYp, iAngle);
            glutPostRedisplay();
        }
        return;
    }

    unsigned char lowerKey = tolower(key);
    keys[lowerKey] = true;

    bool bChangeRes = false;
    bool bChangeFOV = false;
    double dOldAngle;

    switch (lowerKey) {
    case 't': bDoTexture = !bDoTexture; break;
    case '+': case '=': iScrIndex = (iScrIndex + 1) % SRES_NB; bChangeRes = true; break;
    case '-': case '_': iScrIndex = (iScrIndex - 1 < 0) ? SRES_NB - 1 : iScrIndex - 1; bChangeRes = true; break;
    case ']': case '}': iFovIndex = (iFovIndex + 1) % PFOV_NB; bChangeFOV = true; break;
    case '[': case '{': iFovIndex = (iFovIndex - 1 < 0) ? PFOV_NB - 1 : iFovIndex - 1; bChangeFOV = true; break;
    }

    if (bChangeRes || bChangeFOV) {
        dOldAngle = iAngle * DEG_PER_RAY;
        CalcRCParams(aFov[iFovIndex], aScrSize[iScrIndex][0], aScrSize[iScrIndex][1], aSliverScale[iScrIndex]);
        iAngle = (int)(dOldAngle / DEG_PER_RAY);

        if (bChangeRes) {
            SetFrameBuffer(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);
            glutReshapeWindow(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);
        }
    }
}

void KeyboardUpFunc(unsigned char key, int x, int y)
{
    keys[tolower(key)] = false;
}

void LoadPPM(const char* fname, unsigned int* w, unsigned int* h, unsigned char** data, const int mallocflag)
{
    FILE* fp;
    char P, num;
    int max;
    char s[1000];

    if (!(fp = fopen(fname, "rb"))) {
        Error("LoadPPM: Cannot open image file");
    }

    fscanf(fp, "%c%c\n", &P, &num);
    if ((P != 'P') || (num != '6')) {
        Error("LoadPPM: Unknown file format for image");
    }

    do {
        fgets(s, 999, fp);
    } while (s[0] == '#');

    sscanf(s, "%d%d", w, h);
    fgets(s, 999, fp);
    sscanf(s, "%d", &max);

    if (mallocflag) {
        if (!(*data = (unsigned char*)malloc(*w * *h * 3))) {
            Error("LoadPPM: Cannot allocate memory for image data");
        }
    }

    fread(*data, 3, *w * *h, fp);
    fclose(fp);
}

void ReshapeFunc(int w, int h)
{
#ifdef SHOW_DEBUG_INFO
    printf("DEBUG: Current window dimensions are %d x %d\n", w, h);
#endif
}

void TimerFunc(int value)
{
    if (!g_started) {
        glutPostRedisplay();
        glutTimerFunc(50, TimerFunc, 0);
        return;
    }

    double moveSpeed = 6.0;
    int turnSpeed = (int)(6.0 / DEG_PER_RAY);

    if (keys['a']) {
        iAngle += turnSpeed;
        if (iAngle >= ANGLE360) iAngle -= ANGLE360;
    }
    if (keys['d']) {
        iAngle -= turnSpeed;
        if (iAngle < ANGLE0) iAngle += ANGLE360;
    }

    double dDeltaX = 0;
    double dDeltaY = 0;

    if (keys['w']) {
        dDeltaX = moveSpeed * aCos[iAngle];
        dDeltaY = moveSpeed * aSin[iAngle];
    }
    if (keys['s']) {
        dDeltaX = -moveSpeed * aCos[iAngle];
        dDeltaY = -moveSpeed * aSin[iAngle];
    }

    if (dDeltaX != 0 || dDeltaY != 0) {
        int iOldXp = (int)dXp;
        int iOldYp = (int)dYp;
        int iCell;

        if (dDeltaX > 0) {
            iCell = (iOldXp / CELL_WIDTH) + 1;
            dXp += dDeltaX;
            if (aMap[CALCY - (iOldYp / CELL_HEIGHT)][iCell] != CT_EMPTY) {
                if ((int)dXp / CELL_WIDTH == iCell || (int)dXp % CELL_WIDTH >= CELL_WIDTH - SAFE_DIST_FROM_WALL) {
                    dXp = (iCell * CELL_WIDTH) - 1 - SAFE_DIST_FROM_WALL;
                }
            }
        }
        else if (dDeltaX < 0) {
            iCell = (iOldXp / CELL_WIDTH) - 1;
            dXp += dDeltaX;
            if (aMap[CALCY - (iOldYp / CELL_HEIGHT)][iCell] != CT_EMPTY) {
                if ((int)dXp / CELL_WIDTH == iCell || (int)dXp % CELL_WIDTH < SAFE_DIST_FROM_WALL) {
                    dXp = ((iCell + 1) * CELL_WIDTH) + SAFE_DIST_FROM_WALL;
                }
            }
        }

        if (dDeltaY > 0) {
            iCell = (iOldYp / CELL_HEIGHT) + 1;
            dYp += dDeltaY;
            if (aMap[CALCY - iCell][iOldXp / CELL_WIDTH] != CT_EMPTY) {
                if ((int)dYp / CELL_HEIGHT == iCell || (int)dYp % CELL_HEIGHT >= CELL_HEIGHT - SAFE_DIST_FROM_WALL) {
                    dYp = (iCell * CELL_HEIGHT) - 1 - SAFE_DIST_FROM_WALL;
                }
            }
        }
        else if (dDeltaY < 0) {
            iCell = (iOldYp / CELL_HEIGHT) - 1;
            dYp += dDeltaY;
            if (aMap[CALCY - iCell][iOldXp / CELL_WIDTH] != CT_EMPTY) {
                if ((int)dYp / CELL_HEIGHT == iCell || (int)dYp % CELL_HEIGHT < SAFE_DIST_FROM_WALL) {
                    dYp = ((iCell + 1) * CELL_HEIGHT) + SAFE_DIST_FROM_WALL;
                }
            }
        }
    }

    if (fInsanity > 0) {
        fInsanity -= 0.02083f;
    }

    if (fInsanity <= 0.0f) {
        printf("You lost your sanity! Returning to title screen...\n");
        ResetGame();
        memset(keys, false, sizeof(keys));
        g_started = false;
        glutPostRedisplay();
        glutTimerFunc(50, TimerFunc, 0);
        return;
    }

    for (int i = 0; i < NUM_ALMOND_WATER; i++) {
        if (almondWaters[i].active) {
            double dx = almondWaters[i].x - dXp;
            double dy = almondWaters[i].y - dYp;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < CELL_WIDTH / 2.0) {
                almondWaters[i].active = false;
                fInsanity += 30.0f;
                if (fInsanity > fMaxInsanity) fInsanity = fMaxInsanity;
                printf("Drank Almond Water! Sanity is now: %f\n", fInsanity);
            }
        }
    }

    if (stalker.active) {
        int pGridX = (int)(dXp / CELL_WIDTH);
        int pGridY = CALCY - (int)(dYp / CELL_HEIGHT);

        int mGridX = (int)(stalker.x / CELL_WIDTH);
        int mGridY = CALCY - (int)(stalker.y / CELL_HEIGHT);

        double targetX = dXp;
        double targetY = dYp;

        if (mGridX != pGridX || mGridY != pGridY) {
            bool visited[CELLY][CELLX] = { false };
            struct Node {
                int x, y;
                int firstX, firstY;
            };

            Node queue[CELLY * CELLX];
            int head = 0, tail = 0;

            visited[mGridY][mGridX] = true;

            int dX[] = { 0, 0, -1, 1 };
            int dY[] = { -1, 1, 0, 0 };

            for (int i = 0; i < 4; i++) {
                int nextX = mGridX + dX[i];
                int nextY = mGridY + dY[i];

                if (nextX >= 0 && nextX < CELLX && nextY >= 0 && nextY < CELLY) {
                    if (aMap[nextY][nextX] == CT_EMPTY || aMap[nextY][nextX] == 4 || aMap[nextY][nextX] == 5) {
                        visited[nextY][nextX] = true;
                        queue[tail++] = { nextX, nextY, nextX, nextY };
                    }
                }
            }

            int targetStepX = mGridX;
            int targetStepY = mGridY;
            bool foundPath = false;

            while (head < tail) {
                Node current = queue[head++];

                if (current.x == pGridX && current.y == pGridY) {
                    targetStepX = current.firstX;
                    targetStepY = current.firstY;
                    foundPath = true;
                    break;
                }

                for (int i = 0; i < 4; i++) {
                    int nextX = current.x + dX[i];
                    int nextY = current.y + dY[i];

                    if (nextX >= 0 && nextX < CELLX && nextY >= 0 && nextY < CELLY) {
                        if (!visited[nextY][nextX] && (aMap[nextY][nextX] == CT_EMPTY || aMap[nextY][nextX] == 4 || aMap[nextY][nextX] == 5)) {
                            visited[nextY][nextX] = true;
                            queue[tail++] = { nextX, nextY, current.firstX, current.firstY };
                        }
                    }
                }
            }

            if (foundPath) {
                targetX = (targetStepX * CELL_WIDTH) + 32.0;
                targetY = ((CALCY - targetStepY) * CELL_HEIGHT) + 32.0;
            }
        }

        double dx = targetX - stalker.x;
        double dy = targetY - stalker.y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist > 0.5) {
            double moveSpeed = 3.0;
            stalker.x += (dx / dist) * moveSpeed;
            stalker.y += (dy / dist) * moveSpeed;
        }

        double directDx = dXp - stalker.x;
        double directDy = dYp - stalker.y;
        double directDist = sqrt(directDx * directDx + directDy * directDy);

        if (directDist < 35.0) {
            int randGridX, randGridY;
            do {
                randGridX = rand() % CELLX;
                randGridY = rand() % CELLY;
            } while (aMap[randGridY][randGridX] != CT_EMPTY);

            dXp = (randGridX * CELL_WIDTH) + 32.0;
            dYp = ((CALCY - randGridY) * CELL_HEIGHT) + 32.0;

            int mRandGridX, mRandGridY;
            do {
                mRandGridX = rand() % CELLX;
                mRandGridY = rand() % CELLY;
            } while (aMap[mRandGridY][mRandGridX] != CT_EMPTY || (mRandGridX == randGridX && mRandGridY == randGridY));

            stalker.x = (mRandGridX * CELL_WIDTH) + 32.0;
            stalker.y = ((CALCY - mRandGridY) * CELL_HEIGHT) + 32.0;

            fInsanity -= 25.0f;
            g_damageFlashFrames = 6;
            printf("CAUGHT! Warped to a random sector. Sanity damaged!\n");

            if (fInsanity <= 0.0f) {
                printf("You lost your sanity to the monster! Restarting...\n");
                ResetGame();
                memset(keys, false, sizeof(keys));
                g_started = false;
                glutPostRedisplay();
                glutTimerFunc(50, TimerFunc, 0);
                return;
            }
        }
    }

    if (g_damageFlashFrames > 0) {
        g_damageFlashFrames--;
    }

    RayCaster((int)dXp, (int)dYp, iAngle);
    glutPostRedisplay();

    glutTimerFunc(50, TimerFunc, 0);
}

bool IsLight(int cx, int cy) {
    for (int i = 0; i < g_NumLights; i++) {
        if ((int)(g_Lights[i].x / CELL_WIDTH) == cx && (int)(g_Lights[i].y / CELL_HEIGHT) == cy) {
            return true;
        }
    }
    return false;
}

double CalcLightIntensity(double worldX, double worldY)
{
    double totalIntensity = 0.1;

    for (int i = 0; i < g_NumLights; i++) {
        double dx = worldX - g_Lights[i].x;
        double dy = worldY - g_Lights[i].y;
        double distSq = dx * dx + dy * dy;

        if (distSq < 15000.0) {
            double intensity = 1.0 / (1.0 + distSq / 3000.0);
            totalIntensity += intensity;
        }
    }

    if (totalIntensity > 1.0) totalIntensity = 1.0;
    return totalIntensity;
}

void DrawCeilingSliver(const int x, const int yt, const int yb, const int iRayID, const int iRay)
{
    if (yt < yb) return;

    for (int y = yb; y <= yt; y++) {
        int screenY = y - SCR_CENTERY;
        if (screenY <= 0) continue;

        double D = aRectify[iRayID] / (2.0 * screenY);
        double worldX = dXp + D * aCos[iRay];
        double worldY = dYp + D * aSin[iRay];

        int cellX = (int)(worldX / CELL_WIDTH);
        int cellY = (int)(worldY / CELL_HEIGHT);

        double intensity = CalcLightIntensity(worldX, worldY);

        unsigned char r = YELLOW[0];
        unsigned char g = YELLOW[1];
        unsigned char b = YELLOW[2];

        if (IsLight(cellX, cellY)) {
            double localX = worldX - cellX * CELL_WIDTH;
            double localY = worldY - cellY * CELL_HEIGHT;

            if (localX > 20 && localX < 44 && localY > 20 && localY < 44) {
                r = 255;
                g = 255;
                b = 255;
                intensity = 1.0;
            }
        }

        pFrameBuffer[(((y * SCREENX) + x) * 3)] = (unsigned char)(r * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = (unsigned char)(g * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = (unsigned char)(b * intensity);
    }
}

void DrawFloorSliver(const int x, const int yt, const int yb, const int iRayID, const int iRay)
{
    if (yt < yb) return;

    for (int y = yb; y <= yt; y++) {
        int screenY = SCR_CENTERY - y;
        if (screenY <= 0) continue;

        double D = aRectify[iRayID] / (2.0 * screenY);
        double worldX = dXp + D * aCos[iRay];
        double worldY = dYp + D * aSin[iRay];

        double intensity = CalcLightIntensity(worldX, worldY);

        unsigned char r = DYELLOW[0];
        unsigned char g = DYELLOW[1];
        unsigned char b = DYELLOW[2];

        pFrameBuffer[(((y * SCREENX) + x) * 3)] = (unsigned char)(r * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = (unsigned char)(g * intensity);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = (unsigned char)(b * intensity);
    }
}