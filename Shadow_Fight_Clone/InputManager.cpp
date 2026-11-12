#include "InputManager.h"
#include <GL/glut.h>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>

namespace {
    const char* CONTROLS_FILE = "shadow_fight_controls.dat";

    struct XInputGamepadRaw {
        WORD  wButtons;
        BYTE  bLeftTrigger;
        BYTE  bRightTrigger;
        SHORT sThumbLX;
        SHORT sThumbLY;
        SHORT sThumbRX;
        SHORT sThumbRY;
    };

    struct XInputStateRaw {
        DWORD            dwPacketNumber;
        XInputGamepadRaw Gamepad;
    };

    typedef DWORD (WINAPI *PFN_XInputGetState)(DWORD, XInputStateRaw*);

    PFN_XInputGetState loadXInputGetState() {
        static bool attempted = false;
        static PFN_XInputGetState fn = nullptr;
        if (attempted) return fn;
        attempted = true;

        const char* dlls[3] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
        for (int i = 0; i < 3; ++i) {
            HMODULE mod = LoadLibraryA(dlls[i]);
            if (mod != nullptr) {
                fn = (PFN_XInputGetState)(void*)GetProcAddress(mod, "XInputGetState");
                if (fn != nullptr) return fn;
            }
        }
        return nullptr;
    }

    float normalizeJoyAxis(DWORD val, UINT minVal, UINT maxVal) {
        if (maxVal <= minVal) return 0.0f;
        float mid  = 0.5f * ((float)minVal + (float)maxVal);
        float half = 0.5f * ((float)maxVal - (float)minVal);
        if (half < 1.0f) return 0.0f;
        return ((float)val - mid) / half;
    }

    const char* padButtonLabel(PadButton b) {
        switch (b) {
            case PadButton::DPAD_UP:     return "D-Pad Up";
            case PadButton::DPAD_DOWN:   return "D-Pad Down";
            case PadButton::DPAD_LEFT:   return "D-Pad Left";
            case PadButton::DPAD_RIGHT:  return "D-Pad Right";
            case PadButton::STICK_UP:    return "Stick Up";
            case PadButton::STICK_DOWN:  return "Stick Down";
            case PadButton::STICK_LEFT:  return "Stick Left";
            case PadButton::STICK_RIGHT: return "Stick Right";
            case PadButton::A:           return "A";
            case PadButton::B:           return "B";
            case PadButton::X:           return "X";
            case PadButton::Y:           return "Y";
            case PadButton::LB:          return "LB";
            case PadButton::RB:          return "RB";
            case PadButton::LT:          return "LT";
            case PadButton::RT:          return "RT";
            case PadButton::BACK:        return "BACK";
            case PadButton::START:       return "START";
            default:                     return "None";
        }
    }

    bool isTriggerOrBumper(PadButton b) {
        return b == PadButton::LT || b == PadButton::RT ||
               b == PadButton::LB || b == PadButton::RB;
    }
}

InputManager::InputManager()
    : gamepadConnected(false),
      gamepadBackend("None"),
      rebindingActive(false),
      rebindingPad(false),
      rebindTarget(GameAction::MOVE_LEFT),
      rebindPendingTrigger(PadButton::NONE) {
    for (int i = 0; i < KeyCodes::MAX_KEYS; i++) {
        keys[i] = false;
        justPressed[i] = false;
    }
    for (int i = 0; i < (int)PadButton::COUNT; i++) {
        padBtnDown[i] = false;
        padBtnJustPressed[i] = false;
    }
    for (int i = 0; i < (int)GameAction::COUNT; i++) {
        actionDown[i] = false;
        actionJustPressed[i] = false;
    }
    for (int i = 0; i < (int)GamepadNav::COUNT; i++) {
        gpNavDown[i] = false;
        gpNavJustPressed[i] = false;
    }
    resetDefaultBindings();
}

