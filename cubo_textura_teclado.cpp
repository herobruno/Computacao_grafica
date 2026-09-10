// Copyright
// Computação Gráfica
// URI Santiago
// Professor Laurence

#include <GL/glew.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

#include "font.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
const GLfloat kHudHeight = 300;
const GLfloat kHudWidth = 300;

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

const GLfloat kCameraSpeed = 0.1;
const GLfloat kMouseSensitivity = 0.1;

const GLfloat kDefaultCameraEyePositionX = 0;
const GLfloat kDefaultCameraEyePositionY = 0;
const GLfloat kDefaultCameraEyePositionZ = 0;

GLfloat camera_eye_x = kDefaultCameraEyePositionX;
GLfloat camera_eye_y = kDefaultCameraEyePositionY;
GLfloat camera_eye_z = kDefaultCameraEyePositionZ;

const GLfloat kDefaultCameraDirectionX = 0;
const GLfloat kDefaultCameraDirectionY = 0;
const GLfloat kDefaultCameraDirectionZ = -1;

GLfloat camera_direction_x = kDefaultCameraDirectionX;
GLfloat camera_direction_y = kDefaultCameraDirectionY;
GLfloat camera_direction_z = kDefaultCameraDirectionZ;

const GLfloat kCameraUpX = 0;
const GLfloat kCameraUpY = 1;
const GLfloat kCameraUpZ = 0;

GLfloat camera_right_x;
GLfloat camera_right_y;
GLfloat camera_right_z;

const GLfloat kDefaultCameraYaw = -90;
GLfloat camera_yaw = kDefaultCameraYaw;

const GLfloat kCameraPitchLimit = 90;
const GLfloat kDefaultCameraPitch = 0;
GLfloat camera_pitch = kDefaultCameraPitch;

GLuint red_ball_texture_id;
GLuint green_ball_texture_id;
GLuint blue_ball_texture_id;
GLuint brick_texture_id;

