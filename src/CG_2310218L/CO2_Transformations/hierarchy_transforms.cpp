// Moving objects relative to their parent with push, translate, rotate and pop.

#include <GL/gl.h>

#include <cmath>

// one block of the given size, centred on the origin of the current matrix
static void block(float w, float h, float d) {
    glPushMatrix();
    glScalef(w, h, d);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glNormal3f(0, 0, -1);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glNormal3f(0, 1, 0);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glNormal3f(0, -1, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glNormal3f(1, 0, 0);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glNormal3f(-1, 0, 0);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glEnd();
    glPopMatrix();
}

// a robot arm: the forearm hangs off the upper arm and the hand off the forearm,
// so turning the shoulder moves all three and turning the elbow moves the forearm and the hand
void drawArm(float shoulderDegrees, float elbowDegrees, float wristDegrees) {
    glPushMatrix();
    glRotatef(shoulderDegrees, 0, 0, 1);            // everything below turns with the shoulder
    glTranslatef(1.0f, 0, 0);                        // go to the middle of the upper arm
    block(2.0f, 0.4f, 0.4f);
    glTranslatef(1.0f, 0, 0);                        // go to the elbow, the end of the upper arm
    glRotatef(elbowDegrees, 0, 0, 1);
    glTranslatef(0.8f, 0, 0);
    block(1.6f, 0.3f, 0.3f);
    glTranslatef(0.8f, 0, 0);                        // go to the wrist
    glRotatef(wristDegrees, 0, 0, 1);
    glTranslatef(0.3f, 0, 0);
    block(0.6f, 0.5f, 0.5f);
    glPopMatrix();                                   // the matrix is back to what it was before the arm
}

// a row of identical chips: each one is placed with its own translate and rotate, and pop undoes it for the next one
void drawChipRow(int count, float spacing, float spinDegrees) {
    for (int i = 0; i < count; ++i) {
        glPushMatrix();
        glTranslatef(static_cast<float>(i) * spacing, 0, 0);
        glRotatef(spinDegrees + static_cast<float>(i) * 15.0f, 0, 1, 0);
        block(0.8f, 0.2f, 0.8f);
        glPopMatrix();
    }
}

// a ring of lights around a point: rotate by a fixed step, then move out along the x axis, once per light
void drawRingOfBlocks(int count, float radius, float spinDegrees) {
    for (int i = 0; i < count; ++i) {
        glPushMatrix();
        glRotatef(spinDegrees + 360.0f * static_cast<float>(i) / static_cast<float>(count), 0, 1, 0);
        glTranslatef(radius, 0, 0);
        block(0.3f, 0.3f, 0.3f);
        glPopMatrix();
    }
}

// the model view matrix is read back as 16 numbers (column by column), the product of everything pushed so far
void readCurrentMatrix(float out[16]) {
    glGetFloatv(GL_MODELVIEW_MATRIX, out);
}
