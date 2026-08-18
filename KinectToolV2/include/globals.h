#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <d3d11.h>
#include <Kinect.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "resource.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

// Safe release for COM interfaces
template<class T>
inline void SafeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

// =====================================================================
// Constants
// =====================================================================

static const int   kColorWidth    = 1920;
static const int   kColorHeight   = 1080;
static const float kDepthWidth    = 512.0f;
static const float kDepthHeight   = 424.0f;

// Distance thresholds (meters, from SpineBase Z)
static const float kDistTooClose  = 1.0f;
static const float kDistTooFar    = 3.5f;

// Skeleton drawing
static const float kBoneThicknessTracked  = 4.0f;
static const float kBoneThicknessInferred = 1.5f;
static const float kJointRadius           = 5.0f;
static const float kHandCircleRadius      = 20.0f;

// =====================================================================
// Joint names
// =====================================================================

static const char* g_JointNames[JointType_Count] = {
    "Spine Base",       // 0
    "Spine Mid",        // 1
    "Neck",             // 2
    "Head",             // 3
    "Shoulder Left",    // 4
    "Elbow Left",       // 5
    "Wrist Left",       // 6
    "Hand Left",        // 7
    "Shoulder Right",   // 8
    "Elbow Right",      // 9
    "Wrist Right",      // 10
    "Hand Right",       // 11
    "Hip Left",         // 12
    "Knee Left",        // 13
    "Ankle Left",       // 14
    "Foot Left",        // 15
    "Hip Right",        // 16
    "Knee Right",       // 17
    "Ankle Right",      // 18
    "Foot Right",       // 19
    "Spine Shoulder",   // 20
    "Hand Tip Left",    // 21
    "Thumb Left",       // 22
    "Hand Tip Right",   // 23
    "Thumb Right",      // 24
};

// Joint keys for JSON serialization (no spaces)
static const char* g_JointKeys[JointType_Count] = {
    "SpineBase", "SpineMid", "Neck", "Head",
    "ShoulderLeft", "ElbowLeft", "WristLeft", "HandLeft",
    "ShoulderRight", "ElbowRight", "WristRight", "HandRight",
    "HipLeft", "KneeLeft", "AnkleLeft", "FootLeft",
    "HipRight", "KneeRight", "AnkleRight", "FootRight",
    "SpineShoulder", "HandTipLeft", "ThumbLeft", "HandTipRight", "ThumbRight",
};

// =====================================================================
// Bone connections (joint pairs to draw)
// =====================================================================

struct BonePair { JointType j0, j1; };

static const BonePair g_Bones[] = {
    // Torso
    { JointType_Head,          JointType_Neck },
    { JointType_Neck,          JointType_SpineShoulder },
    { JointType_SpineShoulder, JointType_SpineMid },
    { JointType_SpineMid,      JointType_SpineBase },
    { JointType_SpineShoulder, JointType_ShoulderRight },
    { JointType_SpineShoulder, JointType_ShoulderLeft },
    { JointType_SpineBase,     JointType_HipRight },
    { JointType_SpineBase,     JointType_HipLeft },
    // Right arm
    { JointType_ShoulderRight, JointType_ElbowRight },
    { JointType_ElbowRight,    JointType_WristRight },
    { JointType_WristRight,    JointType_HandRight },
    { JointType_HandRight,     JointType_HandTipRight },
    { JointType_WristRight,    JointType_ThumbRight },
    // Left arm
    { JointType_ShoulderLeft,  JointType_ElbowLeft },
    { JointType_ElbowLeft,     JointType_WristLeft },
    { JointType_WristLeft,     JointType_HandLeft },
    { JointType_HandLeft,      JointType_HandTipLeft },
    { JointType_WristLeft,     JointType_ThumbLeft },
    // Right leg
    { JointType_HipRight,      JointType_KneeRight },
    { JointType_KneeRight,     JointType_AnkleRight },
    { JointType_AnkleRight,    JointType_FootRight },
    // Left leg
    { JointType_HipLeft,       JointType_KneeLeft },
    { JointType_KneeLeft,      JointType_AnkleLeft },
    { JointType_AnkleLeft,     JointType_FootLeft },
};
static const int g_BoneCount = _countof(g_Bones);

// Body colors — each tracked body gets a distinct color
static const ImU32 g_BodyColors[BODY_COUNT] = {
    IM_COL32(0, 200, 0, 255),      // green
    IM_COL32(0, 150, 255, 255),    // blue
    IM_COL32(255, 200, 0, 255),    // yellow
    IM_COL32(255, 100, 0, 255),    // orange
    IM_COL32(200, 0, 200, 255),    // purple
    IM_COL32(0, 200, 200, 255),    // cyan
};

// =====================================================================
// Tracked body data
// =====================================================================

