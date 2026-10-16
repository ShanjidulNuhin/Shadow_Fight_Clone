#include "InputManager.h"
#include <cctype>

InputManager::InputManager() {
    for (int i = 0; i < 256; i++) {
        keys[i] = false;
    }
}

void InputManager::keyDown(unsigned char key) {
    keys[std::tolower(key)] = true;
}

void InputManager::keyUp(unsigned char key) {
    keys[std::tolower(key)] = false;
}

bool InputManager::isKeyDown(unsigned char key) const {
    return keys[std::tolower(key)];
}
