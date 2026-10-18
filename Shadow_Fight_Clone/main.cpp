#include <GL/glut.h>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include "Config.h"
#include "Collision.h"
#include "CombatSystem.h"
#include "InputManager.h"
#include "Renderer.h"
#include "Player.h"
#include "Enemy.h"

int lastTime = 0;
int fpsTimer = 0;
int fpsFrames = 0;

bool showDebugBoxes = true;    // press H to toggle

// GLUT callbacks can't carry objects, so these are shared instances for now.
InputManager input;
Renderer renderer(Config::WORLD_WIDTH, Config::WORLD_HEIGHT);
Player player(100.0f, Config::GROUND_HEIGHT);
Enemy enemy(550.0f, Config::GROUND_HEIGHT);

// Green outline = hurtbox (where it can be hit). Red outline = hitbox (where it hits).
void drawDebugBoxes(const Fighter& fighter) {
    Rect hurtbox = fighter.getHurtbox();
    renderer.drawRectOutline(hurtbox.x, hurtbox.y, hurtbox.width, hurtbox.height,
                             0.2f, 1.0f, 0.2f);

    Rect hitbox;
    if (fighter.getHitbox(hitbox)) {
        renderer.drawRectOutline(hitbox.x, hitbox.y, hitbox.width, hitbox.height,
                                 1.0f, 0.2f, 0.2f);
    }
}

// A simple health bar with a name and numbers. (A proper UI class comes later.)
void drawHealthBar(float x, float y, float width, float height,
                   const Fighter& fighter, const char* name) {
    float fraction = (float)fighter.getHealth() / (float)fighter.getMaxHealth();

    renderer.drawRect(x, y, width, height, 0.35f, 0.05f, 0.05f);           // empty part: dark red
    renderer.drawRect(x, y, width * fraction, height, 0.2f, 0.8f, 0.2f);   // remaining health: green
    renderer.drawRectOutline(x, y, width, height, 1.0f, 1.0f, 1.0f);

    char label[64];
    std::snprintf(label, sizeof(label), "%s  %d / %d",
                  name, fighter.getHealth(), fighter.getMaxHealth());
    renderer.drawText(x, y + height + 6.0f, label, 1.0f, 1.0f, 1.0f);
}

// A thin energy bar. It turns gold when there is enough energy for a special attack.
void drawEnergyBar(float x, float y, float width, float height, const Fighter& fighter) {
    float fraction = fighter.getEnergy() / fighter.getMaxEnergy();
    bool specialReady = fighter.getEnergy() >= Attacks::SPECIAL.energyCost;

    renderer.drawRect(x, y, width, height, 0.05f, 0.10f, 0.25f);   // empty part: dark blue
    if (specialReady) {
        renderer.drawRect(x, y, width * fraction, height, 1.0f, 0.8f, 0.2f);   // gold: special ready
    } else {
        renderer.drawRect(x, y, width * fraction, height, 0.3f, 0.6f, 1.0f);   // blue: still charging
    }
    renderer.drawRectOutline(x, y, width, height, 1.0f, 1.0f, 1.0f);
}

void display() {
    renderer.beginFrame();

    // ground
    renderer.drawRect(0, 0, Config::WORLD_WIDTH, Config::GROUND_HEIGHT,
                      0.30f, 0.20f, 0.12f);

    player.draw(renderer);
    enemy.draw(renderer);

    if (showDebugBoxes) {
        drawDebugBoxes(player);
        drawDebugBoxes(enemy);
    }

    // debug label: what is the player doing right now?
    char label[64];
    std::snprintf(label, sizeof(label), "State: %s", player.getStateName());
    renderer.drawText(10.0f, Config::WORLD_HEIGHT - 25.0f, label, 1.0f, 1.0f, 1.0f);

    // what is the enemy's brain doing, and what is its body doing?
    char enemyLabel[64];
    std::snprintf(enemyLabel, sizeof(enemyLabel), "AI: %s (%s)",
                  enemy.getAIStateName(), enemy.getStateName());
    renderer.drawText(Config::WORLD_WIDTH - 310.0f, Config::WORLD_HEIGHT - 25.0f,
                      enemyLabel, 1.0f, 1.0f, 1.0f);

    // health bars: player on the left, enemy on the right
    drawHealthBar(10.0f, Config::WORLD_HEIGHT - 75.0f, 300.0f, 20.0f, player, "PLAYER");
    drawHealthBar(Config::WORLD_WIDTH - 310.0f, Config::WORLD_HEIGHT - 75.0f, 300.0f, 20.0f,
                  enemy, "ENEMY");

    // energy bars, just under the health bars
    drawEnergyBar(10.0f, Config::WORLD_HEIGHT - 92.0f, 300.0f, 10.0f, player);
    drawEnergyBar(Config::WORLD_WIDTH - 310.0f, Config::WORLD_HEIGHT - 92.0f, 300.0f, 10.0f, enemy);

    // controls reminder, on the ground strip
    renderer.drawText(10.0f, 62.0f, "A/D move  W/Space jump  S crouch  J punch  K kick  L special  I block",
                      1.0f, 1.0f, 1.0f);
    renderer.drawText(10.0f, 36.0f, "H: debug boxes   R: refill health and energy",
                      1.0f, 1.0f, 0.6f);

    renderer.endFrame();
}

void idle() {
    int now = glutGet(GLUT_ELAPSED_TIME);
    int elapsedMs = now - lastTime;
    float deltaTime = elapsedMs / 1000.0f;
    lastTime = now;
    if (deltaTime > 0.25f) deltaTime = 0.25f;

    player.update(input, deltaTime);

    // The enemy's brain decides what to do (approach, attack, wait...).
    enemy.update(player, deltaTime);

    // Did either fighter's attack land on the other?
    CombatSystem::resolveAttack(player, enemy);
    CombatSystem::resolveAttack(enemy, player);

    fpsFrames++;
    fpsTimer += elapsedMs;
    if (fpsTimer >= 1000) {
        char title[64];
        std::snprintf(title, sizeof(title), "Shadow Fight - FPS: %d", fpsFrames);
        glutSetWindowTitle(title);
        fpsFrames = 0;
        fpsTimer = 0;
    }

    glutPostRedisplay();
}

void keyboardDown(unsigned char key, int, int) {
    if (key == 27) std::exit(0);   // ESC
    if (key == 'h' || key == 'H') showDebugBoxes = !showDebugBoxes;
    if (key == 'r' || key == 'R') {            // testing aid: refill everyone's health
        player.restoreHealth();
        enemy.restoreHealth();
        player.restoreEnergy();
        enemy.restoreEnergy();
    }
    input.keyDown(key);
}

void keyboardUp(unsigned char key, int, int) {
    input.keyUp(key);
}

void reshape(int w, int h) {
    renderer.resize(w, h);
}

int main(int argc, char** argv) {
    std::srand((unsigned)std::time(nullptr));   // seed the random numbers the enemy uses

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(Config::WORLD_WIDTH, Config::WORLD_HEIGHT);
    glutCreateWindow("Shadow Fight");

    renderer.init();
    glutIgnoreKeyRepeat(1);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboardDown);
    glutKeyboardUpFunc(keyboardUp);
    glutIdleFunc(idle);

    lastTime = glutGet(GLUT_ELAPSED_TIME);

    glutMainLoop();
    return 0;
}