void load_texture(GLuint &texture_id, const std::string &filepath)
{
    int texture_width, texture_height, texture_channels;
    uint8_t *texture_data =
        stbi_load(filepath.c_str(), &texture_width, &texture_height, &texture_channels, 0);

    if (!texture_data)
    {
        std::cerr << "Erro ao carregar a textura: " << filepath << std::endl;
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    GLenum texture_format = ((texture_channels == 4) ? GL_RGBA : GL_RGB);
    glTexImage2D(GL_TEXTURE_2D, 0, texture_format, texture_width, texture_height, 0, texture_format,
                 GL_UNSIGNED_BYTE, texture_data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(texture_data);
}

static void mouse_read(GLFWwindow *window, double mouse_x, double mouse_y)
{
    static bool mouse_initialized = false;
    static GLfloat last_mouse_x = 0;
    static GLfloat last_mouse_y = 0;

    if (!mouse_initialized)
    {
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        mouse_initialized = true;
    }

    GLfloat mouse_delta_x = mouse_x - last_mouse_x;
    GLfloat mouse_delta_y = last_mouse_y - mouse_y;

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    camera_pitch += mouse_delta_y * kMouseSensitivity;

    if (camera_pitch > kCameraPitchLimit)
        camera_pitch = kCameraPitchLimit;

    if (camera_pitch < -kCameraPitchLimit)
        camera_pitch = -kCameraPitchLimit;

    camera_yaw += mouse_delta_x * kMouseSensitivity;

    GLfloat camera_pitch_radians = camera_pitch * (3.1415 / 180);
    GLfloat camera_yaw_radians = camera_yaw * (3.1415 / 180);

    camera_direction_x =
        cosf(camera_yaw_radians) * cosf(camera_pitch_radians);

    camera_direction_y =
        sinf(camera_pitch_radians);

    camera_direction_z =
        sinf(camera_yaw_radians) * cosf(camera_pitch_radians);

    camera_right_x =
        camera_direction_z * kCameraUpY -
        camera_direction_y * kCameraUpZ;

    camera_right_y =
        camera_direction_x * kCameraUpZ -
        camera_direction_z * kCameraUpX;

    camera_right_z =
        camera_direction_y * kCameraUpX -
        camera_direction_x * kCameraUpY;
}

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
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera_eye_x += camera_direction_x * kCameraSpeed;
        camera_eye_y += camera_direction_y * kCameraSpeed;
        camera_eye_z += camera_direction_z * kCameraSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera_eye_x -= camera_direction_x * kCameraSpeed;
        camera_eye_y -= camera_direction_y * kCameraSpeed;
        camera_eye_z -= camera_direction_z * kCameraSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera_eye_x += camera_right_x * kCameraSpeed;
        camera_eye_y += camera_right_y * kCameraSpeed;
        camera_eye_z += camera_right_z * kCameraSpeed;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera_eye_x -= camera_right_x * kCameraSpeed;
        camera_eye_y -= camera_right_y * kCameraSpeed;
        camera_eye_z -= camera_right_z * kCameraSpeed;
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

    glColor3ub(255, 255, 255);
    glEnable(GL_TEXTURE_2D);

    if (translating)
    {
        glBindTexture(GL_TEXTURE_2D, red_ball_texture_id);

        glBegin(GL_QUADS);
        {
            glTexCoord2f(0, 0);
            glVertex2f(5, 15);

            glTexCoord2f(1, 0);
            glVertex2f(15, 15);

            glTexCoord2f(1, 1);
            glVertex2f(15, 25);

            glTexCoord2f(0, 1);
            glVertex2f(5, 25);
        }
        glEnd();
    }

    if (rotating)
    {
        glBindTexture(GL_TEXTURE_2D, green_ball_texture_id);

        glBegin(GL_QUADS);
        {
            glTexCoord2f(0, 0);
            glVertex2f(15, 15);

            glTexCoord2f(1, 0);
            glVertex2f(25, 15);

            glTexCoord2f(1, 1);
            glVertex2f(25, 25);

            glTexCoord2f(0, 1);
            glVertex2f(15, 25);
        }
        glEnd();
    }

    if (scaling)
    {
        glBindTexture(GL_TEXTURE_2D, blue_ball_texture_id);

        glBegin(GL_QUADS);
        {
            glTexCoord2f(0, 0);
            glVertex2f(25, 15);

            glTexCoord2f(1, 0);
            glVertex2f(35, 15);

            glTexCoord2f(1, 1);
            glVertex2f(35, 25);

            glTexCoord2f(0, 1);
            glVertex2f(25, 25);
        }
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);

    glEnable(GL_DEPTH_TEST);
}
void draw()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLfloat camera_center_x = camera_eye_x + camera_direction_x;
    GLfloat camera_center_y = camera_eye_y + camera_direction_y;
    GLfloat camera_center_z = camera_eye_z + camera_direction_z;

    gluLookAt(
        camera_eye_x,
        camera_eye_y,
        camera_eye_z,

        camera_center_x,
        camera_center_y,
        camera_center_z,

        kCameraUpX,
        kCameraUpY,
        kCameraUpZ);

    glTranslatef(
        translate_x,
        translate_y,
        translate_z - kPerspectiveTranslateZ);

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

    glColor3ub(255, 255, 255);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, brick_texture_id);

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex3f(-2.0, 2.0, 2.0);

        glTexCoord2f(1, 0);
        glVertex3f(2.0, 2.0, 2.0);

        glTexCoord2f(1, 1);
        glVertex3f(2.0, -2.0, 2.0);

        glTexCoord2f(0, 1);
        glVertex3f(-2.0, -2.0, 2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex3f(-2.0, 2.0, -2.0);

        glTexCoord2f(1, 0);
        glVertex3f(2.0, 2.0, -2.0);

        glTexCoord2f(1, 1);
        glVertex3f(2.0, -2.0, -2.0);

        glTexCoord2f(0, 1);
        glVertex3f(-2.0, -2.0, -2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex3f(-2.0, 2.0, 2.0);

        glTexCoord2f(1, 0);
        glVertex3f(-2.0, 2.0, -2.0);

        glTexCoord2f(1, 1);
        glVertex3f(-2.0, -2.0, -2.0);

        glTexCoord2f(0, 1);
        glVertex3f(-2.0, -2.0, 2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex3f(2.0, 2.0, 2.0);

        glTexCoord2f(1, 0);
        glVertex3f(2.0, 2.0, -2.0);

        glTexCoord2f(1, 1);
        glVertex3f(2.0, -2.0, -2.0);

        glTexCoord2f(0, 1);
        glVertex3f(2.0, -2.0, 2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex3f(-2.0, 2.0, 2.0);

        glTexCoord2f(1, 0);
        glVertex3f(2.0, 2.0, 2.0);

        glTexCoord2f(1, 1);
        glVertex3f(2.0, 2.0, -2.0);

        glTexCoord2f(0, 1);
        glVertex3f(-2.0, 2.0, -2.0);
    }
    glEnd();

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0, 0);
        glVertex3f(-2.0, -2.0, 2.0);

        glTexCoord2f(1, 0);
        glVertex3f(2.0, -2.0, 2.0);

        glTexCoord2f(1, 1);
        glVertex3f(2.0, -2.0, -2.0);

        glTexCoord2f(0, 1);
        glVertex3f(-2.0, -2.0, -2.0);
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_read);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Falha ao inicializar GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    load_texture(red_ball_texture_id, "textures/red_ball.png");
    load_texture(green_ball_texture_id, "textures/green_ball.png");
    load_texture(blue_ball_texture_id, "textures/blue_ball.png");
    load_texture(brick_texture_id, "textures/terra.jpg");

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