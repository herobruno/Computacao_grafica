// Copyright
// Computação Gráfica
// URI Santiago
// Professor Laurence

#include <GL/glew.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>
#include <cmath>


#include "font.h"

const GLfloat kHudHeight = 300;
const GLfloat kHudWidth = 300;
GLfloat clock_hand_rotate_angle = 0;

std::ostringstream hud_text;

bool perspective_view = true;
const GLfloat kPerspectiveFieldOfViewAngle = 45;
const GLfloat kPerspectiveNearZ = 0.1;
const GLfloat kPerspectiveFarZ = 100;
const GLfloat kPerspectiveTranslateZ = 12;

bool translating = false;
const GLfloat kTranslateLimit = 3;
const GLfloat kDefaultTranslate = 0;
GLfloat translate_increment = 0.1;
GLfloat translate_x = kDefaultTranslate;
GLfloat translate_y = kDefaultTranslate;
GLfloat translate_z = kDefaultTranslate;

bool rotating = false;
const GLfloat kRotateAngleLimit = 180;
const GLfloat kDefaultRotateAngle = 0;
GLfloat rotate_angle_increment = 1;
GLfloat rotate_x = kDefaultRotateAngle;
GLfloat rotate_y = kDefaultRotateAngle;
GLfloat rotate_z = kDefaultRotateAngle;

bool scaling = false;
const GLfloat kScaleLimit = 10;
const GLfloat kDefaultScale = 1;
GLfloat scale_increment = 0.1;
GLfloat scale_x = kDefaultScale;
GLfloat scale_y = kDefaultScale;
GLfloat scale_z = kDefaultScale;

void keyboard_read(GLFWwindow *window)
{

    hud_text.str("");

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        translate_x = kDefaultTranslate;
        translate_y = kDefaultTranslate;
        translate_z = kDefaultTranslate;

        rotate_x = kDefaultRotateAngle;
        rotate_y = kDefaultRotateAngle;
        rotate_z = kDefaultRotateAngle;

        scale_x = kDefaultScale;
        scale_y = kDefaultScale;
        scale_z = kDefaultScale;

        hud_text << "Redefinido!";
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
    {
        translating = true;

        hud_text << "Transladando... ";

        // X
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            translate_x -= translate_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            translate_x += translate_increment;
        }

        // Y
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            translate_y -= translate_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            translate_y += translate_increment;
        }

        // Z
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        {
            translate_z -= translate_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        {
            translate_z += translate_increment;
        }
    }

    if (translate_x > kTranslateLimit)
    {
        translate_x = kTranslateLimit;
    }

    if (translate_x < -kTranslateLimit)
    {
        translate_x = -kTranslateLimit;
    }

    if (translate_y > kTranslateLimit)
    {
        translate_y = kTranslateLimit;
    }

    if (translate_y < -kTranslateLimit)
    {
        translate_y = -kTranslateLimit;
    }

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
    {
        translating = false;
    }
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
    {
        rotating = true;

        hud_text << "Rotacionando... ";

        // X
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            rotate_x -= rotate_angle_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            rotate_x += rotate_angle_increment;
        }

        // Y
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            rotate_y -= rotate_angle_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            rotate_y += rotate_angle_increment;
        }

        // Z
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        {
            rotate_z -= rotate_angle_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        {
            rotate_z += rotate_angle_increment;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_RELEASE)
    {
        rotating = false;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        scaling = true;

        hud_text << "Escalando... ";

        // X
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        {
            scale_x -= scale_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        {
            scale_x += scale_increment;
        }

        // Y
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            scale_y -= scale_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        {
            scale_y += scale_increment;
        }

        // Z
        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        {
            scale_z -= scale_increment;
        }

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        {
            scale_z += scale_increment;
        }
    }

    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_RELEASE)
    {
        scaling = false;
    }
}

void resize_window(GLFWwindow *window)
{
    int window_width, window_height;
    glfwGetFramebufferSize(window, &window_width, &window_height);

    glViewport(0, 0, window_width, window_height);

    GLdouble aspect_ratio =
        (GLdouble)window_width / window_height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glfwSetWindowTitle(window, "Quadrado em perspectiva");

    GLdouble field_of_view = kPerspectiveFieldOfViewAngle;
    GLdouble near = kPerspectiveNearZ;
    GLdouble far = kPerspectiveFarZ;

    gluPerspective(
        field_of_view,
        aspect_ratio,
        near,
        far);
}
void draw_hud(GLFWwindow *window)
{
    int window_width, window_height;
    glfwGetFramebufferSize(window, &window_width, &window_height);

    GLdouble aspect_ratio =
        (GLdouble)window_width / window_height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    GLdouble left = 0;
    GLdouble right = kHudWidth;
    GLdouble bottom = 0;
    GLdouble top = kHudHeight;

    if (window_width > window_height)
    {
        gluOrtho2D(
            (left * aspect_ratio),
            (right * aspect_ratio),
            bottom,
            top);
    }
    else
    {
        gluOrtho2D(
            left,
            right,
            (bottom / aspect_ratio),
            (top / aspect_ratio));
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);

    glColor3ub(255, 255, 255);

    draw_text(
        5,
        5,
        hud_text.str());

    glPointSize(20);

    glBegin(GL_POINTS);
    {
        if (translating)
        {
            glColor3ub(255, 0, 0);
            glVertex2f(7.5, 20);
        }

        if (rotating)
        {
            glColor3ub(0, 255, 0);
            glVertex2f(17.5, 20);
        }

        if (scaling)
        {
            glColor3ub(0, 0, 255);
            glVertex2f(27.5, 20);
        }
    }
    glEnd();

    glEnable(GL_DEPTH_TEST);
}

