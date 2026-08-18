#include "../include/globals.h"

// =====================================================================
// Project a camera-space body point to 2D display coordinates
// =====================================================================

static ImVec2 BodyToScreen(const CameraSpacePoint& pt,
                           float dispW, float dispH,
                           ImVec2 origin)
{
    if (!g_pCoordinateMapper) return origin;

    DepthSpacePoint dp = {};
    g_pCoordinateMapper->MapCameraPointToDepthSpace(pt, &dp);

    float sx = origin.x + dp.X * dispW / kDepthWidth;
    float sy = origin.y + dp.Y * dispH / kDepthHeight;
    return ImVec2(sx, sy);
}

// =====================================================================
// Draw one tracked body's skeleton
// =====================================================================

void DrawSkeleton(ImDrawList* dl,
                         const TrackedBody& body,
                         float dispW, float dispH,
                         ImVec2 origin,
                         ImU32 color)
{
    // Project all joints
    ImVec2 pts[JointType_Count];
    for (int j = 0; j < JointType_Count; ++j)
        pts[j] = BodyToScreen(body.joints[j].Position, dispW, dispH, origin);

    // Draw bones
    ImU32 inferredColor = IM_COL32(128, 128, 128, 180);
    for (int b = 0; b < g_BoneCount; ++b)
    {
        JointType j0 = g_Bones[b].j0;
        JointType j1 = g_Bones[b].j1;
        TrackingState s0 = body.joints[j0].TrackingState;
        TrackingState s1 = body.joints[j1].TrackingState;

        if (s0 == TrackingState_NotTracked || s1 == TrackingState_NotTracked)
            continue;
        if (s0 == TrackingState_Inferred && s1 == TrackingState_Inferred)
            continue;

        bool bothTracked = (s0 == TrackingState_Tracked && s1 == TrackingState_Tracked);
        dl->AddLine(pts[j0], pts[j1],
                    bothTracked ? color : inferredColor,
                    bothTracked ? kBoneThicknessTracked : kBoneThicknessInferred);
    }

    // Draw joints
    ImU32 inferredJoint = IM_COL32(255, 255, 0, 200);
    for (int j = 0; j < JointType_Count; ++j)
    {
        if (body.joints[j].TrackingState == TrackingState_NotTracked)
            continue;

        bool tracked = (body.joints[j].TrackingState == TrackingState_Tracked);
        dl->AddCircleFilled(pts[j], kJointRadius,
                            tracked ? color : inferredJoint);
    }

    // Draw hand states
    auto drawHand = [&](HandState hs, ImVec2 pos)
    {
        ImU32 hc = 0;
        switch (hs)
        {
        case HandState_Closed: hc = IM_COL32(255, 0, 0, 128);   break;
        case HandState_Open:   hc = IM_COL32(0, 255, 0, 128);   break;
        case HandState_Lasso:  hc = IM_COL32(0, 0, 255, 128);   break;
        default: return;
        }
        dl->AddCircleFilled(pos, kHandCircleRadius, hc);
    };

    drawHand(body.leftHand,  pts[JointType_HandLeft]);
    drawHand(body.rightHand, pts[JointType_HandRight]);
}

// =====================================================================
// Render the full application UI
// =====================================================================

