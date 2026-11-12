#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

enum class GamepadNav {
    UP = 0,
    DOWN,
    LEFT,
    RIGHT,
    CONFIRM,
    CANCEL,
    START,
    SPECIAL_Y,
    LB,
    RB,
    COUNT
};

enum class GameAction {
    MOVE_LEFT = 0,
    MOVE_RIGHT,
    JUMP,
    CROUCH,
    RUN_HOLD,
    PUNCH,
    KICK,
    DODGE,
    CHAIN_GRAB,
    BLOCK,
    CRITICAL,
    COUNT
};

enum class PadButton {
    NONE = -1,
    DPAD_UP = 0,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,
    STICK_UP,
    STICK_DOWN,
    STICK_LEFT,
    STICK_RIGHT,
    A,
    B,
    X,
    Y,
    LB,
    RB,
    LT,
    RT,
    BACK,
    START,
    COUNT
};

struct GamepadBinding {
    PadButton button;
    PadButton modifier; // PadButton::NONE for single button; trigger/bumper for combo (e.g. RT + X)
};

namespace KeyCodes {
    const int SHIFT       = 256;
    const int CTRL        = 257;
    const int ARROW_UP    = 258;
    const int ARROW_DOWN  = 259;
    const int ARROW_LEFT  = 260;
    const int ARROW_RIGHT = 261;
    const int MAX_KEYS    = 270;
}

// Tracks held, edge-triggered, and remappable keyboard & gamepad controls.
class InputManager {
public:
    InputManager();

    void keyDown(unsigned char key);
    void keyUp(unsigned char key);
    void specialKeyDown(int glutKey);
    void specialKeyUp(int glutKey);
    void pollGamepad();

    bool isKeyDown(unsigned char key) const;
    bool isCodeDown(int keyCode) const;
    bool wasKeyPressed(unsigned char key) const;
    bool wasGamepadNavPressed(GamepadNav nav) const;
    bool isGamepadConnected() const;
    const char* getGamepadBackendName() const;

    // Logical action queries (respect custom keyboard & gamepad bindings)
    bool isActionDown(GameAction action) const;
    bool wasActionPressed(GameAction action) const;

    // Control customization & persistence
    void loadBindings();
    void saveBindings() const;
    void resetDefaultBindings();

    int getKeyBinding(GameAction action) const;
    void setKeyBinding(GameAction action, int keyCode);
    GamepadBinding getPadBinding(GameAction action) const;
    void setPadBinding(GameAction action, GamepadBinding binding);

    static const char* getActionName(GameAction action);
    static const char* getActionDesc(GameAction action);
    static void formatKeyName(int keyCode, char* outBuf, int bufSize);
    static void formatPadBinding(GamepadBinding binding, char* outBuf, int bufSize);

    // Interactive capture helpers for Controls screen
    void beginKeyRebind(GameAction action);
    void beginPadRebind(GameAction action);
    void cancelRebind();
    bool isRebinding() const;
    bool isRebindingGamepad() const;
    GameAction getRebindAction() const;
    bool feedKeyForRebind(int keyCode); // returns true if handled/captured

    void clearJustPressed();

private:
    void setCodeState(int keyCode, bool down, bool isGamepad);
    void setPadBtnState(PadButton btn, bool down);
    void setGamepadNav(GamepadNav nav, bool down);
    bool evalPadBindingDown(int actionIdx) const;
    int  mapGlutSpecial(int glutKey) const;

    bool keys[KeyCodes::MAX_KEYS];
    bool justPressed[KeyCodes::MAX_KEYS];

    bool padBtnDown[(int)PadButton::COUNT];
    bool padBtnJustPressed[(int)PadButton::COUNT];

    bool actionDown[(int)GameAction::COUNT];
    bool actionJustPressed[(int)GameAction::COUNT];

    bool gpNavDown[(int)GamepadNav::COUNT];
    bool gpNavJustPressed[(int)GamepadNav::COUNT];

    int            keyBindings[(int)GameAction::COUNT];
    GamepadBinding padBindings[(int)GameAction::COUNT];

    bool       gamepadConnected;
    const char* gamepadBackend;

    bool       rebindingActive;
    bool       rebindingPad;
    GameAction rebindTarget;
    PadButton  rebindPendingTrigger;
};

#endif
