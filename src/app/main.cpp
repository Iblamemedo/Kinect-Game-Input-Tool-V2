#include "../include/globals.h"

// =====================================================================
// Entry point
// =====================================================================

int WINAPI wWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE,
    _In_ LPWSTR,
    _In_ int nCmdShow)
{
    // Resolve absolute path to config.json next to the executable
    GetModuleFileNameW(nullptr, g_configPath, MAX_PATH);
    wchar_t* lastSlash = wcsrchr(g_configPath, L'\\');
    if (lastSlash) {
        *lastSlash = L'\0';
        wcscat_s(g_configPath, MAX_PATH, L"\\config.json");
    } else {
        wcscpy_s(g_configPath, MAX_PATH, L"config.json");
    }

    // Performance counter frequency (for Kinect FPS calculation)
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    g_perfFreq = (double)freq.QuadPart;
    QueryPerformanceCounter(&g_bodyFpsTimer);

    // Register window class
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance      = hInstance;
    wc.hIcon         = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hIconSm       = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = L"KinectToolClass";
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0, L"KinectToolClass", L"Kinect Game Input",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 900,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 1;

    // Initialize DX11
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(L"KinectToolClass", hInstance);
        return 1;
    }

    // Initialize textures
    CreateColorTexture();
    CreateDepthTexture();
    CreateInfraredTexture();

    // Initialize Kinect
    InitKinect();

    // Load persistent configuration
    ConfigLoad();

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // -----------------------------------------------------------------
    // Main loop
    // -----------------------------------------------------------------
    bool running = true;
    while (running)
    {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                running = false;
        }
        if (!running) break;

        // Poll Kinect
        ProcessBodyFrame();
        ProcessColorFrame();
        ProcessDepthFrame();
        ProcessInfraredFrame();

        // Upload data to GPU if we have new data
        if (g_colorReady) UpdateColorTexture();
        if (g_depthReady) UpdateDepthTexture();
        if (g_infraredReady) UpdateInfraredTexture();

        // Start ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Render application UI
        RenderUI();

        // Render
        ImGui::Render();
        const float clear_color[4] = { 0.06f, 0.06f, 0.06f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_pMainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_pMainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0);
    }

    // -----------------------------------------------------------------
    // Cleanup
    // -----------------------------------------------------------------
    ReleaseAllInputs(); // Safety: release any held synthetic keys
    ConfigSave(); // Final save to ensure no pending changes are lost

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupColorTexture();
    CleanupDepthTexture();
    CleanupInfraredTexture();
    CleanupKinect();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(L"KinectToolClass", hInstance);

    return 0;
}


// =====================================================================
// Win32 message handler
// =====================================================================

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (g_bindingMapping)
    {
        if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN)
        {
            if (wParam == VK_ESCAPE) {
                g_bindingMapping->keyCode = 0; // Clear it
            } else {
                g_bindingMapping->keyCode = (int)wParam;
            }
            g_bindingMapping = nullptr;
            ConfigSave();
            return 0; // consume
        }
        else if (msg == WM_LBUTTONDOWN) { g_bindingMapping->keyCode = VK_LBUTTON; g_bindingMapping = nullptr; ConfigSave(); return 0; }
        else if (msg == WM_RBUTTONDOWN) { g_bindingMapping->keyCode = VK_RBUTTON; g_bindingMapping = nullptr; ConfigSave(); return 0; }
        else if (msg == WM_MBUTTONDOWN) { g_bindingMapping->keyCode = VK_MBUTTON; g_bindingMapping = nullptr; ConfigSave(); return 0; }
        else if (msg == WM_XBUTTONDOWN) { g_bindingMapping->keyCode = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2; g_bindingMapping = nullptr; ConfigSave(); return 0; }
    }

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
