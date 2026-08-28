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
//#define USE_DOUBLE_BUFFERING                      // Enables double buffering

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
// Too small distances may produce very large sliver scales

// Insanity Mechanic
float fInsanity = 100.0f;       // Starts at 100
float fMaxInsanity = 100.0f;    // The maximum size of the bar

// Game map: 0 = empty cell, >= 1 cell with textured walls
const int aMap[CELLY][CELLX] =
{ {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,1,0,1,0,1,0,1,0,1,0,1,0,1,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,1,0,1,0,1,0,1,0,1,0,1,0,1,3},
  {3,0,0,0,0,0,0,0,0,1,0,0,0,0,0,3},
  {3,1,0,1,0,1,0,1,0,1,0,1,0,1,0,3},
  {3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,1,0,0,1,0,1,0,1,0,1,0,1,3},
  {3,0,0,2,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,0,1,0,1,0,1,0,1,0,1,0,1,0,3},
  {3,0,0,1,0,0,0,0,0,0,0,0,0,0,0,3},
  {3,0,1,0,1,0,1,0,1,0,1,0,1,0,1,3},
  {3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3} };

const int aSliverScale[SRES_NB] =               // Scale slivers to appropriate size (12500, 15000)
{ 12500, 12500, 20000, 22500, 25000 };
const int aScrSize[SRES_NB][2] =               // Supported screen dimensions
{ {320,200}, {320,240}, {640,480}, {800, 600}, { 1024,768 } };
const int aFov[PFOV_NB] =               // Supported FOV (degrees)
{ 45,60,90 };

const int TURN_ANGLE = 5;           // Angle for the left and right turns (degrees)
const int FORWARD_TRAN = 25;           // Translation for the forward and backward movements (world units)

// Colors defined using RGB tuples
//                                    R    G    B  ; intensity values [0, 255]
const unsigned char WHITE[3] = { 255, 255, 255 };
const unsigned char GREEN[3] = { 0, 255,   0 };
const unsigned char BLUE[3] = { 0,   0, 255 };
const unsigned char GRAY[3] = { 128, 128, 128 };
const unsigned char DGRAY[3] = { 64,  64,  64 };      // Dark gray

// Wall textures
const char WALLTEXTURES_IMGFILE[] = "wall_textures.ppm"; // Wall texture image file
const int WALLTEXTURES_NB = 4;                   // Number of distinct wall textures in the texture image                                         
const int WALLTEX_WIDTH = CELL_WIDTH;          // Square wall textures with identical dimensions to cell's
const int WALLTEX_HEIGHT = CELL_WIDTH;
const int WALLTEX_IMGWIDTH = WALLTEXTURES_NB * WALLTEX_WIDTH;

//|___________________
//|
//| Global Variables
//|___________________

//|-------------------------------------------------------------------|
//| The following global variables are recalculted everytime the user |
//| changes the FOV or screen resolutions                             |

// Viewing parameters    
int FOV;                  // Field of view (currently selected)
int SCREENX;              // Screen width (currently selected)
int SCREENY;              // Screen height
int SCRX_1;               // Screen dimension - 1
int SCRY_1;
int SCR_CENTERY;          // Y coordinate of the screen center 
double DEG_PER_RAY;       // Angle between two adjacent rays (degrees)

// Converts degree angles to (1) ray IDs (0-based)
// They can also be thought of as (2) # of rays in the angles
const int ANGLE0 = 0;
int ANGLE90;
int ANGLE180;
int ANGLE270;
int ANGLE360;
int FOVANGLE;
int ANGLETURN;

// Math Tables -- The allocated array size is the number of rays inside the 360 deg 
double* aSlope = NULL;       // Slopes
double* aInvSlope = NULL;       // Slope inverses
double* aCos = NULL;       // Cosines
double* aInvCos = NULL;       // Cosine inverses
double* aSin = NULL;       // Sines
double* aInvSin = NULL;       // Sine inverses
double* aXStep = NULL;       // X steps when advancing Y by one cell
double* aYStep = NULL;       // Y steps when advancing X by one cell
double* aRectify = NULL;       // To rectify fishbowl distorsion

// Frame buffer (colors are stored in R,G,B order)
GLubyte* pFrameBuffer = NULL;

//|               End of recalculated variables                       |
//|___________________________________________________________________|

//|--------------------------|
//| General global variables |

// Player pose     
double dXp;                        // X position -- (0,0) at bottom left of map
double dYp;                        // Y position
int iAngle;                        // Look at angle (ray ID)

// Screen resolution and FOV selectors
int iScrIndex = SRES_1024x768;
int iFovIndex = PFOV_60;

// Texture mapping
bool bDoTexture = false;   // Is texture mapping active?
unsigned char* walltex_imgdata = NULL;    // Pointer to the loaded wall texture image (raw data format)

//|___________________
//|
//| Function Prototypes
//|___________________

void InitProgram();
void CalcRCParams(const int iFov, const int iScrX, const int iScrY, const int iSliverScale);
void SetFrameBuffer(const int iScrX, const int iScrY);
void Error(const char* szMsg);
void RayCaster(const int iXp, const int iYp, const int iAngle);
void DrawSolidSliver(const int x, const int yt, const int yb,
    const unsigned char r, const unsigned char g, const unsigned char b);
void DrawTexturedSliver(const int x, const int yt, const int yb, const int tid, const int tc);
void DisplayFunc();
void KeyboardFunc(unsigned char key, int x, int y);
void LoadPPM(const char* fname, unsigned int* w, unsigned int* h, unsigned char** data, const int mallocflag);
void ReshapeFunc(int w, int h);
void TimerFunc(int value);
//|____________________________________________________________________
//|
//| Function: main
//|
//| Parameters:
//|   argc     [in] Number of arguments passed to the program.
//|   argv     [in] Array of arguments.
//| Return: 
//|   Program's exit code.
//|
//| Program's entry point.
//|____________________________________________________________________

