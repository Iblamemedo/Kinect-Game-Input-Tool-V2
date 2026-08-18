#include "globals.h"

ID3D11Device*            g_pd3dDevice          = nullptr;
ID3D11DeviceContext*     g_pd3dDeviceContext    = nullptr;
IDXGISwapChain*          g_pSwapChain           = nullptr;
ID3D11RenderTargetView*  g_pMainRenderTargetView = nullptr;

IKinectSensor*        g_pKinectSensor      = nullptr;
IBodyFrameReader*     g_pBodyFrameReader    = nullptr;
IColorFrameReader*    g_pColorFrameReader   = nullptr;
IDepthFrameReader*    g_pDepthFrameReader   = nullptr;
IInfraredFrameReader* g_pInfraredFrameReader = nullptr;
ICoordinateMapper*    g_pCoordinateMapper   = nullptr;

RGBQUAD*                       g_pColorBuffer  = nullptr;
ID3D11Texture2D*               g_pColorTex     = nullptr;
ID3D11ShaderResourceView*      g_pColorSRV     = nullptr;
bool                           g_colorReady    = false;

RGBQUAD*                       g_pDepthBuffer  = nullptr;
ID3D11Texture2D*               g_pDepthTex     = nullptr;
ID3D11ShaderResourceView*      g_pDepthSRV     = nullptr;
bool                           g_depthReady    = false;

RGBQUAD*                       g_pInfraredBuffer = nullptr;
ID3D11Texture2D*               g_pInfraredTex    = nullptr;
ID3D11ShaderResourceView*      g_pInfraredSRV    = nullptr;
bool                           g_infraredReady   = false;

TrackedBody g_bodies[BODY_COUNT] = {};
int         g_trackedBodyCount   = 0;
UINT64      g_logicalPersonTrackingIds[BODY_COUNT] = {0};

ViewMode       g_viewMode       = MODE_BODY;
int            g_selectedPerson = -1;
TrackingConfig g_config         = {};
wchar_t        g_configPath[MAX_PATH] = L"";
SingleMapping* g_bindingMapping = nullptr;

float    g_kinectBodyFps       = 0.0f;
int      g_bodyFrameCount      = 0;
LARGE_INTEGER g_bodyFpsTimer   = {};
double   g_perfFreq            = 0.0;
DistStatus g_distStatus        = DIST_NO_PERSON;