void RenderUI()
{
    ImGuiIO& io = ImGui::GetIO();
    bool configChanged = false;

    // Fullscreen window covering the viewport
    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##Main", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoSavedSettings);

    // --- Top bar ---
    ImGui::Text("Kinect Game Input");
    ImGui::SameLine(ImGui::GetWindowWidth() - 320.0f);
    if (ImGui::Checkbox("Input [ ON / OFF ]", &g_config.globalInputEnabled))
        configChanged = true;
    ImGui::SameLine();
    if (ImGui::Button("BODY", ImVec2(80, 0))) g_viewMode = MODE_BODY;
    ImGui::SameLine();
    
    ImGui::SetNextItemWidth(120);
    const char* previewModes[] = { "RGB", "Depth", "Infrared" };
    int currentPreviewMode = 0;
    if (g_viewMode == MODE_RGB) currentPreviewMode = 0;
    else if (g_viewMode == MODE_DEPTH) currentPreviewMode = 1;
    else if (g_viewMode == MODE_INFRARED) currentPreviewMode = 2;
    else currentPreviewMode = -1;
    
    if (ImGui::BeginCombo("Camera Preview", currentPreviewMode >= 0 ? previewModes[currentPreviewMode] : "Select..."))
    {
        for (int i = 0; i < 3; i++)
        {
            bool is_selected = (currentPreviewMode == i);
            if (ImGui::Selectable(previewModes[i], is_selected))
            {
                if (i == 0) g_viewMode = MODE_RGB;
                else if (i == 1) g_viewMode = MODE_DEPTH;
                else if (i == 2) g_viewMode = MODE_INFRARED;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Separator();

    // --- Layout: display (left) + panel (right) ---
    float panelW = 480.0f; // Made panel a bit wider for mapping controls
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float displayW = ImGui::GetContentRegionAvail().x - panelW - spacing;
    float contentH = ImGui::GetContentRegionAvail().y;

    // === LEFT: Display area ===
    ImGui::BeginChild("##Display", ImVec2(displayW, contentH), true,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    {
        ImVec2 regionSize = ImGui::GetContentRegionAvail();
        ImVec2 cursorPos  = ImGui::GetCursorScreenPos();

        if (g_viewMode == MODE_BODY)
        {
            // Aspect-ratio-correct display area (depth camera: 512x424)
            float aspect = kDepthWidth / kDepthHeight;
            float dw, dh;
            if (regionSize.x / regionSize.y > aspect)
            { dh = regionSize.y; dw = dh * aspect; }
            else
            { dw = regionSize.x; dh = dw / aspect; }

            float ox = cursorPos.x + (regionSize.x - dw) * 0.5f;
            float oy = cursorPos.y + (regionSize.y - dh) * 0.5f;
            ImVec2 origin(ox, oy);

            ImDrawList* dl = ImGui::GetWindowDrawList();

            // Black background
            dl->AddRectFilled(origin, ImVec2(ox + dw, oy + dh), IM_COL32(0, 0, 0, 255));

            // Draw each tracked body
            for (int i = 0; i < BODY_COUNT; ++i)
            {
                if (g_bodies[i].tracked && g_config.persons[i].enabled)
                    DrawSkeleton(dl, g_bodies[i], dw, dh, origin, g_BodyColors[i]);
            }

            // Border
            dl->AddRect(origin, ImVec2(ox + dw, oy + dh), IM_COL32(60, 60, 60, 255));
        }
        else // Camera Preview modes
        {
            ID3D11ShaderResourceView* srv = nullptr;
            float texW = 0, texH = 0;
            bool ready = false;
            
            if (g_viewMode == MODE_RGB) { srv = g_pColorSRV; texW = kColorWidth; texH = kColorHeight; ready = g_colorReady; }
            else if (g_viewMode == MODE_DEPTH) { srv = g_pDepthSRV; texW = 512; texH = 424; ready = g_depthReady; }
            else if (g_viewMode == MODE_INFRARED) { srv = g_pInfraredSRV; texW = 512; texH = 424; ready = g_infraredReady; }
            
            if (ready && srv)
            {
                float aspect = texW / texH;
                float dw, dh;
                if (regionSize.x / regionSize.y > aspect)
                { dh = regionSize.y; dw = dh * aspect; }
                else
                { dw = regionSize.x; dh = dw / aspect; }

                float ox = (regionSize.x - dw) * 0.5f;
                float oy = (regionSize.y - dh) * 0.5f;

                ImGui::SetCursorPos(ImVec2(ox, oy));
                ImGui::Image((ImTextureID)srv, ImVec2(dw, dh));
            }
            else
            {
                ImGui::TextWrapped("Waiting for camera data...");
            }
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // === RIGHT: Control panel ===
    ImGui::BeginChild("##Panel", ImVec2(panelW, contentH), true);
    {
        // ----- Tracking section -----
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Tracking");
        ImGui::Separator();

        // Person selector — Auto + all 6 persons (always shown)
        {
            char previewLabel[64] = "Auto";
            if (g_selectedPerson >= 0)
                sprintf_s(previewLabel, "Person %d", g_selectedPerson + 1);

            if (ImGui::BeginCombo("Mode", previewLabel))
            {
                if (ImGui::Selectable("Auto", g_selectedPerson < 0))
                    g_selectedPerson = -1;

                for (int i = 0; i < BODY_COUNT; ++i)
                {
                    char label[64];
                    if (g_bodies[i].tracked)
                        sprintf_s(label, "Person %d (Tracked)", i + 1);
                    else
                        sprintf_s(label, "Person %d", i + 1);
                    if (ImGui::Selectable(label, g_selectedPerson == i))
                        g_selectedPerson = i;
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Spacing();

        // ----- Person enable/disable toggles -----
        ImGui::Text("People:");
        for (int i = 0; i < BODY_COUNT; ++i)
        {
            ImGui::PushID(i);
            char label[32];
            sprintf_s(label, "Person %d", i + 1);

            bool enabled = g_config.persons[i].enabled;
            if (g_bodies[i].tracked)
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            else
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

            if (ImGui::Checkbox(label, &enabled))
            {
                g_config.persons[i].enabled = enabled;
                configChanged = true;
            }
            ImGui::PopStyleColor();

            // Show tracking status inline
            if (g_bodies[i].tracked)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "[T]");
            }
            ImGui::PopID();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // ----- Tracking points -----
        if (g_selectedPerson < 0)
            ImGui::Text("Tracking points (Shared/Auto):");
        else
            ImGui::Text("Tracking points (Person %d):", g_selectedPerson + 1);
        ImGui::Spacing();

        // Point to the active joint config
        bool* activeJoints = (g_selectedPerson < 0)
            ? g_config.autoJoints
            : g_config.persons[g_selectedPerson].joints;
            
        JointMapping* activeMappings = (g_selectedPerson < 0)
            ? g_config.autoMappings
            : g_config.persons[g_selectedPerson].mappings;

        // Determine which person's live data to show
        int showPerson = -1;
        if (g_selectedPerson >= 0 && g_bodies[g_selectedPerson].tracked)
        {
            showPerson = g_selectedPerson;
        }
        else
        {
            // Auto: pick first enabled + tracked person
            for (int i = 0; i < BODY_COUNT; ++i)
                if (g_config.persons[i].enabled && g_bodies[i].tracked)
                    { showPerson = i; break; }
        }

        // Joint checkboxes and mappings
        int selectedJointCount = 0;
        ImGui::BeginChild("##Joints", ImVec2(-1, 380), false, ImGuiWindowFlags_HorizontalScrollbar); // increased height slightly
        for (int j = 0; j < JointType_Count; ++j)
        {
            ImGui::PushID(j);
            if (ImGui::Checkbox(g_JointNames[j], &activeJoints[j]))
                configChanged = true;
                
            if (activeJoints[j])
            {
                selectedJointCount++;
                ImGui::Indent();

                // Show tracking state and position for live data
                if (showPerson >= 0 && g_bodies[showPerson].tracked)
                {
                    const Joint& jt = g_bodies[showPerson].joints[j];
                    const char* stateStr = "N/A";
                    ImVec4 stateCol(0.5f, 0.5f, 0.5f, 1.0f);
                    switch (jt.TrackingState)
                    {
                    case TrackingState_Tracked:    stateStr = "Tracked";  stateCol = ImVec4(0.3f, 1.0f, 0.3f, 1.0f); break;
                    case TrackingState_Inferred:   stateStr = "Inferred"; stateCol = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;
                    case TrackingState_NotTracked: stateStr = "Lost";     stateCol = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); break;
                    }
                    ImGui::TextColored(stateCol, "[%s]", stateStr);
                    ImGui::SameLine();
                    ImGui::TextDisabled("  %.2f, %.2f, %.2f", jt.Position.X, jt.Position.Y, jt.Position.Z);
                }

                int removeIdx = -1;
                for (size_t m = 0; m < activeMappings[j].items.size(); ++m)
                {
                    ImGui::PushID((int)m);
                    SingleMapping& sm = activeMappings[j].items[m];
                    
                    ImGui::SetNextItemWidth(90);
                    if (ImGui::Combo("##Dir", (int*)&sm.direction, g_DirNames, (int)GestureDir::Count))
                        configChanged = true;
                    ImGui::SameLine();
                    
                    char bindLabel[64];
                    if (g_bindingMapping == &sm) {
                        strcpy_s(bindLabel, "[ Press a key... ]");
                    } else if (sm.keyCode == 0) {
                        strcpy_s(bindLabel, "[ None ]");
                    } else {
                        char keyName[32] = {};
                        UINT scanCode = MapVirtualKeyW(sm.keyCode, MAPVK_VK_TO_VSC);
                        if (scanCode != 0 || sm.keyCode != 0) {
                            switch (sm.keyCode) {
                                case VK_LBUTTON: strcpy_s(keyName, "LClick"); break;
                                case VK_RBUTTON: strcpy_s(keyName, "RClick"); break;
                                case VK_MBUTTON: strcpy_s(keyName, "MClick"); break;
                                case VK_XBUTTON1: strcpy_s(keyName, "XClick1"); break;
                                case VK_XBUTTON2: strcpy_s(keyName, "XClick2"); break;
                                default:
                                    GetKeyNameTextA(scanCode << 16, keyName, 32);
                                    if (keyName[0] == 0) sprintf_s(keyName, "VK_%d", sm.keyCode);
                                    break;
                            }
                        }
                        sprintf_s(bindLabel, "[ %s ]", keyName);
                    }
                    
                    if (ImGui::Button(bindLabel, ImVec2(90, 0))) {
                        ReleaseAllInputs();
                        g_bindingMapping = &sm;
                    }
                    
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70);
                    if (ImGui::Combo("##Beh", (int*)&sm.behavior, g_BehaviorNames, (int)GestureBehavior::Count))
                        configChanged = true;
                    
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(70);
                    if (ImGui::SliderFloat("##Thr", &sm.threshold, 0.01f, 1.0f, "%.2f"))
                        configChanged = true;
                        
                    ImGui::SameLine();
                    if (ImGui::Button("X")) {
                        removeIdx = (int)m;
                    }
                    
                    ImGui::PopID();
                }
                
                if (removeIdx >= 0) {
                    ReleaseAllInputs(); // In case we removed an active mapping
                    if (g_bindingMapping == &activeMappings[j].items[removeIdx])
                        g_bindingMapping = nullptr;
                    activeMappings[j].items.erase(activeMappings[j].items.begin() + removeIdx);
                    configChanged = true;
                }
                
                if (ImGui::Button("+ Add Mapping")) {
                    activeMappings[j].items.push_back(SingleMapping());
                    configChanged = true;
                }

                ImGui::Unindent();
                ImGui::Spacing();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::Text("Selected: %d / %d", selectedJointCount, JointType_Count);

        // Auto-save on any config change
        if (configChanged)
            ConfigSave();

        ImGui::Spacing();
        ImGui::Spacing();

        // ----- Diagnostics section -----
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "Diagnostics");
        ImGui::Separator();

        // Kinect connection status
        BOOLEAN isAvailable = false;
        if (g_pKinectSensor)
            g_pKinectSensor->get_IsAvailable(&isAvailable);

        if (isAvailable)
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Kinect: Connected");
        else if (g_pKinectSensor)
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Kinect: Not Available");
        else
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Kinect: Not Found");

        ImGui::Text("Kinect FPS: %.1f", g_kinectBodyFps);
        ImGui::Text("Bodies: %d", g_trackedBodyCount);
        ImGui::Text("Tracking: %s", g_trackedBodyCount > 0 ? "ACTIVE" : "IDLE");

        ImGui::Spacing();
        ImGui::Text("Distance:");

        // Distance status with colored boxes
        switch (g_distStatus)
        {
        case DIST_TOO_CLOSE:
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float w = ImGui::GetContentRegionAvail().x;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 30), IM_COL32(200, 30, 30, 255), 4.0f);
            dl->AddText(ImVec2(p.x + w * 0.5f - 35, p.y + 7), IM_COL32(255, 255, 255, 255), "TOO CLOSE");
            ImGui::Dummy(ImVec2(w, 30));
            break;
        }
        case DIST_TOO_FAR:
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float w = ImGui::GetContentRegionAvail().x;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 30), IM_COL32(200, 30, 30, 255), 4.0f);
            dl->AddText(ImVec2(p.x + w * 0.5f - 25, p.y + 7), IM_COL32(255, 255, 255, 255), "TOO FAR");
            ImGui::Dummy(ImVec2(w, 30));
            break;
        }
        case DIST_GOOD:
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float w = ImGui::GetContentRegionAvail().x;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 30), IM_COL32(30, 150, 30, 255), 4.0f);
            dl->AddText(ImVec2(p.x + w * 0.5f - 17, p.y + 7), IM_COL32(255, 255, 255, 255), "GOOD");
            ImGui::Dummy(ImVec2(w, 30));
            break;
        }
        case DIST_NO_PERSON:
        default:
        {
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float w = ImGui::GetContentRegionAvail().x;
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + 30), IM_COL32(80, 80, 80, 255), 4.0f);
            dl->AddText(ImVec2(p.x + w * 0.5f - 35, p.y + 7), IM_COL32(180, 180, 180, 255), "NO PERSON");
            ImGui::Dummy(ImVec2(w, 30));
            break;
        }
        }
    }
    ImGui::EndChild();

    ImGui::End();
}

