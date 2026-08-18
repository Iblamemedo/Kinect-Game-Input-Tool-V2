#include "../include/globals.h"

// =====================================================================
// Kinect — initialization
// =====================================================================

bool InitKinect()
{
    HRESULT hr = GetDefaultKinectSensor(&g_pKinectSensor);
    if (FAILED(hr) || !g_pKinectSensor)
        return false;

    hr = g_pKinectSensor->Open();
    if (FAILED(hr))
        return false;

    // Coordinate mapper (for projecting joints to screen space)
    g_pKinectSensor->get_CoordinateMapper(&g_pCoordinateMapper);

    // Body frame reader
    IBodyFrameSource* pBodySrc = nullptr;
    hr = g_pKinectSensor->get_BodyFrameSource(&pBodySrc);
    if (SUCCEEDED(hr))
    {
        pBodySrc->OpenReader(&g_pBodyFrameReader);
        SafeRelease(pBodySrc);
    }

    // Color frame reader
    IColorFrameSource* pColorSrc = nullptr;
    hr = g_pKinectSensor->get_ColorFrameSource(&pColorSrc);
    if (SUCCEEDED(hr))
    {
        pColorSrc->OpenReader(&g_pColorFrameReader);
        SafeRelease(pColorSrc);
    }
    
    // Depth frame reader
    IDepthFrameSource* pDepthSrc = nullptr;
    hr = g_pKinectSensor->get_DepthFrameSource(&pDepthSrc);
    if (SUCCEEDED(hr))
    {
        pDepthSrc->OpenReader(&g_pDepthFrameReader);
        SafeRelease(pDepthSrc);
    }
    
    // Infrared frame reader
    IInfraredFrameSource* pInfraredSrc = nullptr;
    hr = g_pKinectSensor->get_InfraredFrameSource(&pInfraredSrc);
    if (SUCCEEDED(hr))
    {
        pInfraredSrc->OpenReader(&g_pInfraredFrameReader);
        SafeRelease(pInfraredSrc);
    }

    return true;
}

void CleanupKinect()
{
    SafeRelease(g_pBodyFrameReader);
    SafeRelease(g_pColorFrameReader);
    SafeRelease(g_pDepthFrameReader);
    SafeRelease(g_pInfraredFrameReader);
    SafeRelease(g_pCoordinateMapper);

    if (g_pKinectSensor)
    {
        g_pKinectSensor->Close();
        SafeRelease(g_pKinectSensor);
    }
}

// =====================================================================
// Color texture — create / update / cleanup
// =====================================================================

bool CreateColorTexture()
{
    g_pColorBuffer = new RGBQUAD[kColorWidth * kColorHeight];

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width            = kColorWidth;
    desc.Height           = kColorHeight;
    desc.MipLevels        = 1;
    desc.ArraySize        = 1;
    desc.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage            = D3D11_USAGE_DYNAMIC;
    desc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags   = D3D11_CPU_ACCESS_WRITE;

    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pColorTex);
    if (FAILED(hr)) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format                    = desc.Format;
    srvDesc.ViewDimension             = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels       = 1;

    hr = g_pd3dDevice->CreateShaderResourceView(g_pColorTex, &srvDesc, &g_pColorSRV);
    return SUCCEEDED(hr);
}

void UpdateColorTexture()
{
    if (!g_pColorTex || !g_pColorBuffer) return;

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = g_pd3dDeviceContext->Map(g_pColorTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    if (SUCCEEDED(hr))
    {
        // Copy row by row (pitch may differ)
        const UINT srcRowBytes = kColorWidth * sizeof(RGBQUAD);
        for (int y = 0; y < kColorHeight; ++y)
        {
            memcpy(
                (BYTE*)mapped.pData + y * mapped.RowPitch,
                (BYTE*)g_pColorBuffer + y * srcRowBytes,
                srcRowBytes);
        }
        g_pd3dDeviceContext->Unmap(g_pColorTex, 0);
    }
}

void CleanupColorTexture()
{
    SafeRelease(g_pColorSRV);
    SafeRelease(g_pColorTex);
    delete[] g_pColorBuffer;
    g_pColorBuffer = nullptr;
}

bool CreateDepthTexture()
{
    g_pDepthBuffer = new RGBQUAD[512 * 424];
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 512; desc.Height = 424; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pDepthTex);
    if (FAILED(hr)) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; srvDesc.Texture2D.MipLevels = 1;
    return SUCCEEDED(g_pd3dDevice->CreateShaderResourceView(g_pDepthTex, &srvDesc, &g_pDepthSRV));
}