struct TrackedBody
{
    bool             tracked;
    Joint            joints[JointType_Count];
    JointOrientation orientations[JointType_Count];
    HandState        leftHand;
    HandState        rightHand;
    UINT64           trackingId;
};

// =====================================================================
// Application mode
// =====================================================================

enum ViewMode { MODE_BODY = 0, MODE_RGB, MODE_DEPTH, MODE_INFRARED };


extern ID3D11Device*            g_pd3dDevice;
extern ID3D11DeviceContext*     g_pd3dDeviceContext;
extern IDXGISwapChain*          g_pSwapChain;
extern ID3D11RenderTargetView*  g_pMainRenderTargetView;

extern IKinectSensor*        g_pKinectSensor;
extern IBodyFrameReader*     g_pBodyFrameReader;
extern IColorFrameReader*    g_pColorFrameReader;
extern IDepthFrameReader*    g_pDepthFrameReader;
extern IInfraredFrameReader* g_pInfraredFrameReader;
extern ICoordinateMapper*    g_pCoordinateMapper;

extern RGBQUAD*                       g_pColorBuffer;
extern ID3D11Texture2D*               g_pColorTex;
extern ID3D11ShaderResourceView*      g_pColorSRV;
extern bool                           g_colorReady;

extern RGBQUAD*                       g_pDepthBuffer;
extern ID3D11Texture2D*               g_pDepthTex;
extern ID3D11ShaderResourceView*      g_pDepthSRV;
extern bool                           g_depthReady;

extern RGBQUAD*                       g_pInfraredBuffer;
extern ID3D11Texture2D*               g_pInfraredTex;
extern ID3D11ShaderResourceView*      g_pInfraredSRV;
extern bool                           g_infraredReady;

extern TrackedBody g_bodies[BODY_COUNT];
extern int         g_trackedBodyCount;
extern UINT64      g_logicalPersonTrackingIds[BODY_COUNT];
// =====================================================================
// Tracking configuration (Auto shared + per-person independent)
// =====================================================================

enum class GestureDir { Forward = 0, Backward, Left, Right, Up, Down, Count };
enum class GestureBehavior { Hold = 0, Single, Count };

static const char* g_DirNames[] = { "Forward", "Backward", "Left", "Right", "Up", "Down" };
static const char* g_BehaviorNames[] = { "Hold", "Single" };

struct SingleMapping {
    int keyCode;
    GestureDir direction;
    float threshold;
    GestureBehavior behavior;
    
    // Runtime state (not saved to JSON)
    bool isActive[BODY_COUNT];
    bool justFired[BODY_COUNT];
    
    SingleMapping() : keyCode(0), direction(GestureDir::Forward), threshold(0.15f), behavior(GestureBehavior::Hold) {
        memset(isActive, 0, sizeof(isActive));
        memset(justFired, 0, sizeof(justFired));
    }
};

struct JointMapping {
    std::vector<SingleMapping> items;
};

struct PersonConfig
{
    bool enabled;
    bool joints[JointType_Count];
    JointMapping mappings[JointType_Count];
};

struct TrackingConfig
{
    bool         globalInputEnabled;          // Global enable
    bool         autoJoints[JointType_Count]; // shared Auto config
    JointMapping autoMappings[JointType_Count];
    PersonConfig persons[BODY_COUNT];         // per-person configs
};


extern ViewMode       g_viewMode;
extern int            g_selectedPerson;
extern TrackingConfig g_config;
extern wchar_t        g_configPath[MAX_PATH];
extern SingleMapping* g_bindingMapping;

extern float    g_kinectBodyFps;
extern int      g_bodyFrameCount;
extern LARGE_INTEGER g_bodyFpsTimer;
extern double   g_perfFreq;

enum DistStatus { DIST_GOOD = 0, DIST_TOO_CLOSE, DIST_TOO_FAR, DIST_NO_PERSON };
extern DistStatus g_distStatus;

// Config
void ConfigLoad();
void ConfigSave();
void ParseJointsFromPos(const char* ptr, bool jointsOut[JointType_Count]);
void ParseMappingsFromPos(const char* ptr, JointMapping mappingsOut[JointType_Count]);

// Kinect
bool InitKinect();
void CleanupKinect();
bool CreateColorTexture();
void UpdateColorTexture();
void CleanupColorTexture();
bool CreateDepthTexture();
void UpdateDepthTexture();
void CleanupDepthTexture();
bool CreateInfraredTexture();
void UpdateInfraredTexture();
void CleanupInfraredTexture();
void ProcessBodyFrame();
void ProcessColorFrame();
void ProcessDepthFrame();
void ProcessInfraredFrame();

// Input
void ProcessInputMappings();
void ReleaseAllInputs();

// UI
void RenderUI();

// DX11
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();

// App
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);  
void CreateRenderTarget();  
void CleanupRenderTarget(); 