int main(int argc, char** argv)
{
    // GLUT initialization
    glutInit(&argc, argv);
#ifdef USE_DOUBLE_BUFFERING
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
#else
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
#endif  
    glutInitWindowSize(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);
    glutCreateWindow("Guten Tag! Ray Caster");

    // Program initialization
    InitProgram();

    // "Ray cast" the scene into frame buffer
    RayCaster((int)dXp, (int)dYp, iAngle);

    // Sets callback functions
    glutDisplayFunc(DisplayFunc);
    glutKeyboardFunc(KeyboardFunc);
    glutReshapeFunc(ReshapeFunc);


    // Start the 1-second interval loop
    glutTimerFunc(1000, TimerFunc, 0);

    // Starts GLUT event processing loop:
    glutMainLoop();

    return 0;
}

//|____________________________________________________________________
//|
//| Function: InitProgram
//|
//| Parameters: None.
//| Return: None.
//|
//| Initializes the program; should be called first.
//|____________________________________________________________________

void InitProgram()
{
    unsigned int w, h;       // Texture image dimensions

    // Calculates relevant global variables for the first time
    CalcRCParams(aFov[iFovIndex], aScrSize[iScrIndex][0], aScrSize[iScrIndex][1], aSliverScale[iScrIndex]);

    // Initializes player pose
    // Check for bug at (64,305) 
    dXp = (2 * CELL_WIDTH) + 32;
    dYp = (3 * CELL_HEIGHT) + 32;
    iAngle = ANGLE90;

    // Allocates framebuffer
    SetFrameBuffer(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);

    // Load textures
    LoadPPM(WALLTEXTURES_IMGFILE, &w, &h, &walltex_imgdata, 1);
    if ((w != WALLTEX_IMGWIDTH) || (h != WALLTEX_HEIGHT)) { // Dimensions check
        Error("InitProgram: Invalid texture image dimensions");
    }
}

//|____________________________________________________________________
//|
//| Function: CalcRCParams
//|
//| Parameters:
//|   iFov            [in] The selected FOV.
//|   iScrX           [in] The selected screen width.
//|   iScrY           [in] The selected screen height.
//|   iSliverScale    [in] The selected sliver scale.
//| Return: 
//|   None.
//|
//| Calculate ray caster parameters based on the selected FOV and screen resolutions.
//| Must be called everytime when either of these two variables is changed.
//|____________________________________________________________________