void UpdateDepthTexture()
{
    if (!g_pDepthTex || !g_pDepthBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(g_pd3dDeviceContext->Map(g_pDepthTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        const UINT srcRowBytes = 512 * sizeof(RGBQUAD);
        for (int y = 0; y < 424; ++y) memcpy((BYTE*)mapped.pData + y * mapped.RowPitch, (BYTE*)g_pDepthBuffer + y * srcRowBytes, srcRowBytes);
        g_pd3dDeviceContext->Unmap(g_pDepthTex, 0);
    }
}

void CleanupDepthTexture()
{
    SafeRelease(g_pDepthSRV); SafeRelease(g_pDepthTex);
    delete[] g_pDepthBuffer; g_pDepthBuffer = nullptr;
}

bool CreateInfraredTexture()
{
    g_pInfraredBuffer = new RGBQUAD[512 * 424];
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = 512; desc.Height = 424; desc.MipLevels = 1; desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC; desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, &g_pInfraredTex);
    if (FAILED(hr)) return false;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {}; srvDesc.Format = desc.Format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; srvDesc.Texture2D.MipLevels = 1;
    return SUCCEEDED(g_pd3dDevice->CreateShaderResourceView(g_pInfraredTex, &srvDesc, &g_pInfraredSRV));
}

void UpdateInfraredTexture()
{
    if (!g_pInfraredTex || !g_pInfraredBuffer) return;
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(g_pd3dDeviceContext->Map(g_pInfraredTex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        const UINT srcRowBytes = 512 * sizeof(RGBQUAD);
        for (int y = 0; y < 424; ++y) memcpy((BYTE*)mapped.pData + y * mapped.RowPitch, (BYTE*)g_pInfraredBuffer + y * srcRowBytes, srcRowBytes);
        g_pd3dDeviceContext->Unmap(g_pInfraredTex, 0);
    }
}

void CleanupInfraredTexture()
{
    SafeRelease(g_pInfraredSRV); SafeRelease(g_pInfraredTex);
    delete[] g_pInfraredBuffer; g_pInfraredBuffer = nullptr;
}


// =====================================================================
// Process body frame
// =====================================================================

void ProcessBodyFrame()
{
    if (!g_pBodyFrameReader) return;

    IBodyFrame* pFrame = nullptr;
    HRESULT hr = g_pBodyFrameReader->AcquireLatestFrame(&pFrame);
    if (FAILED(hr)) return;

    IBody* ppBodies[BODY_COUNT] = {};
    hr = pFrame->GetAndRefreshBodyData(BODY_COUNT, ppBodies);

    if (SUCCEEDED(hr))
    {
        TrackedBody newLogicalBodies[BODY_COUNT] = {};
        IBody* validKinectBodies[BODY_COUNT] = {};
        UINT64 validKinectIds[BODY_COUNT] = {0};
        int numValidKinect = 0;

        for (int k = 0; k < BODY_COUNT; ++k)
        {
            BOOLEAN isTracked = false;
            if (SUCCEEDED(ppBodies[k]->get_IsTracked(&isTracked)) && isTracked)
            {
                validKinectBodies[numValidKinect] = ppBodies[k];
                ppBodies[k]->get_TrackingId(&validKinectIds[numValidKinect]);
                numValidKinect++;
            }
        }

        bool kinectBodyUsed[BODY_COUNT] = {false};
        
        // Match existing tracking IDs to stable logical slots
        for (int i = 0; i < BODY_COUNT; ++i)
        {
            if (!g_config.persons[i].enabled) {
                g_logicalPersonTrackingIds[i] = 0;
                continue;
            }
            
            if (g_logicalPersonTrackingIds[i] != 0) {
                bool found = false;
                for (int k = 0; k < numValidKinect; ++k) {
                    if (!kinectBodyUsed[k] && validKinectIds[k] == g_logicalPersonTrackingIds[i]) {
                        kinectBodyUsed[k] = true;
                        found = true;
                        newLogicalBodies[i].tracked = true;
                        newLogicalBodies[i].trackingId = validKinectIds[k];
                        validKinectBodies[k]->GetJoints(JointType_Count, newLogicalBodies[i].joints);
                        validKinectBodies[k]->GetJointOrientations(JointType_Count, newLogicalBodies[i].orientations);
                        validKinectBodies[k]->get_HandLeftState(&newLogicalBodies[i].leftHand);
                        validKinectBodies[k]->get_HandRightState(&newLogicalBodies[i].rightHand);
                        break;
                    }
                }
                if (!found) g_logicalPersonTrackingIds[i] = 0;
            }
        }

        // Assign unmapped Kinect bodies to the first available enabled logical slot
        for (int k = 0; k < numValidKinect; ++k)
        {
            if (kinectBodyUsed[k]) continue;
            for (int i = 0; i < BODY_COUNT; ++i)
            {
                if (g_config.persons[i].enabled && g_logicalPersonTrackingIds[i] == 0)
                {
                    g_logicalPersonTrackingIds[i] = validKinectIds[k];
                    kinectBodyUsed[k] = true;
                    newLogicalBodies[i].tracked = true;
                    newLogicalBodies[i].trackingId = validKinectIds[k];
                    validKinectBodies[k]->GetJoints(JointType_Count, newLogicalBodies[i].joints);
                    validKinectBodies[k]->GetJointOrientations(JointType_Count, newLogicalBodies[i].orientations);
                    validKinectBodies[k]->get_HandLeftState(&newLogicalBodies[i].leftHand);
                    validKinectBodies[k]->get_HandRightState(&newLogicalBodies[i].rightHand);
                    break;
                }
            }
        }

        g_trackedBodyCount = 0;
        for (int i = 0; i < BODY_COUNT; ++i)
        {
            g_bodies[i] = newLogicalBodies[i];
            if (g_bodies[i].tracked) g_trackedBodyCount++;
        }

        // --- Kinect body FPS ---
        g_bodyFrameCount++;
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - g_bodyFpsTimer.QuadPart) / g_perfFreq;
        if (elapsed >= 1.0)
        {
            g_kinectBodyFps  = (float)(g_bodyFrameCount / elapsed);
            g_bodyFrameCount = 0;
            g_bodyFpsTimer   = now;
        }

        // --- Distance diagnostic (from selected / first enabled tracked person) ---
        int person = g_selectedPerson;
        if (person < 0 || person >= BODY_COUNT || !g_bodies[person].tracked || !g_config.persons[person].enabled)
        {
            // Auto: pick first enabled tracked body
            person = -1;
            for (int i = 0; i < BODY_COUNT; ++i)
            {
                if (g_bodies[i].tracked && g_config.persons[i].enabled) { person = i; break; }
            }
        }

        if (person < 0)
        {
            g_distStatus = DIST_NO_PERSON;
        }
        else
        {
            float z = g_bodies[person].joints[JointType_SpineBase].Position.Z;
            if (z < kDistTooClose)       g_distStatus = DIST_TOO_CLOSE;
            else if (z > kDistTooFar)    g_distStatus = DIST_TOO_FAR;
            else                         g_distStatus = DIST_GOOD;
        }
    }

    for (int i = 0; i < BODY_COUNT; ++i)
        SafeRelease(ppBodies[i]);
    SafeRelease(pFrame);

    ProcessInputMappings();
}

// =====================================================================
// Process color frame
// =====================================================================

void ProcessColorFrame()
{
    if (!g_pColorFrameReader) return;

    IColorFrame* pFrame = nullptr;
    HRESULT hr = g_pColorFrameReader->AcquireLatestFrame(&pFrame);
    if (FAILED(hr)) return;

    ColorImageFormat fmt = ColorImageFormat_None;
    pFrame->get_RawColorImageFormat(&fmt);

    if (fmt == ColorImageFormat_Bgra)
    {
        UINT sz = 0;
        BYTE* pRaw = nullptr;
        pFrame->AccessRawUnderlyingBuffer(&sz, &pRaw);
        if (pRaw)
            memcpy(g_pColorBuffer, pRaw, kColorWidth * kColorHeight * sizeof(RGBQUAD));
    }
    else
    {
        UINT sz = kColorWidth * kColorHeight * sizeof(RGBQUAD);
        pFrame->CopyConvertedFrameDataToArray(sz, (BYTE*)g_pColorBuffer, ColorImageFormat_Bgra);
    }

    g_colorReady = true;
    SafeRelease(pFrame);
}

void ProcessDepthFrame()
{
    if (!g_pDepthFrameReader) return;
    IDepthFrame* pFrame = nullptr;
    if (FAILED(g_pDepthFrameReader->AcquireLatestFrame(&pFrame))) return;
    
    UINT nBufferSize = 0;
    UINT16* pBuffer = nullptr;
    if (SUCCEEDED(pFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer)))
    {
        USHORT nMinDepth = 0;
        pFrame->get_DepthMinReliableDistance(&nMinDepth);
        USHORT nMaxDepth = USHRT_MAX;
        
        RGBQUAD* pRGBX = g_pDepthBuffer;
        const UINT16* pBufferEnd = pBuffer + (512 * 424);
        while (pBuffer < pBufferEnd)
        {
            USHORT depth = *pBuffer;
            BYTE intensity = static_cast<BYTE>((depth >= nMinDepth) && (depth <= nMaxDepth) ? (depth % 256) : 0);
            pRGBX->rgbRed   = intensity;
            pRGBX->rgbGreen = intensity;
            pRGBX->rgbBlue  = intensity;
            pRGBX->rgbReserved = 255;
            ++pRGBX;
            ++pBuffer;
        }
        g_depthReady = true;
    }
    SafeRelease(pFrame);
}

