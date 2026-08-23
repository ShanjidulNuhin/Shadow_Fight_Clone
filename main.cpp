/* ============================================================
   SHADOW FIGHT CLONE  -  OpenGL (GLUT/freeglut) + C++
   ------------------------------------------------------------
   A 2D silhouette fighting game skeleton: two fighters drawn
   as black/grey silhouettes (just like Shadow Fight's art
   style) made purely out of OpenGL primitives -- no image
   files needed to get started.

   Player 1 (black)  : A / D move, W jump, F punch, G kick, S block
   Player 2 (grey AI): moves and attacks automatically
   R                 : restart after a KO
   ESC               : quit
   ============================================================ */

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

// ---------------------------------------------------------------
// Window / arena constants
// ---------------------------------------------------------------
const int   WIN_W    = 800;
const int   WIN_H    = 500;
const float GROUND_Y = 100.0f;
const float MOVE_SPEED   = 4.0f;
const float GRAVITY      = -0.6f;
const float JUMP_FORCE   = 13.0f;
const float ATTACK_RANGE = 70.0f;
const int   MAX_HEALTH   = 100;

enum FighterState { IDLE, WALK, PUNCH, KICK, BLOCK, HIT, JUMP_S };

struct Fighter {
    float x, y;
    float vx, vy;
    int   facing;        // 1 = right, -1 = left
    int   health;
    FighterState state;
    int   stateTimer;    // frames remaining in current action
    bool  onGround;
    bool  hitRegistered;  // prevents one swing from hitting many times
    float r, g, b;        // silhouette tint
};

Fighter p1, p2;
bool  keys[256] = { false };
bool  gameOver  = false;
int   winner    = 0;
int   aiCooldown = 0;

// ---------------------------------------------------------------
// Setup helpers
// ---------------------------------------------------------------
void initFighter(Fighter &f, float startX, int face, float r, float g, float b) {
    f.x = startX; f.y = GROUND_Y;
    f.vx = 0; f.vy = 0;
    f.facing = face;
    f.health = MAX_HEALTH;
    f.state = IDLE;
    f.stateTimer = 0;
    f.onGround = true;
    f.hitRegistered = false;
    f.r = r; f.g = g; f.b = b;
}

void resetGame() {
    initFighter(p1, 220, 1, 0.05f, 0.05f, 0.05f);
    initFighter(p2, 580, -1, 0.25f, 0.25f, 0.30f);
    gameOver = false;
    aiCooldown = 30;
}

// ---------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------
void drawCircle(float cx, float cy, float radius, int segments = 20) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float theta = 2.0f * 3.1415926f * i / segments;
        glVertex2f(cx + radius * cosf(theta), cy + radius * sinf(theta));
    }
    glEnd();
}

void drawText(float x, float y, void *font, const char *text) {
    glRasterPos2f(x, y);
    for (const char *c = text; *c != '\0'; c++)
        glutBitmapCharacter(font, *c);
}

void drawHealthBar(float x, float y, float w, float h, int health) {
    // backing plate
    glColor3f(0.25f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();

    // current health (green -> red as it drops)
    float pct = (float)health / MAX_HEALTH;
    if (pct < 0) pct = 0;
    glColor3f(1.0f - pct, pct, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w * pct, y);
        glVertex2f(x + w * pct, y + h); glVertex2f(x, y + h);
    glEnd();

    // border
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);
        glVertex2f(x, y); glVertex2f(x + w, y);
        glVertex2f(x + w, y + h); glVertex2f(x, y + h);
    glEnd();
}