void CalcRCParams(const int iFov, const int iScrX, const int iScrY, const int iSliverScale)
{
    int iRay;
    double dRadian;

    // Setup global viewing parameters    
    FOV = iFov;     // FOV in degrees
    SCREENX = iScrX;    // Screen resolutions in pixels
    SCREENY = iScrY;
    SCRX_1 = SCREENX - 1;
    SCRY_1 = SCREENY - 1;
    SCR_CENTERY = SCREENY / 2;
    DEG_PER_RAY = ((double)FOV) / SCREENX; // Angle between two adjacent rays (degrees)

    // Converts degree angles to (1) ray IDs (0-based)
    // They can also be thought of as (2) # of rays in the angles
    //ANGLE0 = 0;
    ANGLE90 = (int)(90 / DEG_PER_RAY);
    ANGLE180 = (int)(180 / DEG_PER_RAY);
    ANGLE270 = (int)(270 / DEG_PER_RAY);
    ANGLE360 = (int)(360 / DEG_PER_RAY);
    FOVANGLE = (int)(FOV / DEG_PER_RAY);
    ANGLETURN = (int)(TURN_ANGLE / DEG_PER_RAY);

    // Frees the math tables
    if (aSlope) { free(aSlope); }
    if (aInvSlope) { free(aInvSlope); }
    if (aCos) { free(aCos); }
    if (aInvCos) { free(aInvCos); }
    if (aSin) { free(aSin); }
    if (aInvSin) { free(aInvSin); }
    if (aXStep) { free(aXStep); }
    if (aYStep) { free(aYStep); }
    if (aRectify) { free(aRectify); }

    // Allocates the math tables
    if (!(aSlope = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aSlope");
    }
    if (!(aInvSlope = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aInvSlope");
    }
    if (!(aCos = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aCos");
    }
    if (!(aInvCos = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aInvCos");
    }
    if (!(aSin = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aSin");
    }
    if (!(aInvSin = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aInvSin");
    }
    if (!(aXStep = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aXStep");
    }
    if (!(aYStep = (double*)malloc(ANGLE360 * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aYStep");
    }
    if (!(aRectify = (double*)malloc(FOVANGLE * sizeof(double)))) {
        Error("CalcRCParams: Not enough memory to allocate aRectify");
    }

    // Computes the math tables
    for (iRay = 0; iRay < ANGLE360; iRay++) {
        dRadian = SHIFTRAD + (iRay * 2 * PI / ANGLE360);    // Converts from ray ID to radian

        aSlope[iRay] = tan(dRadian);
        aInvSlope[iRay] = 1 / aSlope[iRay];
        aCos[iRay] = cos(dRadian);
        aInvCos[iRay] = 1 / aCos[iRay];
        aSin[iRay] = sin(dRadian);
        aInvSin[iRay] = 1 / aSin[iRay];

        // Calculates YStep for Y Intersections = m*CELL_WIDTH
        if ((iRay >= ANGLE0) && (iRay < ANGLE180)) {
            // [0,90)         : Positive slope and move right ( / --> ), so increase Y
            // [90,180)       : Negative slope and move left  ( \ <-- ), so increase Y
            // m              = rate of change in Y per change in X
            // m * CELL_WIDTH = Delta Y resulting from X movement by one cell
            aYStep[iRay] = fabs(aSlope[iRay] * CELL_WIDTH);
        }
        else {
            // [180,270)      : Positive slope and move left  ( / <-- ), so decrease Y
            // [270,360)      : Negative slope and move right ( \ --> ), so decrease Y
            aYStep[iRay] = -1 * fabs(aSlope[iRay] * CELL_WIDTH);
        }

        // Calculates XStep for X Intersections = m^-1*CELL_HEIGHT
        if ((iRay < ANGLE90) || (iRay >= ANGLE270)) {
            // [0,90)             : Positive slope and move up    ( / ^ ), so increase X
            // [270,360)          : Negative slope and move down  ( \ v ), so increase X
            // m^-1               = rate of change in X per change in Y
            // m^-1 * CELL_HEIGHT = Delta X resulting from Y movement by one cell
            aXStep[iRay] = fabs(aInvSlope[iRay] * CELL_HEIGHT);
        }
        else {
            // [90,180)           : Negative slope and move up    ( \ ^ ), so decrease X
            // [180,270)          : Positive slope and move down  ( / v ), so decrease X
            aXStep[iRay] = -1 * fabs(aInvSlope[iRay] * CELL_HEIGHT);
        }
    }

    for (iRay = -(FOVANGLE / 2); iRay < FOVANGLE / 2; iRay++) {
        dRadian = SHIFTRAD + (iRay * 2 * PI / ANGLE360);
        aRectify[iRay + (FOVANGLE / 2)] = (1 / cos(dRadian)) * iSliverScale;
    }
}

//|____________________________________________________________________
//|
//| Function: SetFrameBuffer
//|
//| Parameters:
//|   iScrX     [in] The selected screen width.
//|   iScrY     [in] The selected screen height.
//| Return: 
//|   None.
//|
//| Frees the existing framebuffer memory and allocates memory for the new framebuffer 
//| based on the selected screen dimensions.
//|____________________________________________________________________

void SetFrameBuffer(const int iScrX, const int iScrY)
{
    const int FRAMEBUFFER_SIZE = iScrX * iScrY * 3;

    // 1. Deallocates the current framebuffer (if any)
    if (pFrameBuffer) {
        free(pFrameBuffer);
    }

    // 2. Allocates memory for the new framebuffer
    pFrameBuffer = (GLubyte*)malloc(FRAMEBUFFER_SIZE);
    if (!pFrameBuffer) {
        // Not enough memory! abort
        Error("SetFrameBuffer: Not enough memory to allocate the framebuffer");
    }
    memset(pFrameBuffer, 0, FRAMEBUFFER_SIZE); // Clears frame buffer to BLACK
}

//|____________________________________________________________________
//|
//| Function: Error
//|
//| Parameters:
//|   szMsg     [in] Error message.
//| Return:
//|   None.
//|
//| Error occurs in the program. Prints the error, waits for keypress
//| and terminates the program.
//|____________________________________________________________________

void Error(const char* szMsg)
{
    printf("Error: %s\n", szMsg);
    _getch();
    exit(1);
}

//|____________________________________________________________________
//|
//| Function: RayCaster
//|
//| Parameters:
//|   iXp         [in] Player's X position.
//|   iYp         [in] Player's Y position.
//|   iAngle      [in] Player's orientation.
//| Return: 
//|   None.
//|
//| Performs the main raycasting loop to render a frame into the framebuffer (pFrameBuffer).
//|____________________________________________________________________

void RayCaster(const int iXp, const int iYp, const int iAngle)
{
    int iRay;           // Ray ID [0, ANGLE360)
    int iRayID;         // Image or screen column ID [0, SCREENX)
    int iXBound;        // X-Wall (vertical wall) to test for intersection
    int iNextX;         // Offset used to find the next X-Wall to test for intersection
    int iShiftX;        // Offset used to find the cell behind iXBound
    int iYBound;        // Y-Wall (horizontal wall) to test for intersection
    int iNextY;         // Offset used to find the next Y-Wall to test for intersection
    int iShiftY;        // Offset used to find the cell behind iYBound
    double dY;          // Y coordinate of the intersection 
    double dYDist = LONG_DISTANCE;  // Distance from the player to the intersection on X-Wall
    double dX;          // X coordinate of the intersection
    double dXDist = LONG_DISTANCE;  // Distance from the player to the intersection on Y-Wall
    bool bCast;         // Is the casting for the current ray done?
    int iYCell;         // The cell behind the intersected wall (used to determine the texture)
    int iXCell;
    int iYCellY;        // Saves the intersected cell for the Y intersection
    int iXCellY;
    int iYCellX;        // Saves the intersected cell for the X intersection
    int iXCellX;
    double dScale;      // Scale factor for a sliver
    int iTop;
    int iBottom;
    int iCol;
    int iCellTypeX = CT_EMPTY; // Type of cell behind X intersection
    int iCellTypeY = CT_EMPTY; // Type of cell behind Y intersection

    // Computes the starting angle to begin ray casting
    iRay = iAngle - (FOVANGLE / 2);
    if (iRay < ANGLE0)
        iRay = ANGLE360 + iRay;

    //|____________________________________________________________________
    //|
    //| Casts SCREENX (screen width) # of rays to generate an image into the framebuffer (pFrameBuffer)
    //| One ray generates one screen column (sliver)
    //|____________________________________________________________________

    for (iRayID = 0; iRayID < SCREENX; iRayID++) {
        //|_________________________________________________
        //| 
        //| 1. Finds the first Y intersection with an X-Wall
        //|_________________________________________________

        if ((iRay >= ANGLE270) || (iRay < ANGLE90)) {
            // 1.1 Casting ray right so test for intersection with the right boundary of the player's cell
            iXBound = ((iXp / CELL_WIDTH) * CELL_WIDTH) + CELL_WIDTH;
            // TODO: iXBound = (iXp & CELLW_MASK) + CELL_WIDTH;
            iNextX = CELL_WIDTH;      // To move right by one cell
            iShiftX = 0;               // 1?
        }
        else {
            // 1.2 Casting ray left so test for intersection with the left boundary of the player's cell
            iXBound = (iXp / CELL_WIDTH) * CELL_WIDTH;
            // TODO: iXBound = iXp & CELLW_MASK;
            iNextX = -CELL_WIDTH;    // To move left by one cell
            iShiftX = -1;
        }
        // TODEL: First Y-intersection: Yi = M(Xb-Xp)+Yp
        //dY = (aSlope[iRay] * (iXBound - iXp)) + iYp;

        //|_________________________________________________
        //| 
        //| 2. Finds the first X intersection with a Y-Wall
        //|_________________________________________________

        if ((iRay >= ANGLE0) && (iRay < ANGLE180)) {
            // 2.1 Casting ray up so test for intersection with the top boundary of the player's cell
            iYBound = ((iYp / CELL_HEIGHT) * CELL_HEIGHT) + CELL_HEIGHT;
            // TODO: iYBound = (iYp & CELLH_MASK) + CELL_HEIGHT;
            iNextY = CELL_HEIGHT;    // To move up by one cell
            iShiftY = 0;              // 1?
        }
        else {
            // 2.2 Casting ray down so test for intersection with the bottom boundary of the player's cell
            iYBound = (iYp / CELL_HEIGHT) * CELL_HEIGHT;
            // TODO: iYBound = iYp & CELLH_MASK;
            iNextY = -CELL_HEIGHT;   // To move down by one cell
            iShiftY = -1;
        }
        // TODEL: First X-intersection: Xi = M^-1(Yi-Yp)+Xp, 
        //dX = (aInvSlope[iRay] * (iYBound - iYp)) + iXp;

        //|_________________________________________________
        //| 
        //| 3. Loop to consider each intersection
        //|    - Stops if the cell behind is non-empty (draw the wall)
        //|    - Otherwise, continue casating to find the intersection with non-empty cell behind
        //|    - Does seperate casting for each axis test
        //|_________________________________________________

        //|_________________________________________________
        //| 
        //| 3.1 Ray casting loop for the Y intersection
        //|_________________________________________________

        // 3.1.1 Skip this loop for vertical rays! (only considers Y-Wall intersection)          
        if ((iRay == ANGLE90) || (iRay == ANGLE270)) {
            bCast = false;
            // TODEL: dYDist = LONG_DISTANCE;
        }
        else {
            // First Y-intersection: Yi = M(Xb-Xp)+Yp
            dY = (aSlope[iRay] * (iXBound - iXp)) + iYp;
            bCast = true;
        }

        //|_________________________________________________
        //| 
        //| 3.1.2 X-Wall intersection test Loop
        //| Pre-conditions:  iShiftX must be computed before entering the loop
        //|                  dY, iXBound must be computed before begining each iteration
        //| Post-conditions: If intersect, iCellTypeY, iYCellY, iXCellY, dYDist must be computed
        //|                  Otherwise, dyDist = LONG_DISTANCE
        //|_________________________________________________

        while (bCast) {
            // Finds the cell behind the intersection
            iYCell = (int)(dY / CELL_HEIGHT);
            iXCell = (iXBound + iShiftX) / CELL_WIDTH;
            // TODO: iYCell = (int)dY >> 6;
            // TODO: iXCell = (iXBound + iShiftX) >> 6;

            if ((iYCell < 0) || (iYCell >= CELLY)) {
                // Safety check: Very high (near vertical) slopes can produce Y-intersections outside of the map boundary
                dYDist = LONG_DISTANCE;
                bCast = false;
            }
            else
                if ((iCellTypeY = aMap[CALCY - iYCell][iXCell]) != CT_EMPTY) {
                    // Intersected with non-empty cell, so calculates the distance to intersection, and stops casting
                    iYCellY = iYCell;
                    iXCellY = iXCell;
                    dYDist = (dY - iYp) * aInvSin[iRay];
                    bCast = false;
                }
                else {
                    // Intersected with empty cell, so continue casting until finding non-empty cell                                
                    dY = aYStep[iRay] + dY;
                    iXBound = iXBound + iNextX;
                }
        }

        //|_________________________________________________
        //| 
        //| 3.2 Ray casting loop for the X intersection
        //|_________________________________________________

        // 3.2.1 Skip this loop for horizontal rays! (only considers X-Wall intersection)  
        if ((iRay == ANGLE0) || (iRay == ANGLE180)) {
            bCast = false;
            // TODEL: dXDist = LONG_DISTANCE;
        }
        else {
            // First X-intersection: Xi = M^-1(Yi-Yp)+Xp, 
            dX = (aInvSlope[iRay] * (iYBound - iYp)) + iXp;
            bCast = true;
        }

        //|_________________________________________________
        //| 
        //| 3.2.2 Y-Wall intersection test Loop
        //| Pre-conditions:  iShiftY must be computed before entering the loop
        //|                  dX, iYBound must be computed before begining each iteration
        //| Post-conditions: if intersected, iCellTypeX, iYCellX, iXCellX, dXDist must be computed
        //|                  Otherwise, dXDist = LONG_DISTANCE
        //|_________________________________________________

        while (bCast) {
            // Finds the cell behind the intersection
            iXCell = (int)(dX / CELL_WIDTH);
            iYCell = (iYBound + iShiftY) / CELL_HEIGHT;
            // TODO: iXCell = (int)dX >> 6;
            // TODO: iYCell = (iYBound + iShiftY) >> 6;

            if ((iXCell < 0) || (iXCell >= CELLX)) {
                // Safety check: Very low (near horizontal) slopes can produce X-intersections outside of the map boundary
                dXDist = LONG_DISTANCE;
                bCast = false;
            }
            else
                if ((iCellTypeX = aMap[CALCY - iYCell][iXCell]) != CT_EMPTY) {
                    // Intersected with non-empty cell, so calculates the distance to intersection, and stops casting                           
                    iXCellX = iXCell;
                    iYCellX = iYCell;
                    dXDist = (dX - iXp) * aInvCos[iRay];
                    bCast = false;
                }
                else {
                    // Intersected with empty cell, so continue casting until finding non-empty cell                                                                                                        
                    dX = aXStep[iRay] + dX;
                    iYBound = iYBound + iNextY;
                }
        }

        //|_________________________________________________
        //| 
        //| 4. Draws the sliver into the framebuffer (pFrameBuffer)
        //| Pre-conditions: 
        //|   For Y-intersect: Intersected cell is  (iXCellY,iYCellY)
        //|                    Intersected point is (iXBound, dY) 
        //|   For X intersect: Intersected cell is  (iXCellX,iYCellX)
        //|                    Intersected point is (dX, iYBound) 
        //|_________________________________________________

        if (dYDist <= dXDist) {
            //|_________________________________________________
            //| 
            //| 4.1 Y-intersection is closer
            //|_________________________________________________

            // Find sliver height
            dScale = aRectify[iRayID] / ((1e-10) + dYDist);
            iBottom = (int)(SCR_CENTERY - (dScale / 2));          // Framebuffer is bottom up (bottom Y = 0)
            iTop = (int)(SCR_CENTERY + (dScale / 2));

            // Safety check: Too small distances may produce extremely large scales
            // Clip to screen boundary
            if ((iBottom < 0) || (iBottom > SCRY_1)) {
                // iBottom > SCRY_1 indicates extremely large scales causing the sign to flip (- to +)
                // So set iBottom to a reasonable value
                iBottom = 0;
            }
            if ((iTop > SCRY_1) || (iTop < 0)) {
                // iTop < 0 indicates extremely large scales causing the sign to flip (+ to -)
                // So set iTop to a reasonable value
                iTop = SCRY_1;
            }

            // Ray casting is done from right to left of the screen
            iCol = SCRX_1 - iRayID;

            // Draws the sliver 
            if (bDoTexture) {
                // With texture mapping
                // TODO: ((int)dY & CELLH_MOD)
                DrawTexturedSliver(iCol, iTop, iBottom, iCellTypeY - 1, ((int)dY) % CELL_HEIGHT);
            }
            else {
                // No texture mapping, draws a solid color
                if (((int)dY) % CELL_HEIGHT == 0) {
                    DrawSolidSliver(iCol, iTop, iBottom, WHITE[0], WHITE[1], WHITE[2]);
                }
                else {
                    DrawSolidSliver(iCol, iTop, iBottom, BLUE[0], BLUE[1], BLUE[2]);
                }
            }

            // Paints the ceiling and floor above and below the sliver, respectively
            DrawSolidSliver(iCol, SCRY_1, iTop + 1, DGRAY[0], DGRAY[1], DGRAY[2]);    // Ceiling
            DrawSolidSliver(iCol, iBottom - 1, 0, GRAY[0], GRAY[1], GRAY[2]);    // Floor
        }
        else {
            //|_________________________________________________
            //| 
            //| 4.2 X-intersection is closer
            //|_________________________________________________

            // Find sliver height
            dScale = aRectify[iRayID] / ((1e-10) + dXDist);
            iBottom = (int)(SCR_CENTERY - (dScale / 2));            // Framebuffer is bottom up (bottom Y = 0)
            iTop = (int)(SCR_CENTERY + (dScale / 2));

            // Safety check: Too small distances may produce extremely large scales
            // Clip to screen boundary
            if ((iBottom < 0) || (iBottom > SCRY_1)) {
                // iBottom > SCRY_1 indicates extremely large scales causing the sign to flip (- to +)
                // So set iBottom to a reasonable value
                iBottom = 0;
            }
            if ((iTop > SCRY_1) || (iTop < 0)) {
                // iTop < 0 indicates extremely large scales causing the sign to flip (+ to -)
                // So set iTop to a reasonable value
                iTop = SCRY_1;
            }

            // Ray casting is done from right to left of the screen
            iCol = SCRX_1 - iRayID;

            // Draws the sliver
            if (bDoTexture) {
                // With texture mapping
                // TODO: ((int)dX & CELLW_MOD)
                DrawTexturedSliver(iCol, iTop, iBottom, iCellTypeX - 1, ((int)dX) % CELL_WIDTH);
            }
            else {
                // No texture mapping, draws a solid color
                if (((int)dX) % CELL_WIDTH == 0) { // TODO: if (((int)dX & CELLW_MOD) == 0)
                    DrawSolidSliver(iCol, iTop, iBottom, WHITE[0], WHITE[1], WHITE[2]);
                }
                else {
                    DrawSolidSliver(iCol, iTop, iBottom, GREEN[0], GREEN[1], GREEN[2]);
                }
            }

            // Paints the ceiling and floor above and below the sliver, respectively
            DrawSolidSliver(iCol, SCRY_1, iTop + 1, DGRAY[0], DGRAY[1], DGRAY[2]);    // Ceiling
            DrawSolidSliver(iCol, iBottom - 1, 0, GRAY[0], GRAY[1], GRAY[2]);    // Floor
        }

        //|_________________________________________________
        //| 
        //| 5. Proceeds to the next iteration to cast the next ray
        //|_________________________________________________

        iRay++;
        if (iRay == ANGLE360)
            iRay = ANGLE0;
 }

 // --- POST-PROCESSING: Hallucination Effect ---
 if (fInsanity < 40.0f) {
     // Iterate through the entire pixel matrix
     for (int y = 0; y < SCREENY; y++) {
         for (int x = 0; x < SCREENX; x++) {
             int index = ((y * SCREENX) + x) * 3;

             // Effect 1: Boost the Red Channel for a danger tint
             int red = pFrameBuffer[index];
             pFrameBuffer[index] = (red + 70 > 255) ? 255 : red + 70;

             // Effect 2: Darken every 4th line for a glitchy scanline look
             if (y % 4 == 0) {
                 pFrameBuffer[index] /= 2; // R
                 pFrameBuffer[index + 1] /= 2; // G
                 pFrameBuffer[index + 2] /= 2; // B
             }
         }
     }
 } 


 // --- HUD: Draw the Insanity Bar ---
 int barMaxWidth = SCREENX;
 int barWidth = (int)((fInsanity / fMaxInsanity) * barMaxWidth);
 int barHeight = 20;
 int startX = 0; // Padding from the left edge
 int startY = SCREENY - 40; // Padding from the top (Y is bottom-up)

 for (int y = startY; y < startY + barHeight; y++) {
     for (int x = startX; x < startX + barWidth; x++) {
         // Draw pure green directly into the framebuffer
         pFrameBuffer[(((y * SCREENX) + x) * 3)] = 0;   // R
         pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = 255; // G
         pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = 0;   // B
     }
 }

#ifdef SHOW_DEBUG_INFO
    for (int x = 0; x < SCREENX; x++) {
        // Top line-1
        pFrameBuffer[((((SCRY_1 - 1) * SCREENX) + x) * 3)] = 255;
        pFrameBuffer[((((SCRY_1 - 1) * SCREENX) + x) * 3) + 1] = 0;
        pFrameBuffer[((((SCRY_1 - 1) * SCREENX) + x) * 3) + 2] = 0;

        // Middle line
        pFrameBuffer[(((SCR_CENTERY * SCREENX) + x) * 3)] = 255;
        pFrameBuffer[(((SCR_CENTERY * SCREENX) + x) * 3) + 1] = 0;
        pFrameBuffer[(((SCR_CENTERY * SCREENX) + x) * 3) + 2] = 0;

        // Bottom line+1
        pFrameBuffer[((SCREENX + x) * 3)] = 255;
        pFrameBuffer[((SCREENX + x) * 3) + 1] = 0;
        pFrameBuffer[((SCREENX + x) * 3) + 2] = 0;
    }
#endif
}

//|____________________________________________________________________
//|
//| Function: DrawSolidSliver
//|
//| Parameters:
//|   x     [in] Screen column.
//|   yt    [in] Top row.
//|   yb    [in] Bottom row.
//|   r     [in] Red intensity.
//|   g     [in] Green intensity.
//|   b     [in] Blue intensity.
//| Return: 
//|   None.
//|
//| Draws a sliver at column 'x' from rows 'yt' to 'yb' with color ('r','g','b').
//|____________________________________________________________________

void DrawSolidSliver(const int x, const int yt, const int yb,
    const unsigned char r, const unsigned char g, const unsigned char b)
{
    if (yt < yb) return;      // Safety check

    int y;

    // Draws from bottom to top pixel
    for (y = yb; y <= yt; y++) {
        pFrameBuffer[(((y * SCREENX) + x) * 3)] = r;
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = g;
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = b;
    }
}

//|____________________________________________________________________
//|
//| Function: DrawTexturedSliver
//|
//| Parameters:
//|   x     [in] Screen column.
//|   yt    [in] Top row.
//|   yb    [in] Bottom row.
//|   tid   [in] Wall texture ID (0-based).
//|   tc    [in] Wall texture column [0, CELL_WIDTH).
//| Return: 
//|   None.
//|
//| Draws a texture-mapped sliver at column 'x' from rows 'yt' to 'yb', using
//| a wall texture identified by 'tid' at a column 'tc'.
//|
//| TODO: Support larger textures
//|____________________________________________________________________

void DrawTexturedSliver(const int x, const int yt, const int yb, const int tid, const int tc)
{
    if (yt < yb) return;                                  // Safety check

    int sliver_height = yt - yb + 1;                      // Sliver height in pixels
    float tx_step = (float)WALLTEX_HEIGHT / sliver_height;   // Rate of change in texel y per change in screen y
    float tx_y = 0;                                // Current texel y (starting at top row)
    int y;                                                // Current screen y
    unsigned char* c;                                     // Pointer to the current texel

    // Draws from top to bottom pixel
    for (y = yt; y >= yb; y--) {
        // Calculates image data offset
        // TODO: OPTIMIZE -- It is a bit messy here
        c = walltex_imgdata + ((tid * WALLTEX_WIDTH) + tc + ((int)tx_y * WALLTEX_IMGWIDTH)) * 3;

        // Plots texel
        pFrameBuffer[(((y * SCREENX) + x) * 3)] = *c;
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 1] = *(c + 1);
        pFrameBuffer[(((y * SCREENX) + x) * 3) + 2] = *(c + 2);

        // Updates texel y
        tx_y += tx_step;
    }
}

//|____________________________________________________________________
//|
//| Function: DisplayFunc
//|
//| Parameters: None.
//| Return: None.
//|
//| GLUT display callback function called for each redraw needed.
//|____________________________________________________________________

void DisplayFunc()
{
    // Set the raster position to the lower-left corner to avoid a problem 
    // (with glDrawPixels) when the window is resized to smaller dimensions
    glRasterPos2i(-1, -1);

    // Write the information stored in "pFrameBuffer" to the color buffer
    glDrawPixels(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1], GL_RGB, GL_UNSIGNED_BYTE,
        pFrameBuffer);

#ifdef USE_DOUBLE_BUFFERING
    glutSwapBuffers();
#else
    glFlush();
#endif
}

//|____________________________________________________________________
//|
//| Function: KeyboardFunc
//|
//| Parameters:
//|   key     [in] Key code.
//|   x       [in] X-coordinate of mouse when key is pressed.
//|   y       [in] Y-coordinate of mouse when key is pressed.
//| Return: 
//|   None.
//|
//| GLUT keyboard callback function called for every key press event.
//|____________________________________________________________________

void KeyboardFunc(unsigned char key, int x, int y)
{
    bool   bTranslate = false;   // Is the player translated?
    bool   bChangeRes = false;   // Is resolution changed?
    bool   bChangeFOV = false;   // Is FOV changed?
    double dDeltaX, dDeltaY;     // Delta translation 
    double dOldAngle;            // Player's view direction in degrees (before recalculating ray casting parameters)

    switch (key) {
        //|_________________________________________________
        //| 
        //| WASD for player's movement
        //|_________________________________________________

    case 'a':               // Left turn
    case 'A':
        iAngle += ANGLETURN;
        if (iAngle >= ANGLE360)
            iAngle = iAngle - ANGLE360;
        break;
    case 'd':               // Right turn
    case 'D':
        iAngle -= ANGLETURN;
        if (iAngle < ANGLE0)
            iAngle = ANGLE360 + iAngle;
        break;
    case 'w':               // Forward translation
    case 'W':
        // Computes the delta translation
        dDeltaX = FORWARD_TRAN * aCos[iAngle];
        dDeltaY = FORWARD_TRAN * aSin[iAngle];
        bTranslate = true;
        break;
    case 's':               // Backward translation
    case 'S':
        // Computes the delta translation
        dDeltaX = -FORWARD_TRAN * aCos[iAngle];
        dDeltaY = -FORWARD_TRAN * aSin[iAngle];
        bTranslate = true;
        break;

        //|_________________________________________________
        //| 
        //| Texture mapping
        //|_________________________________________________
    case 't':
    case 'T':
        bDoTexture = !bDoTexture;
        break;

        //|_________________________________________________
        //| 
        //| Resolution and FOV changes
        //|_________________________________________________
    case '+':               // Increases resolution
    case '=':
        iScrIndex++;
        if (iScrIndex == SRES_NB) {
            iScrIndex = 0;
        }
        bChangeRes = true;
        break;
    case '-':               // Decreases resolution
    case '_':
        iScrIndex--;
        if (iScrIndex < 0) {
            iScrIndex = SRES_NB - 1;
        }
        bChangeRes = true;
        break;
    case ']':               // Increases FOV
    case '}':
        iFovIndex++;
        if (iFovIndex == PFOV_NB) {
            iFovIndex = 0;
        }
        bChangeFOV = true;
        break;
    case '[':               // Decreases FOV
    case '{':
        iFovIndex--;
        if (iFovIndex < 0) {
            iFovIndex = PFOV_NB - 1;
        }
        bChangeFOV = true;
        break;

        // Otherwise, does nothing
    default:
        return;
    }

    if (bChangeRes || bChangeFOV) {
        //|_________________________________________________
        //| 
        //| Recalculates ray casting parameters if resolution or FOV is changed
        //|_________________________________________________
        dOldAngle = iAngle * DEG_PER_RAY;               // Maps old ray ID to degrees
        CalcRCParams(aFov[iFovIndex], aScrSize[iScrIndex][0], aScrSize[iScrIndex][1], aSliverScale[iScrIndex]);
        iAngle = (int)(dOldAngle / DEG_PER_RAY);     // Maps degrees to new ray ID

        if (bChangeRes) {
            // Destroys and reallocates framebuffer
            SetFrameBuffer(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);

            // Set window size to match the new resolution
            glutReshapeWindow(aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);

            printf("Screen resolutions are %d x %d\n", aScrSize[iScrIndex][0], aScrSize[iScrIndex][1]);
        }
        else { // implied bChangeFOV
            printf("Player's FOV is %d\n", aFov[iFovIndex]);
        }
    }

    if (bTranslate) {
#ifdef USE_ADVANCED_PLAYER_TRANSLATION
        //|_________________________________________________
        //| 
        //| Advanced movement mechanics: Player can slide along walls
        //|_________________________________________________

        int iOldXp, iOldYp;       // The player's old position
        int iCp;                  // The player's current position (X or Y)
        int iCell;                // The next cell wrt player before translation (X or Y)

        // Player's position before translation
        iOldXp = (int)dXp;
        iOldYp = (int)dYp;

        if (dDeltaX > 0) {
            //|_________________________________________________
            //| 
            //| Move right
            //|_________________________________________________

            iCell = (iOldXp / CELL_WIDTH) + 1;      // Cell on the right of player before translation
            // TODO: (iOldXp >> 6) + 1
            dXp += dDeltaX;                      // Player's position after translation      
            if (aMap[CALCY - (iOldYp / CELL_HEIGHT)][iCell] != CT_EMPTY) {
                // There is a wall (non-emptied cell) on the right
                // Make sure the player is within the safe distance from the wall after the translation

                iCp = (int)dXp;                       // Position after the transation
                if ((iCp / CELL_WIDTH == iCell) ||
                    (iCp % CELL_WIDTH >= CELL_WIDTH - SAFE_DIST_FROM_WALL)) {
                    // TODO: iCp >> 6
                    // TODO: iCp & CELLW_MOD
                    // Player moves into non-emptied cell on the right, or
                    // Player crosses the safe distance from the wall
                    // Moves player to the safe distance
                    dXp = (iCell * CELL_WIDTH) - 1 - SAFE_DIST_FROM_WALL;
                } // else { The player is still within the safe distance after transation, so it is safe to move }
            } // else { There is an empty cell on the right, so it is safe to move }      
        }
        else
            if (dDeltaX < 0) {
                //|_________________________________________________
                //| 
                //| Move left
                //|_________________________________________________

                iCell = (iOldXp / CELL_WIDTH) - 1;      // Cell on the left of player before translation
                // TODO: (iOldXp >> 6) - 1
                dXp += dDeltaX;                      // Player's position after translation      
                if (aMap[CALCY - (iOldYp / CELL_HEIGHT)][iCell] != CT_EMPTY) {
                    // There is a wall (non-emptied cell) on the left
                    // Make sure the player is within the safe distance from the wall after the translation

                    iCp = (int)dXp;                       // Position after the transation
                    if ((iCp / CELL_WIDTH == iCell) ||
                        (iCp % CELL_WIDTH < SAFE_DIST_FROM_WALL)) {
                        // TODO: iCp >> 6
                        // TODO: iCp & CELLW_MOD
                        // Player moves into non-emptied cell on the left, or
                        // Player crosses the safe distance from the wall
                        // Moves player to the safe distance
                        dXp = ((iCell + 1) * CELL_WIDTH) + SAFE_DIST_FROM_WALL;
                    } // else { The player is still within the safe distance after transation, so it is safe to move }
                } // else { There is an empty cell on the left, so it is safe to move }      
            }

        if (dDeltaY > 0) {
            //|_________________________________________________
            //| 
            //| Move up
            //|_________________________________________________

            iCell = (iOldYp / CELL_HEIGHT) + 1;     // Cell above player before translation
            // TODO: (iOldYp >> 6) + 1
            dYp += dDeltaY;                      // Player's position after translation      
            if (aMap[CALCY - iCell][iOldXp / CELL_WIDTH] != CT_EMPTY) {
                // There is a wall (non-emptied cell) above
                // Make sure the player is within the safe distance from the wall after the translation

                iCp = (int)dYp;                       // Position after the transation
                if ((iCp / CELL_HEIGHT == iCell) ||
                    (iCp % CELL_HEIGHT >= CELL_HEIGHT - SAFE_DIST_FROM_WALL)) {
                    // TODO: iCp >> 6
                    // TODO: iCp & CELLW_MOD
                    // Player moves into non-emptied cell above, or
                    // Player crosses the safe distance from the wall
                    // Moves player to the safe distance
                    dYp = (iCell * CELL_HEIGHT) - 1 - SAFE_DIST_FROM_WALL;
                } // else { The player is still within the safe distance after transation, so it is safe to move }
            } // else { There is an empty cell above, so it is safe to move }      
        }
        else
            if (dDeltaY < 0) {
                //|_________________________________________________
                //| 
                //| Move down
                //|_________________________________________________

                iCell = (iOldYp / CELL_HEIGHT) - 1;     // Cell below player before translation
                // TODO: (iOldYp >> 6) - 1
                dYp += dDeltaY;                      // Player's position after translation      
                if (aMap[CALCY - iCell][iOldXp / CELL_WIDTH] != CT_EMPTY) {
                    // There is a wall (non-emptied cell) below
                    // Make sure the player is within the safe distance from the wall after the translation

                    iCp = (int)dYp;                       // Position after the transation
                    if ((iCp / CELL_HEIGHT == iCell) ||
                        (iCp % CELL_HEIGHT < SAFE_DIST_FROM_WALL)) {
                        // TODO: iCp >> 6
                        // TODO: iCp & CELLW_MOD
                        // Player moves into non-emptied cell below, or
                        // Player crosses the safe distance from the wall
                        // Moves player to the safe distance
                        dYp = ((iCell + 1) * CELL_HEIGHT) + SAFE_DIST_FROM_WALL;
                    } // else { The player is still within the safe distance after transation, so it is safe to move }
                } // else { There is an empty cell below, so it is safe to move }      
            }
#else
        //|_________________________________________________
        //| 
        //| Simple movement mechanics: No sliding along walls
        //|_________________________________________________

        double dOldXp, dOldYp;       // The player's old position

        // Player's position before translation
        dOldXp = dXp;
        dOldYp = dYp;

        // Computes the new position
        dXp += dDeltaX;
        dYp += dDeltaY;

        if (aMap[CALCY - (((int)dYp) / CELL_HEIGHT)][((int)(dXp)) / CELL_WIDTH] != CT_EMPTY) {
            // TODO: if (aMap[CALCY - (iYp >> 6)][iXp >> 6] == 0)
            // Player lands into non-emptied cell, return to the previous position
            dXp = dOldXp;
            dYp = dOldYp;
        } // else { The move is valid }
#endif
    }

    RayCaster((int)dXp, (int)dYp, iAngle);    // Perform ray casting after the movement to update the framebuffer
    glutPostRedisplay();                      // Asks GLUT to redraw the screen
}

//|____________________________________________________________________
//|
//| Function: LoadPPM
//|
//| Parameter:
//|  fname      [in]     Name of file to load.
//|  w          [out]    Width of loaded image in pixels.
//|  h          [out]    Height of loaded image in pixels.
//|  data       [in/out] Image data address (in or out depending on mallocflag)
//|  mallocflag [in]     1 if memory not pre-allocated, 0 if data already points
//|                      to allocated memory that can hold the image. Note that
//|                      if new memory is allocated, free() should be used to
//|                      deallocate when it is no longer needed.
//| Return:
//|   None.
//| 
//| A minimal Portable PixMap (PPM) image file loader.
//|____________________________________________________________________

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

    do
    {
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

//|____________________________________________________________________
//|
//| Function: ReshapeFunc
//|
//| Parameters:
//|   w     [in] Width of the current window.
//|   h     [in] Height of the current window.
//| Return:
//|   None.
//|
//| GLUT reshape callback function called everytime the window is resized.
//|____________________________________________________________________

void ReshapeFunc(int w, int h)
{
#ifdef SHOW_DEBUG_INFO
    // Tracks the current window dimensions
    printf("DEBUG: Current window dimensions are %d x %d\n", w, h);
#endif
}

void TimerFunc(int value)
{
    // Drain the sanity bar
    if (fInsanity > 0) {
        fInsanity -= 1.0f; // Amount to decrease every second
    }

    // Redraw the screen to reflect the updated bar, even if standing still
    RayCaster((int)dXp, (int)dYp, iAngle);
    glutPostRedisplay();

    // Call this timer again in 1000 milliseconds (1 second)
    glutTimerFunc(1000, TimerFunc, 0);
}