void InputManager::resetDefaultBindings() {
    keyBindings[(int)GameAction::MOVE_LEFT]  = 'a';
    keyBindings[(int)GameAction::MOVE_RIGHT] = 'd';
    keyBindings[(int)GameAction::JUMP]       = 'w';
    keyBindings[(int)GameAction::CROUCH]     = 's';
    keyBindings[(int)GameAction::RUN_HOLD]   = KeyCodes::SHIFT;
    keyBindings[(int)GameAction::PUNCH]      = 'j';
    keyBindings[(int)GameAction::KICK]       = 'k';
    keyBindings[(int)GameAction::DODGE]      = 'u';
    keyBindings[(int)GameAction::CHAIN_GRAB] = 'o';
    keyBindings[(int)GameAction::BLOCK]      = 'i';
    keyBindings[(int)GameAction::CRITICAL]   = 'l';

    // Exact controller defaults requested:
    // B=Dodge, A=Crouch, Y=Kick, X=Punch, RB=Block, LT+Stick=Run, RT+X=Critical
    padBindings[(int)GameAction::MOVE_LEFT]  = { PadButton::STICK_LEFT,  PadButton::NONE };
    padBindings[(int)GameAction::MOVE_RIGHT] = { PadButton::STICK_RIGHT, PadButton::NONE };
    padBindings[(int)GameAction::JUMP]       = { PadButton::STICK_UP,    PadButton::NONE };
    padBindings[(int)GameAction::CROUCH]     = { PadButton::A,           PadButton::NONE };
    padBindings[(int)GameAction::RUN_HOLD]   = { PadButton::LT,          PadButton::NONE };
    padBindings[(int)GameAction::PUNCH]      = { PadButton::X,           PadButton::NONE };
    padBindings[(int)GameAction::KICK]       = { PadButton::Y,           PadButton::NONE };
    padBindings[(int)GameAction::DODGE]      = { PadButton::B,           PadButton::NONE };
    padBindings[(int)GameAction::CHAIN_GRAB] = { PadButton::LB,          PadButton::NONE };
    padBindings[(int)GameAction::BLOCK]      = { PadButton::RB,          PadButton::NONE };
    padBindings[(int)GameAction::CRITICAL]   = { PadButton::X,           PadButton::RT   };
}

