#include "../include/globals.h"

// =====================================================================
// Input Mapping Evaluation
// =====================================================================

void SendInputEvent(int vk, bool down)
{
    if (vk == 0) return;
    INPUT in = {};
    if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON || vk == VK_XBUTTON1 || vk == VK_XBUTTON2)
    {
        in.type = INPUT_MOUSE;
        if (vk == VK_LBUTTON) in.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
        else if (vk == VK_RBUTTON) in.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
        else if (vk == VK_MBUTTON) in.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
        else if (vk == VK_XBUTTON1 || vk == VK_XBUTTON2) {
            in.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
            in.mi.mouseData = (vk == VK_XBUTTON1) ? XBUTTON1 : XBUTTON2;
        }
    }
    else
    {
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = (WORD)vk;
        if (!down) in.ki.dwFlags = KEYEVENTF_KEYUP;
    }
    SendInput(1, &in, sizeof(INPUT));
}

bool g_heldKeys[256] = {false};

void EvaluateMapping(int personIndex, JointType joint, JointMapping& mapping, bool forceRelease, bool outFrameHoldKeys[256])
{
    for (auto& sm : mapping.items)
    {
        if (sm.keyCode <= 0 || sm.keyCode >= 256) continue;

        bool shouldBeActive = false;
        
        if (!forceRelease && g_bodies[personIndex].joints[joint].TrackingState != TrackingState_NotTracked && g_bodies[personIndex].joints[JointType_SpineBase].TrackingState != TrackingState_NotTracked)
        {
            const CameraSpacePoint& p = g_bodies[personIndex].joints[joint].Position;
            const CameraSpacePoint& anchor = g_bodies[personIndex].joints[JointType_SpineBase].Position;
            float thresh = sm.threshold;

            if (sm.direction == GestureDir::Forward)  shouldBeActive = (p.Z - anchor.Z) < -thresh;
            else if (sm.direction == GestureDir::Backward) shouldBeActive = (p.Z - anchor.Z) > thresh;
            else if (sm.direction == GestureDir::Left)     shouldBeActive = (p.X - anchor.X) < -thresh;
            else if (sm.direction == GestureDir::Right)    shouldBeActive = (p.X - anchor.X) > thresh;
            else if (sm.direction == GestureDir::Up)       shouldBeActive = (p.Y - anchor.Y) > thresh;
            else if (sm.direction == GestureDir::Down)     shouldBeActive = (p.Y - anchor.Y) < -thresh;
        }

        if (sm.behavior == GestureBehavior::Hold)
        {
            if (shouldBeActive) {
                sm.isActive[personIndex] = true;
                outFrameHoldKeys[sm.keyCode] = true;
            } else {
                sm.isActive[personIndex] = false;
            }
        }
        else if (sm.behavior == GestureBehavior::Single)
        {
            if (shouldBeActive && !sm.isActive[personIndex]) {
                SendInputEvent(sm.keyCode, true);
                SendInputEvent(sm.keyCode, false);
                sm.isActive[personIndex] = true;
            }
            else if (!shouldBeActive && sm.isActive[personIndex]) {
                sm.isActive[personIndex] = false;
            }
        }
    }
}

void ProcessInputMappings()
{
    bool frameHoldKeys[256] = {false};

    for (int i = 0; i < BODY_COUNT; ++i)
    {
        bool bodyValid = g_config.globalInputEnabled && g_config.persons[i].enabled && g_bodies[i].tracked;
        
        for (int j = 0; j < JointType_Count; ++j)
        {
            bool autoMapped = g_config.autoJoints[j] && !g_config.autoMappings[j].items.empty();
            EvaluateMapping(i, (JointType)j, g_config.autoMappings[j], !bodyValid || !autoMapped, frameHoldKeys);
            
            bool personMapped = g_config.persons[i].joints[j] && !g_config.persons[i].mappings[j].items.empty();
            EvaluateMapping(i, (JointType)j, g_config.persons[i].mappings[j], !bodyValid || !personMapped, frameHoldKeys);
        }
    }

    // Resolve Hold states globally
    for (int k = 1; k < 256; ++k)
    {
        if (frameHoldKeys[k] && !g_heldKeys[k]) {
            SendInputEvent(k, true);
            g_heldKeys[k] = true;
        }
        else if (!frameHoldKeys[k] && g_heldKeys[k]) {
            SendInputEvent(k, false);
            g_heldKeys[k] = false;
        }
    }
}

void ReleaseAllInputs()
{
    for (int k = 1; k < 256; ++k) {
        if (g_heldKeys[k]) {
            SendInputEvent(k, false);
            g_heldKeys[k] = false;
        }
    }
    
    // Also clear isActive state from all mapping items so they don't incorrectly
    // think they are holding keys after we released them globally
    for (int j = 0; j < JointType_Count; ++j) {
        for (auto& sm : g_config.autoMappings[j].items) {
            memset(sm.isActive, 0, sizeof(sm.isActive));
        }
        for (int i = 0; i < BODY_COUNT; ++i) {
            for (auto& sm : g_config.persons[i].mappings[j].items) {
                memset(sm.isActive, 0, sizeof(sm.isActive));
            }
        }
    }
}

