#include "InputManager.h"
#include <cctype>

InputManager::InputManager() {
    for (int i = 0; i < 256; i++) {
        keys[i] = false;
        justPressed[i] = false;
    }
}

void InputManager::keyDown(unsigned char key) {
    unsigned char k = (unsigned char)std::tolower(key);
    if (!keys[k]) {
        justPressed[k] = true;
    }
    keys[k] = true;
}

void InputManager::keyUp(unsigned char key) {
    keys[(unsigned char)std::tolower(key)] = false;
}

bool InputManager::isKeyDown(unsigned char key) const {
    return keys[(unsigned char)std::tolower(key)];
}

bool InputManager::wasKeyPressed(unsigned char key) const {
    return justPressed[(unsigned char)std::tolower(key)];
}

void InputManager::clearJustPressed() {
    for (int i = 0; i < 256; i++) {
        justPressed[i] = false;
    }
}