void InputManager::loadBindings() {
    resetDefaultBindings();
    std::ifstream in(CONTROLS_FILE);
    if (!in.is_open()) return;

    std::string line;
    while (std::getline(in, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        std::istringstream ss(val);

        for (int i = 0; i < (int)GameAction::COUNT; ++i) {
            char kName[32], pName[32], mName[32];
            std::snprintf(kName, sizeof(kName), "key_%d", i);
            std::snprintf(pName, sizeof(pName), "pad_%d", i);
            std::snprintf(mName, sizeof(mName), "mod_%d", i);

            int num = -1;
            if (key == kName && (ss >> num) && num > 0 && num < KeyCodes::MAX_KEYS) {
                keyBindings[i] = num;
            } else if (key == pName && (ss >> num) && num >= -1 && num < (int)PadButton::COUNT) {
                padBindings[i].button = (PadButton)num;
            } else if (key == mName && (ss >> num) && num >= -1 && num < (int)PadButton::COUNT) {
                padBindings[i].modifier = (PadButton)num;
            }
        }
    }
}

void InputManager::saveBindings() const {
    std::ofstream out(CONTROLS_FILE, std::ios::trunc);
    if (!out.is_open()) return;
    for (int i = 0; i < (int)GameAction::COUNT; ++i) {
        out << "key_" << i << "=" << keyBindings[i] << "\n";
        out << "pad_" << i << "=" << (int)padBindings[i].button << "\n";
        out << "mod_" << i << "=" << (int)padBindings[i].modifier << "\n";
    }
}

int InputManager::mapGlutSpecial(int glutKey) const {
    switch (glutKey) {
        case GLUT_KEY_UP:    return KeyCodes::ARROW_UP;
        case GLUT_KEY_DOWN:  return KeyCodes::ARROW_DOWN;
        case GLUT_KEY_LEFT:  return KeyCodes::ARROW_LEFT;
        case GLUT_KEY_RIGHT: return KeyCodes::ARROW_RIGHT;
        default:             return -1;
    }
}

void InputManager::setCodeState(int keyCode, bool down, bool /*isGamepad*/) {
    if (keyCode < 0 || keyCode >= KeyCodes::MAX_KEYS) return;
    if (down && !keys[keyCode]) {
        justPressed[keyCode] = true;
    }
    keys[keyCode] = down;
}

void InputManager::keyDown(unsigned char key) {
    unsigned char k = (unsigned char)std::tolower(key);
    setCodeState((int)k, true, false);
}

void InputManager::keyUp(unsigned char key) {
    unsigned char k = (unsigned char)std::tolower(key);
    setCodeState((int)k, false, false);
}

void InputManager::specialKeyDown(int glutKey) {
    int code = mapGlutSpecial(glutKey);
    if (code >= 0) {
        if (rebindingActive && !rebindingPad) {
            feedKeyForRebind(code);
            return;
        }
        setCodeState(code, true, false);
    }
}

void InputManager::specialKeyUp(int glutKey) {
    int code = mapGlutSpecial(glutKey);
    if (code >= 0) setCodeState(code, false, false);
}

void InputManager::setPadBtnState(PadButton btn, bool down) {
    int idx = (int)btn;
    if (idx < 0 || idx >= (int)PadButton::COUNT) return;
    if (down && !padBtnDown[idx]) {
        padBtnJustPressed[idx] = true;
    }
    padBtnDown[idx] = down;
}

void InputManager::setGamepadNav(GamepadNav nav, bool down) {
    int idx = (int)nav;
    if (idx < 0 || idx >= (int)GamepadNav::COUNT) return;
    if (down && !gpNavDown[idx]) {
        gpNavJustPressed[idx] = true;
    }
    gpNavDown[idx] = down;
}

bool InputManager::evalPadBindingDown(int actionIdx) const {
    if (!gamepadConnected) return false;
    GamepadBinding b = padBindings[actionIdx];
    if (b.button == PadButton::NONE) return false;

    auto isBtnActive = [&](PadButton btn) -> bool {
        int id = (int)btn;
        if (id < 0 || id >= (int)PadButton::COUNT) return false;
        if (padBtnDown[id]) return true;
        // D-Pad and Left Stick naturally mirror each other for movement & jump
        if (btn == PadButton::STICK_LEFT  && padBtnDown[(int)PadButton::DPAD_LEFT])  return true;
        if (btn == PadButton::DPAD_LEFT   && padBtnDown[(int)PadButton::STICK_LEFT]) return true;
        if (btn == PadButton::STICK_RIGHT && padBtnDown[(int)PadButton::DPAD_RIGHT]) return true;
        if (btn == PadButton::DPAD_RIGHT  && padBtnDown[(int)PadButton::STICK_RIGHT])return true;
        if (btn == PadButton::STICK_UP    && padBtnDown[(int)PadButton::DPAD_UP])    return true;
        if (btn == PadButton::DPAD_UP     && padBtnDown[(int)PadButton::STICK_UP])   return true;
        return false;
    };

    if (!isBtnActive(b.button)) return false;

    if (b.modifier != PadButton::NONE) {
        return isBtnActive(b.modifier);
    }

    // If this action uses NO modifier, suppress it when another action's combo modifier using the SAME button is held!
    // Example: holding RT + pressing X fires CRITICAL without also firing single-X PUNCH.
    for (int other = 0; other < (int)GameAction::COUNT; ++other) {
        if (other == actionIdx) continue;
        if (padBindings[other].modifier != PadButton::NONE &&
            padBindings[other].button == b.button &&
            isBtnActive(padBindings[other].modifier)) {
            return false;
        }
    }

    return true;
}

void InputManager::pollGamepad() {
    // Poll Windows modifier keys continuously so Shift+A / Shift+D runs even if Shift is pressed after A/D
    bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrlHeld  = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    setCodeState(KeyCodes::SHIFT, shiftHeld, false);
    setCodeState(KeyCodes::CTRL,  ctrlHeld,  false);

    if (rebindingActive && !rebindingPad) {
        if (justPressed[KeyCodes::SHIFT]) feedKeyForRebind(KeyCodes::SHIFT);
        else if (justPressed[KeyCodes::CTRL]) feedKeyForRebind(KeyCodes::CTRL);
    }

    bool dUp = false, dDown = false, dLeft = false, dRight = false;
    bool sUp = false, sDown = false, sLeft = false, sRight = false;
    bool btnA = false, btnB = false, btnX = false, btnY = false;
    bool btnLB = false, btnRB = false, btnLT = false, btnRT = false;
    bool btnBack = false, btnStart = false;

    bool found = false;

    // 1. Try XInput (Xbox 360 / Xbox One / Series X|S / XInput controllers)
    PFN_XInputGetState xinputFn = loadXInputGetState();
    if (xinputFn != nullptr) {
        for (DWORD slot = 0; slot < 4; ++slot) {
            XInputStateRaw st = {};
            if (xinputFn(slot, &st) == ERROR_SUCCESS) {
                found = true;
                gamepadBackend = "XInput Gamepad";
                WORD w = st.Gamepad.wButtons;
                dUp      = ((w & 0x0001) != 0);
                dDown    = ((w & 0x0002) != 0);
                dLeft    = ((w & 0x0004) != 0);
                dRight   = ((w & 0x0008) != 0);
                sUp      = (st.Gamepad.sThumbLY >  13500);
                sDown    = (st.Gamepad.sThumbLY < -13500);
                sLeft    = (st.Gamepad.sThumbLX <  -7849);
                sRight   = (st.Gamepad.sThumbLX >   7849);
                btnStart = ((w & 0x0010) != 0);
                btnBack  = ((w & 0x0020) != 0);
                btnLB    = ((w & 0x0100) != 0);
                btnRB    = ((w & 0x0200) != 0);
                btnA     = ((w & 0x1000) != 0);
                btnB     = ((w & 0x2000) != 0);
                btnX     = ((w & 0x4000) != 0);
                btnY     = ((w & 0x8000) != 0);
                btnLT    = (st.Gamepad.bLeftTrigger  > 30);
                btnRT    = (st.Gamepad.bRightTrigger > 30);
                break;
            }
        }
    }

    // 2. Fallback to WinMM joystick API (USB / DirectInput / PlayStation controllers)
    if (!found) {
        UINT numDevs = joyGetNumDevs();
        if (numDevs > 4) numDevs = 4;
        for (UINT id = 0; id < numDevs; ++id) {
            JOYINFOEX ji = {};
            ji.dwSize = sizeof(JOYINFOEX);
            ji.dwFlags = JOY_RETURNALL;
            if (joyGetPosEx(id, &ji) == JOYERR_NOERROR) {
                found = true;
                gamepadBackend = "USB Controller";

                JOYCAPSA caps = {};
                UINT xMin = 0, xMax = 65535, yMin = 0, yMax = 65535;
                if (joyGetDevCapsA(id, &caps, sizeof(caps)) == JOYERR_NOERROR) {
                    xMin = caps.wXmin; xMax = caps.wXmax;
                    yMin = caps.wYmin; yMax = caps.wYmax;
                }

                float ax = normalizeJoyAxis(ji.dwXpos, xMin, xMax);
                float ay = normalizeJoyAxis(ji.dwYpos, yMin, yMax);

                sLeft  = (ax < -0.28f);
                sRight = (ax >  0.28f);
                sUp    = (ay < -0.40f);
                sDown  = (ay >  0.40f);

                if (ji.dwPOV != JOY_POVCENTERED && ji.dwPOV < 36000) {
                    if (ji.dwPOV >= 31500 || ji.dwPOV <= 4500)  dUp    = true;
                    if (ji.dwPOV >= 4500  && ji.dwPOV <= 13500) dRight = true;
                    if (ji.dwPOV >= 13500 && ji.dwPOV <= 22500) dDown  = true;
                    if (ji.dwPOV >= 22500 && ji.dwPOV <= 31500) dLeft  = true;
                }

                DWORD b = ji.dwButtons;
                btnA     = ((b & (1u << 0)) != 0);
                btnB     = ((b & (1u << 1)) != 0);
                btnX     = ((b & (1u << 2)) != 0);
                btnY     = ((b & (1u << 3)) != 0);
                btnLB    = ((b & (1u << 4)) != 0);
                btnRB    = ((b & (1u << 5)) != 0);
                btnLT    = ((b & (1u << 6)) != 0);
                btnRT    = ((b & (1u << 7)) != 0);
                btnBack  = ((b & (1u << 8)) != 0);
                btnStart = ((b & (1u << 9)) != 0);
                break;
            }
        }
    }

    gamepadConnected = found;
    if (!found) {
        gamepadBackend = "None";
    }

    setPadBtnState(PadButton::DPAD_UP,     dUp);
    setPadBtnState(PadButton::DPAD_DOWN,   dDown);
    setPadBtnState(PadButton::DPAD_LEFT,   dLeft);
    setPadBtnState(PadButton::DPAD_RIGHT,  dRight);
    setPadBtnState(PadButton::STICK_UP,    sUp);
    setPadBtnState(PadButton::STICK_DOWN,  sDown);
    setPadBtnState(PadButton::STICK_LEFT,  sLeft);
    setPadBtnState(PadButton::STICK_RIGHT, sRight);
    setPadBtnState(PadButton::A,           btnA);
    setPadBtnState(PadButton::B,           btnB);
    setPadBtnState(PadButton::X,           btnX);
    setPadBtnState(PadButton::Y,           btnY);
    setPadBtnState(PadButton::LB,          btnLB);
    setPadBtnState(PadButton::RB,          btnRB);
    setPadBtnState(PadButton::LT,          btnLT);
    setPadBtnState(PadButton::RT,          btnRT);
    setPadBtnState(PadButton::BACK,        btnBack);
    setPadBtnState(PadButton::START,       btnStart);

    // Handle live Gamepad Rebinding Capture if waiting for a controller button
    bool suppressNavThisPoll = (rebindingActive && rebindingPad);
    if (suppressNavThisPoll) {
        if (padBtnJustPressed[(int)PadButton::START] || padBtnJustPressed[(int)PadButton::BACK]) {
            cancelRebind();
        } else {
            // Check if a held modifier trigger/bumper was released cleanly on its own (e.g. binding LT alone to Run)
            if (rebindPendingTrigger != PadButton::NONE && !padBtnDown[(int)rebindPendingTrigger]) {
                setPadBinding(rebindTarget, { rebindPendingTrigger, PadButton::NONE });
                saveBindings();
                rebindingActive = false;
                rebindPendingTrigger = PadButton::NONE;
            } else {
                // Find any newly pressed non-modifier button OR start holding a trigger/bumper
                PadButton heldMod = PadButton::NONE;
                PadButton modList[4] = { PadButton::LT, PadButton::RT, PadButton::LB, PadButton::RB };
                for (int m = 0; m < 4; ++m) {
                    if (padBtnDown[(int)modList[m]]) {
                        heldMod = modList[m];
                        break;
                    }
                }

                bool captured = false;
                for (int bIdx = 0; bIdx < (int)PadButton::COUNT; ++bIdx) {
                    PadButton pb = (PadButton)bIdx;
                    if (pb == PadButton::START || pb == PadButton::BACK) continue;
                    if (!padBtnJustPressed[bIdx]) continue;

                    if (isTriggerOrBumper(pb)) {
                        // Allow holding trigger/bumper to form a combo like RT + X, or release for single button
                        if (rebindTarget == GameAction::RUN_HOLD || rebindTarget == GameAction::BLOCK) {
                            setPadBinding(rebindTarget, { pb, PadButton::NONE });
                            saveBindings();
                            rebindingActive = false;
                            rebindPendingTrigger = PadButton::NONE;
                            captured = true;
                        } else {
                            rebindPendingTrigger = pb;
                        }
                        break;
                    } else {
                        PadButton modToUse = (heldMod != PadButton::NONE && heldMod != pb) ? heldMod : PadButton::NONE;
                        setPadBinding(rebindTarget, { pb, modToUse });
                        saveBindings();
                        rebindingActive = false;
                        rebindPendingTrigger = PadButton::NONE;
                        captured = true;
                        break;
                    }
                }
                (void)captured;
            }
        }
    }

    // Evaluate combined held + edge state for all logical combat actions
    for (int i = 0; i < (int)GameAction::COUNT; ++i) {
        int kc = keyBindings[i];
        bool kbNow = (kc > 0 && kc < KeyCodes::MAX_KEYS) ? keys[kc] : false;
        // Keep SPACE as built-in secondary Jump when JUMP is bound to 'w' and SPACE isn't bound elsewhere
        if (i == (int)GameAction::JUMP && kc == 'w' && keys[(int)' ']) {
            bool spaceUsedElsewhere = false;
            for (int j = 0; j < (int)GameAction::COUNT; ++j) {
                if (j != i && keyBindings[j] == ' ') spaceUsedElsewhere = true;
            }
            if (!spaceUsedElsewhere) kbNow = true;
        }

        bool gpNow = evalPadBindingDown(i);
        // Also let Stick/D-Pad Down crouch naturally unless Down is bound to another action
        if (i == (int)GameAction::CROUCH && gamepadConnected &&
            (padBtnDown[(int)PadButton::STICK_DOWN] || padBtnDown[(int)PadButton::DPAD_DOWN])) {
            gpNow = true;
        }

        bool now = kbNow || gpNow;
        if (now && !actionDown[i]) {
            actionJustPressed[i] = true;
        }
        actionDown[i] = now;
    }

    // Menu / UI navigation edges (suppressed while finishing/canceling pad rebind capture)
    setGamepadNav(GamepadNav::UP,        !suppressNavThisPoll && (dUp || sUp));
    setGamepadNav(GamepadNav::DOWN,      !suppressNavThisPoll && (dDown || sDown));
    setGamepadNav(GamepadNav::LEFT,      !suppressNavThisPoll && (dLeft || sLeft));
    setGamepadNav(GamepadNav::RIGHT,     !suppressNavThisPoll && (dRight || sRight));
    setGamepadNav(GamepadNav::CONFIRM,   !suppressNavThisPoll && btnA);
    setGamepadNav(GamepadNav::CANCEL,    !suppressNavThisPoll && btnB);
    setGamepadNav(GamepadNav::START,     !suppressNavThisPoll && btnStart);
    setGamepadNav(GamepadNav::SPECIAL_Y, !suppressNavThisPoll && btnY);
    setGamepadNav(GamepadNav::LB,        !suppressNavThisPoll && btnLB);
    setGamepadNav(GamepadNav::RB,        !suppressNavThisPoll && btnRB);
}

bool InputManager::isKeyDown(unsigned char key) const {
    return isCodeDown((int)std::tolower(key));
}

bool InputManager::isCodeDown(int keyCode) const {
    if (keyCode < 0 || keyCode >= KeyCodes::MAX_KEYS) return false;
    return keys[keyCode];
}

bool InputManager::wasKeyPressed(unsigned char key) const {
    unsigned char k = (unsigned char)std::tolower(key);
    return justPressed[(int)k];
}

bool InputManager::wasGamepadNavPressed(GamepadNav nav) const {
    int idx = (int)nav;
    if (idx < 0 || idx >= (int)GamepadNav::COUNT) return false;
    return gpNavJustPressed[idx];
}

bool InputManager::isGamepadConnected() const {
    return gamepadConnected;
}

const char* InputManager::getGamepadBackendName() const {
    return gamepadBackend;
}

bool InputManager::isActionDown(GameAction action) const {
    int idx = (int)action;
    if (idx < 0 || idx >= (int)GameAction::COUNT) return false;
    return actionDown[idx];
}

bool InputManager::wasActionPressed(GameAction action) const {
    int idx = (int)action;
    if (idx < 0 || idx >= (int)GameAction::COUNT) return false;
    return actionJustPressed[idx];
}

int InputManager::getKeyBinding(GameAction action) const {
    int idx = (int)action;
    if (idx < 0 || idx >= (int)GameAction::COUNT) return 0;
    return keyBindings[idx];
}

void InputManager::setKeyBinding(GameAction action, int keyCode) {
    int idx = (int)action;
    if (idx < 0 || idx >= (int)GameAction::COUNT) return;
    int oldKey = keyBindings[idx];
    for (int i = 0; i < (int)GameAction::COUNT; ++i) {
        if (i != idx && keyBindings[i] == keyCode) {
            keyBindings[i] = oldKey; // swap conflicting binding cleanly
        }
    }
    keyBindings[idx] = keyCode;
}

GamepadBinding InputManager::getPadBinding(GameAction action) const {
    int idx = (int)action;
    if (idx < 0 || idx >= (int)GameAction::COUNT) return { PadButton::NONE, PadButton::NONE };
    return padBindings[idx];
}

void InputManager::setPadBinding(GameAction action, GamepadBinding binding) {
    int idx = (int)action;
    if (idx < 0 || idx >= (int)GameAction::COUNT) return;
    GamepadBinding oldBind = padBindings[idx];
    for (int i = 0; i < (int)GameAction::COUNT; ++i) {
        if (i != idx &&
            padBindings[i].button == binding.button &&
            padBindings[i].modifier == binding.modifier) {
            padBindings[i] = oldBind; // swap identical conflict cleanly
        }
    }
    padBindings[idx] = binding;
}

const char* InputManager::getActionName(GameAction action) {
    switch (action) {
        case GameAction::MOVE_LEFT:  return "Move Left";
        case GameAction::MOVE_RIGHT: return "Move Right";
        case GameAction::JUMP:       return "Jump";
        case GameAction::CROUCH:     return "Crouch";
        case GameAction::RUN_HOLD:   return "Run (Hold + Move)";
        case GameAction::PUNCH:      return "Punch / Combo";
        case GameAction::KICK:       return "Kick / Air Kick";
        case GameAction::DODGE:      return "Dodge Roll";
        case GameAction::CHAIN_GRAB: return "Chain Grab (Kuro)";
        case GameAction::BLOCK:      return "Block Guard";
        case GameAction::CRITICAL:   return "Critical / Special";
        default:                     return "Unknown";
    }
}

const char* InputManager::getActionDesc(GameAction action) {
    switch (action) {
        case GameAction::MOVE_LEFT:  return "Step or run left";
        case GameAction::MOVE_RIGHT: return "Step or run right";
        case GameAction::JUMP:       return "Leap into air / crossover";
        case GameAction::CROUCH:     return "Duck under high attacks";
        case GameAction::RUN_HOLD:   return "Hold + Move Left/Right to sprint";
        case GameAction::PUNCH:      return "Tap x3 for 3-Hit Weapon/Fist Combo";
        case GameAction::KICK:       return "Unblockable kick / Jump+Kick in air";
        case GameAction::DODGE:      return "Roll under punches/chain (Ch 1,3,4)";
        case GameAction::CHAIN_GRAB: return "Long spiked pull + 2s stun (Ch 2)";
        case GameAction::BLOCK:      return "Hold to block incoming attacks";
        case GameAction::CRITICAL:   return "Character signature Critical Attack";
        default:                     return "";
    }
}

void InputManager::formatKeyName(int keyCode, char* outBuf, int bufSize) {
    if (keyCode == KeyCodes::SHIFT)       std::snprintf(outBuf, bufSize, "SHIFT");
    else if (keyCode == KeyCodes::CTRL)   std::snprintf(outBuf, bufSize, "CTRL");
    else if (keyCode == KeyCodes::ARROW_UP)    std::snprintf(outBuf, bufSize, "UP ARROW");
    else if (keyCode == KeyCodes::ARROW_DOWN)  std::snprintf(outBuf, bufSize, "DOWN ARROW");
    else if (keyCode == KeyCodes::ARROW_LEFT)  std::snprintf(outBuf, bufSize, "LEFT ARROW");
    else if (keyCode == KeyCodes::ARROW_RIGHT) std::snprintf(outBuf, bufSize, "RIGHT ARROW");
    else if (keyCode == ' ')              std::snprintf(outBuf, bufSize, "SPACE");
    else if (keyCode >= 'a' && keyCode <= 'z') std::snprintf(outBuf, bufSize, "%c", (char)std::toupper(keyCode));
    else if (keyCode >= 33 && keyCode < 127)   std::snprintf(outBuf, bufSize, "%c", (char)keyCode);
    else                                       std::snprintf(outBuf, bufSize, "KEY %d", keyCode);
}

void InputManager::formatPadBinding(GamepadBinding binding, char* outBuf, int bufSize) {
    if (binding.button == PadButton::NONE) {
        std::snprintf(outBuf, bufSize, "UNBOUND");
        return;
    }
    if (binding.modifier != PadButton::NONE) {
        std::snprintf(outBuf, bufSize, "%s + %s",
                      padButtonLabel(binding.modifier), padButtonLabel(binding.button));
    } else {
        std::snprintf(outBuf, bufSize, "%s", padButtonLabel(binding.button));
    }
}

void InputManager::beginKeyRebind(GameAction action) {
    rebindingActive      = true;
    rebindingPad         = false;
    rebindTarget         = action;
    rebindPendingTrigger = PadButton::NONE;
}

void InputManager::beginPadRebind(GameAction action) {
    rebindingActive      = true;
    rebindingPad         = true;
    rebindTarget         = action;
    rebindPendingTrigger = PadButton::NONE;
}

void InputManager::cancelRebind() {
    rebindingActive      = false;
    rebindPendingTrigger = PadButton::NONE;
}

bool InputManager::isRebinding() const {
    return rebindingActive;
}

bool InputManager::isRebindingGamepad() const {
    return rebindingActive && rebindingPad;
}

GameAction InputManager::getRebindAction() const {
    return rebindTarget;
}

bool InputManager::feedKeyForRebind(int keyCode) {
    if (!rebindingActive || rebindingPad) return false;
    if (keyCode == 27 || keyCode == 13) { // ESC or Enter cancels rebind cleanly
        cancelRebind();
        return true;
    }
    if (keyCode <= 0 || keyCode >= KeyCodes::MAX_KEYS) return false;
    if (keyCode < 256) keyCode = std::tolower(keyCode);
    setKeyBinding(rebindTarget, keyCode);
    saveBindings();
    rebindingActive = false;
    return true;
}

void InputManager::clearJustPressed() {
    for (int i = 0; i < KeyCodes::MAX_KEYS; i++) {
        justPressed[i] = false;
    }
    for (int i = 0; i < (int)PadButton::COUNT; i++) {
        padBtnJustPressed[i] = false;
    }
    for (int i = 0; i < (int)GameAction::COUNT; i++) {
        actionJustPressed[i] = false;
    }
    for (int i = 0; i < (int)GamepadNav::COUNT; i++) {
        gpNavJustPressed[i] = false;
    }
}
