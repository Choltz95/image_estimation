#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <GL/glut.h>
#include "render.h"

GLint windW = 300, windH = 300;

GLuint selectBuf[MAXSELECT];
GLfloat feedBuf[MAXFEED];
GLint vp[4];
float zRotation = 90.0;
float zoom = 1.0;
GLint objectCount;
GLint numObjects;
GLenum linePoly = GL_FALSE;

static void InitObjects(GLint num) {
  GLint i;
  float x, y;

  if (num > MAXOBJS) {
    num = MAXOBJS;
  }
  if (num < 1) {
    num = 1;
  }
  objectCount = num;

  srand((unsigned int) time(NULL));
  for (i = 0; i < num; i++) {
    x = (rand() % 300) - 150;
    y = (rand() % 300) - 150;

    objects[i].v1[0] = x + (rand() % 50) - 25;
    objects[i].v2[0] = x + (rand() % 50) - 25;
    objects[i].v3[0] = x + (rand() % 50) - 25;
    objects[i].v1[1] = y + (rand() % 50) - 25;
    objects[i].v2[1] = y + (rand() % 50) - 25;
    objects[i].v3[1] = y + (rand() % 50) - 25;
    objects[i].color[0] = ((rand() % 100) + 50) / 150.0;
    objects[i].color[1] = ((rand() % 100) + 50) / 150.0;
    objects[i].color[2] = ((rand() % 100) + 50) / 150.0;
    objects[i].color[3] = (double)rand() / (double)RAND_MAX; // alpha value
  }
}

void Init(void) {
  numObjects = 50;
  InitObjects(numObjects);
}

void Reshape(int width, int height) {
  windW = width;
  windH = height;
  // multisampling
  glEnable(GL_MULTISAMPLE);
  glutSetOption(GLUT_MULTISAMPLE, 8);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
  // transparency
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glViewport(0, 0, windW, windH);
  glGetIntegerv(GL_VIEWPORT, vp);
}

static void Render(GLenum mode) {
  GLint i;

  for (i = 0; i < objectCount; i++) {
    if (mode == GL_SELECT) {
      glLoadName(i);
    }
    //glColor3fv(objects[i].color);
    glColor4fv(objects[i].color);
    glBegin(GL_POLYGON);
    glVertex2fv(objects[i].v1);
    glVertex2fv(objects[i].v2);
    glVertex2fv(objects[i].v3);
    glEnd();
  }
}

static GLint DoSelect(GLint x, GLint y) {
  GLint hits;

  glSelectBuffer(MAXSELECT, selectBuf);
  glRenderMode(GL_SELECT);
  glInitNames();
  glPushName(~0);

  glPushMatrix();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPickMatrix(x, windH - y, 4, 4, vp);
  gluOrtho2D(-175, 175, -175, 175);
  glMatrixMode(GL_MODELVIEW);

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glScalef(zoom, zoom, zoom);
  glRotatef(zRotation, 0, 0, 1);

  Render(GL_SELECT);

  glPopMatrix();

  hits = glRenderMode(GL_RENDER);
  if (hits <= 0) {
    return -1;
  }
  return selectBuf[(hits - 1) * 4 + 3];
}

static void RecolorTri(GLint h) {
  objects[h].color[0] = ((rand() % 100) + 50) / 150.0;
  objects[h].color[1] = ((rand() % 100) + 50) / 150.0;
  objects[h].color[2] = ((rand() % 100) + 50) / 150.0;
}

static void DeleteTri(GLint h) {
  objects[h] = objects[objectCount - 1];
  objectCount--;
}

static void GrowTri(GLint h) {
  float v[2];
  float *oldV;
  GLint i;

  v[0] = objects[h].v1[0] + objects[h].v2[0] + objects[h].v3[0];
  v[1] = objects[h].v1[1] + objects[h].v2[1] + objects[h].v3[1];
  v[0] /= 3;
  v[1] /= 3;

  for (i = 0; i < 3; i++) {
    switch (i) {
    case 0:
      oldV = objects[h].v1;
      break;
    case 1:
      oldV = objects[h].v2;
      break;
    case 2:
      oldV = objects[h].v3;
      break;
    }
    oldV[0] = 1.5 * (oldV[0] - v[0]) + v[0];
    oldV[1] = 1.5 * (oldV[1] - v[1]) + v[1];
  }
}

void Mouse(int button, int state, int mouseX, int mouseY) {
  GLint hit;

  if (state == GLUT_DOWN) {
    hit = DoSelect((GLint) mouseX, (GLint) mouseY);
    if (hit != -1) {
      if (button == GLUT_LEFT_BUTTON) {
        RecolorTri(hit);
      } else if (button == GLUT_MIDDLE_BUTTON) {
        GrowTri(hit);
      } else if (button == GLUT_RIGHT_BUTTON) {
        DeleteTri(hit);
      }
      glutPostRedisplay();
    }
  }
}

void Draw(void) {
  glPushMatrix();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-175, 175, -175, 175);
  glMatrixMode(GL_MODELVIEW);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glScalef(zoom, zoom, zoom);
  glRotatef(zRotation, 0, 0, 1);
  Render(GL_RENDER);
  glPopMatrix();
  glutSwapBuffers();
}

static void DumpFeedbackVert(GLint * i, GLint n) {
  GLint index;

  index = *i;
  if (index + 7 > n) {
    *i = n;
    printf("  ???\n");
    return;
  }
  printf("  (%g %g %g), color = (%4.2f %4.2f %4.2f)\n",
    feedBuf[index],
    feedBuf[index + 1],
    feedBuf[index + 2],
    feedBuf[index + 3],
    feedBuf[index + 4],
    feedBuf[index + 5]);
  index += 7;
  *i = index;
}

static void DrawFeedback(GLint n) {
  GLint i;
  GLint verts;

  printf("Feedback results (%d floats):\n", n);
  for (i = 0; i < n; i++) {
    switch ((GLint) feedBuf[i]) {
    case GL_POLYGON_TOKEN:
      printf("Polygon");
      i++;
      if (i < n) {
        verts = (GLint) feedBuf[i];
        i++;
        printf(": %d vertices", verts);
      } else {
        verts = 0;
      }
      printf("\n");
      while (verts) {
        DumpFeedbackVert(&i, n);
        verts--;
      }
      i--;
      break;
    case GL_LINE_TOKEN:
      printf("Line:\n");
      i++;
      DumpFeedbackVert(&i, n);
      DumpFeedbackVert(&i, n);
      i--;
      break;
    case GL_LINE_RESET_TOKEN:
      printf("Line Reset:\n");
      i++;
      DumpFeedbackVert(&i, n);
      DumpFeedbackVert(&i, n);
      i--;
      break;
    default:
      printf("%9.2f\n", feedBuf[i]);
      break;
    }
  }
  if (i == MAXFEED) {
    printf("...\n");
  }
  printf("\n");
}

void DoFeedback(void) {
  GLint x;

  glFeedbackBuffer(MAXFEED, GL_3D_COLOR, feedBuf);
  (void) glRenderMode(GL_FEEDBACK);

  glPushMatrix();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(-175, 175, -175, 175);
  glMatrixMode(GL_MODELVIEW);

  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glScalef(zoom, zoom, zoom);
  glRotatef(zRotation, 0, 0, 1);

  Render(GL_FEEDBACK);

  glPopMatrix();

  x = glRenderMode(GL_RENDER);
  if (x == -1) {
    x = MAXFEED;
  }
  DrawFeedback((GLint) x);
}