void drawFighter(const Fighter &f) {
    glColor3f(f.r, f.g, f.b);
    float dir = (float)f.facing;

    float bodyW = 30, bodyH = 50;
    float legSwing = (f.state == WALK) ? 8.0f * sinf(f.stateTimer * 0.6f) : 0.0f;

    // torso
    glBegin(GL_QUADS);
        glVertex2f(f.x - bodyW / 2, f.y + 40);
        glVertex2f(f.x + bodyW / 2, f.y + 40);
        glVertex2f(f.x + bodyW / 2, f.y + 40 + bodyH);
        glVertex2f(f.x - bodyW / 2, f.y + 40 + bodyH);
    glEnd();

    // head
    drawCircle(f.x, f.y + 105, 15);

    // legs
    glBegin(GL_QUADS);
        // left leg
        glVertex2f(f.x - 12, f.y);
        glVertex2f(f.x - 2, f.y);
        glVertex2f(f.x - 2 + legSwing, f.y + 40);
        glVertex2f(f.x - 12 + legSwing, f.y + 40);
        // right leg
        glVertex2f(f.x + 2, f.y);
        glVertex2f(f.x + 12, f.y);
        glVertex2f(f.x + 12 - legSwing, f.y + 40);
        glVertex2f(f.x + 2 - legSwing, f.y + 40);
    glEnd();

    // guard arm (shrinks a little when blocking, just a visual cue)
    float guard = (f.state == BLOCK) ? 4.0f : 10.0f;
    glBegin(GL_QUADS);
        glVertex2f(f.x - dir * (bodyW / 2), f.y + 70);
        glVertex2f(f.x - dir * (bodyW / 2 + guard), f.y + 70);
        glVertex2f(f.x - dir * (bodyW / 2 + guard), f.y + 80);
        glVertex2f(f.x - dir * (bodyW / 2), f.y + 80);
    glEnd();

    // punching arm
    float armExtend = (f.state == PUNCH) ? 35.0f : 10.0f;
    glBegin(GL_QUADS);
        glVertex2f(f.x + dir * (bodyW / 2), f.y + 70);
        glVertex2f(f.x + dir * (bodyW / 2 + armExtend), f.y + 70);
        glVertex2f(f.x + dir * (bodyW / 2 + armExtend), f.y + 80);
        glVertex2f(f.x + dir * (bodyW / 2), f.y + 80);
    glEnd();

    // kicking leg
    if (f.state == KICK) {
        glBegin(GL_QUADS);
            glVertex2f(f.x + dir * 5, f.y + 8);
            glVertex2f(f.x + dir * 50, f.y + 8);
            glVertex2f(f.x + dir * 50, f.y + 22);
            glVertex2f(f.x + dir * 5, f.y + 22);
        glEnd();
    }
}

// ---------------------------------------------------------------
// Game logic
// ---------------------------------------------------------------
void handlePlayerInput() {
    if (gameOver) return;
    if (p1.state == IDLE || p1.state == WALK) {
        if (keys['a'] || keys['A']) { p1.vx = -MOVE_SPEED; p1.facing = -1; p1.state = WALK; }
        else if (keys['d'] || keys['D']) { p1.vx = MOVE_SPEED; p1.facing = 1; p1.state = WALK; }
        else { p1.vx = 0; p1.state = IDLE; }

        if (keys['s'] || keys['S']) p1.state = BLOCK;
    }
}

void updateAI() {
    if (gameOver) return;
    float dx = p1.x - p2.x;
    float adist = fabsf(dx);
    p2.facing = (dx < 0) ? -1 : 1;

    if (p2.state == IDLE || p2.state == WALK) {
        if (adist > ATTACK_RANGE + 15) {
            p2.vx = (dx < 0) ? -MOVE_SPEED * 0.6f : MOVE_SPEED * 0.6f;
            p2.state = WALK;
        } else {
            p2.vx = 0;
            if (aiCooldown <= 0) {
                p2.state = (rand() % 2 == 0) ? PUNCH : KICK;
                p2.stateTimer = 20;
                p2.hitRegistered = false;
                aiCooldown = 45 + rand() % 30;
            } else {
                p2.state = IDLE;
            }
        }
    }
    if (aiCooldown > 0) aiCooldown--;
}

void updateFighter(Fighter &f) {
    f.x += f.vx;
    if (!f.onGround) {
        f.vy += GRAVITY;
        f.y += f.vy;
        if (f.y <= GROUND_Y) {
            f.y = GROUND_Y; f.vy = 0; f.onGround = true;
            if (f.state == JUMP_S) f.state = IDLE;
        }
    }
    if (f.x < 40) f.x = 40;
    if (f.x > WIN_W - 40) f.x = WIN_W - 40;

    if (f.stateTimer > 0) {
        f.stateTimer--;
    } else if (f.state == PUNCH || f.state == KICK || f.state == HIT) {
        f.state = IDLE;
    }
}

