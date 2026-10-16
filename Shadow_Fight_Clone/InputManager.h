#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

// Remembers which keyboard keys are currently held down.
class InputManager {
public:
    InputManager();

    void keyDown(unsigned char key);   // call when a key is pressed
    void keyUp(unsigned char key);     // call when a key is released
    bool isKeyDown(unsigned char key) const;

private:
    bool keys[256];
};

#endif