void ProcessInfraredFrame()
{
    if (!g_pInfraredFrameReader) return;
    IInfraredFrame* pFrame = nullptr;
    if (FAILED(g_pInfraredFrameReader->AcquireLatestFrame(&pFrame))) return;
    
    UINT nBufferSize = 0;
    UINT16* pBuffer = nullptr;
    if (SUCCEEDED(pFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer)))
    {
        float InfraredSourceValueMaximum = static_cast<float>(USHRT_MAX);
        float InfraredOutputValueMinimum = 0.01f;
        float InfraredOutputValueMaximum = 1.0f;
        float InfraredSceneValueAverage = 0.08f;
        float InfraredSceneStandardDeviations = 3.0f;

        RGBQUAD* pDest = g_pInfraredBuffer;
        const UINT16* pBufferEnd = pBuffer + (512 * 424);
        while (pBuffer < pBufferEnd)
        {
            float intensityRatio = static_cast<float>(*pBuffer) / InfraredSourceValueMaximum;
            intensityRatio /= InfraredSceneValueAverage * InfraredSceneStandardDeviations;
            if (intensityRatio > InfraredOutputValueMaximum) intensityRatio = InfraredOutputValueMaximum;
            if (intensityRatio < InfraredOutputValueMinimum) intensityRatio = InfraredOutputValueMinimum;
            
            BYTE intensity = static_cast<BYTE>(intensityRatio * 255.0f);
            pDest->rgbRed   = intensity;
            pDest->rgbGreen = intensity;
            pDest->rgbBlue  = intensity;
            pDest->rgbReserved = 255;
            ++pDest;
            ++pBuffer;
        }
        g_infraredReady = true;
    }
    SafeRelease(pFrame);
}