void checkAttack(Fighter &attacker, Fighter &defender) {
    if ((attacker.state == PUNCH || attacker.state == KICK) && !attacker.hitRegistered) {
        // "active" window of the swing -- roughly the middle of the animation
        if (attacker.stateTimer <= 16 && attacker.stateTimer >= 8) {
            float dist = fabsf(defender.x - attacker.x);
            bool facingRight = (attacker.facing == 1 && defender.x > attacker.x);
            bool facingLeft  = (attacker.facing == -1 && defender.x < attacker.x);
            if (dist < ATTACK_RANGE && (facingRight || facingLeft)) {
                int dmg = (attacker.state == PUNCH) ? 8 : 14;
                if (defender.state == BLOCK) dmg = dmg / 4; // blocking reduces damage
                defender.health -= dmg;
                if (defender.health < 0) defender.health = 0;
                if (defender.state != BLOCK) {
                    defender.state = HIT;
                    defender.stateTimer = 15;
                }
                attacker.hitRegistered = true;
            }
        }
    }
    if (attacker.state == IDLE || attacker.state == WALK) attacker.hitRegistered = false;
}

// ---------------------------------------------------------------
// GLUT callbacks
// ---------------------------------------------------------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    // sky gradient
    glBegin(GL_QUADS);
        glColor3f(0.55f, 0.75f, 0.95f); glVertex2f(0, WIN_H);       glVertex2f(WIN_W, WIN_H);
        glColor3f(0.85f, 0.80f, 0.70f); glVertex2f(WIN_W, GROUND_Y); glVertex2f(0, GROUND_Y);
    glEnd();

    // ground
    glColor3f(0.30f, 0.20f, 0.12f);
    glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(WIN_W, 0);
        glVertex2f(WIN_W, GROUND_Y); glVertex2f(0, GROUND_Y);
    glEnd();

    drawFighter(p1);
    drawFighter(p2);

    drawHealthBar(30, WIN_H - 40, 300, 20, p1.health);
    drawHealthBar(WIN_W - 330, WIN_H - 40, 300, 20, p2.health);

    glColor3f(0, 0, 0);
    drawText(WIN_W / 2 - 8, WIN_H - 34, GLUT_BITMAP_HELVETICA_18, "VS");

    if (gameOver) {
        glColor3f(0.85f, 0.1f, 0.1f);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "PLAYER %d WINS!  Press R to restart", winner);
        drawText(WIN_W / 2 - 160, WIN_H / 2, GLUT_BITMAP_HELVETICA_18, buf);
    }

    glutSwapBuffers();
}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H);
    glMatrixMode(GL_MODELVIEW);
}

void keyDown(unsigned char key, int, int) {
    keys[key] = true;

    if (key == 27) std::exit(0); // ESC

    if (gameOver) {
        if (key == 'r' || key == 'R') resetGame();
        return;
    }

    if ((key == 'f' || key == 'F') && (p1.state == IDLE || p1.state == WALK)) {
        p1.state = PUNCH; p1.stateTimer = 20; p1.hitRegistered = false;
    }
    if ((key == 'g' || key == 'G') && (p1.state == IDLE || p1.state == WALK)) {
        p1.state = KICK; p1.stateTimer = 20; p1.hitRegistered = false;
    }
    if ((key == 'w' || key == 'W') && p1.onGround) {
        p1.vy = JUMP_FORCE; p1.onGround = false; p1.state = JUMP_S;
    }
}

void keyUp(unsigned char key, int, int) {
    keys[key] = false;
}

void timerFunc(int) {
    handlePlayerInput();
    updateAI();
    updateFighter(p1);
    updateFighter(p2);
    checkAttack(p1, p2);
    checkAttack(p2, p1);

    if (!gameOver) {
        if (p1.health <= 0) { gameOver = true; winner = 2; }
        if (p2.health <= 0) { gameOver = true; winner = 1; }
    }

    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0); // ~60 FPS
}

// ---------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------
int main(int argc, char **argv) {
    std::srand((unsigned)std::time(nullptr));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Shadow Fight Clone - OpenGL/GLUT");

    glClearColor(1, 1, 1, 1);
    resetGame();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutTimerFunc(16, timerFunc, 0);

    glutMainLoop();
    return 0;
}
