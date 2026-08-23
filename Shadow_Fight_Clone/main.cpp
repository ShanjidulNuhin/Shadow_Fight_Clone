#include <GL/glut.h>
#include <cstdlib>

const int WIN_W = 800;
const int WIN_H = 600;

float squareX = 100.0f;   // starting position
float squareSpeed = 2.0f; // pixels moved per frame

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glColor3f(1.0f, 1.0f, 1.0f); // white
    glBegin(GL_QUADS);
        glVertex2f(squareX,       280.0f);
        glVertex2f(squareX + 50,  280.0f);
        glVertex2f(squareX + 50,  330.0f);
        glVertex2f(squareX,       330.0f);
    glEnd();

    glutSwapBuffers();
}

void update(int value) {
    squareX += squareSpeed;

    // bounce off the right edge
    if (squareX + 50 > WIN_W) {
        squareX = WIN_W - 50;
        squareSpeed = -squareSpeed;
    }
    // bounce off the left edge
    if (squareX < 0) {
        squareX = 0;
        squareSpeed = -squareSpeed;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) std::exit(0);
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, WIN_W, 0, WIN_H); // left, right, bottom, top
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(WIN_W, WIN_H);
    glutCreateWindow("Shadow Fight - Coordinate Test");

    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(0, update, 0);

    glutMainLoop();
    return 0;
}