void draw()
{

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glTranslatef(
        translate_x,
        translate_y,
        translate_z - kPerspectiveTranslateZ);

    glRotatef(rotate_x, 1, 0, 0);
    glRotatef(rotate_y, 0, 1, 0);
    glRotatef(rotate_z, 0, 0, 1);

    glScalef(
        scale_x,
        scale_y,
        scale_z);


    glBegin(GL_QUADS);
    {
        glColor3ub(255, 0, 0);
        glVertex3f(-2.0, 2.0, 2.0);

        glColor3ub(0, 255, 0);
        glVertex3f(2.0, 2.0, 2.0);

        glColor3ub(0, 0, 255);
        glVertex3f(2.0, -2.0, 2.0);

        glColor3ub(255, 255, 0);
        glVertex3f(-2.0, -2.0, 2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glColor3ub(255, 0, 255);
        glVertex3f(-2.0, 2.0, -2.0);

        glColor3ub(0, 255, 255);
        glVertex3f(2.0, 2.0, -2.0);

        glColor3ub(255, 255, 255);
        glVertex3f(2.0, -2.0, -2.0);

        glColor3ub(100, 100, 100);
        glVertex3f(-2.0, -2.0, -2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glColor3ub(200, 0, 0);
        glVertex3f(-2.0, 2.0, 2.0);
        glVertex3f(-2.0, 2.0, -2.0);
        glVertex3f(-2.0, -2.0, -2.0);
        glVertex3f(-2.0, -2.0, 2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glColor3ub(0, 200, 0);
        glVertex3f(2.0, 2.0, 2.0);
        glVertex3f(2.0, 2.0, -2.0);
        glVertex3f(2.0, -2.0, -2.0);
        glVertex3f(2.0, -2.0, 2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glColor3ub(0, 0, 200);
        glVertex3f(-2.0, 2.0, 2.0);
        glVertex3f(2.0, 2.0, 2.0);
        glVertex3f(2.0, 2.0, -2.0);
        glVertex3f(-2.0, 2.0, -2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glColor3ub(200, 200, 0);
        glVertex3f(-2.0, -2.0, 2.0);
        glVertex3f(2.0, -2.0, 2.0);
        glVertex3f(2.0, -2.0, -2.0);
        glVertex3f(-2.0, -2.0, -2.0);
    }
    glEnd();


    // RELÓGIO 1 - FRENTE

    glPushMatrix();
    {
        glTranslatef(0, 0, 2.05);

        glLineWidth(5);
        glColor3ub(255, 0, 255);

        glRotatef(clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.25, 0, 0.05);
        }
        glEnd();

        glRotatef(2 * clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.5, 0, 0.05);
        }
        glEnd();
    }
    glPopMatrix();


    // RELÓGIO 2 - TRÁS

    glPushMatrix();
    {
        glTranslatef(0, 0, -2.05);
        glRotatef(180, 0, 1, 0);

        glLineWidth(5);
        glColor3ub(255, 0, 255);

        glRotatef(clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.25, 0, 0.05);
        }
        glEnd();

        glRotatef(2 * clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.5, 0, 0.05);
        }
        glEnd();
    }
    glPopMatrix();


    // RELÓGIO 3 - DIREITA

    glPushMatrix();
    {
        glTranslatef(2.05, 0, 0);
        glRotatef(90, 0, 1, 0);

        glLineWidth(5);
        glColor3ub(255, 0, 255);

        glRotatef(clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.25, 0, 0.05);
        }
        glEnd();

        glRotatef(2 * clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.5, 0, 0.05);
        }
        glEnd();
    }
    glPopMatrix();


    // RELÓGIO 4 - ESQUERDA

    glPushMatrix();
    {
        glTranslatef(-2.05, 0, 0);
        glRotatef(-90, 0, 1, 0);

        glLineWidth(5);
        glColor3ub(255, 0, 255);

        glRotatef(clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.25, 0, 0.05);
        }
        glEnd();

        glRotatef(2 * clock_hand_rotate_angle, 0, 0, 1);

        glBegin(GL_LINES);
        {
            glVertex3f(0, 0, 0.05);
            glVertex3f(0.5, 0, 0.05);
        }
        glEnd();
    }
    glPopMatrix();

   clock_hand_rotate_angle -= 0.1;
}

int main()
{
    if (!glfwInit())
    {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return EXIT_FAILURE;
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);

    GLFWwindow *window = glfwCreateWindow(
        mode->width,
        mode->height,
        "Quadrado em perspectiva",
        monitor,
        NULL);
    if (!window)
    {
        std::cerr << "Falha ao criar a janela GLFW" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwSetWindowPos(window, 0, 0);
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Falha ao inicializar GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);

    glClearColor(
        0.08f,
        0.08f,
        0.08f,
        1.0f);

    while (!glfwWindowShouldClose(window))
    {

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        keyboard_read(window);
        resize_window(window);
        draw();
        draw_hud(window);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}