// Copyright
// Computação Gráfica
// URI Santiago
// Professor Laurence

#define STB_IMAGE_IMPLEMENTATION

#include <GL/glew.h>
#include <GL/glu.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "glb_model.h"

const GLfloat kCameraSpeed = 0.1f;
const GLfloat kMouseSensitivity = 0.1f;

const GLfloat kDefaultCameraEyePositionX = 0.0f;
const GLfloat kDefaultCameraEyePositionY = 0.5f;
const GLfloat kDefaultCameraEyePositionZ = -7.5f;

GLfloat camera_eye_x = kDefaultCameraEyePositionX;
GLfloat camera_eye_y = kDefaultCameraEyePositionY;
GLfloat camera_eye_z = kDefaultCameraEyePositionZ;

const GLfloat kDefaultCameraDirectionX = 0.0f;
const GLfloat kDefaultCameraDirectionY = 0.0f;
const GLfloat kDefaultCameraDirectionZ = -1.0f;

GLfloat camera_direction_x = kDefaultCameraDirectionX;
GLfloat camera_direction_y = kDefaultCameraDirectionY;
GLfloat camera_direction_z = kDefaultCameraDirectionZ;

const GLfloat kCameraUpX = 0.0f;
const GLfloat kCameraUpY = 1.0f;
const GLfloat kCameraUpZ = 0.0f;

GLfloat camera_right_x = 1.0f;
GLfloat camera_right_y = 0.0f;
GLfloat camera_right_z = 0.0f;

const GLfloat kDefaultCameraYaw = -90.0f;
GLfloat camera_yaw = kDefaultCameraYaw;

const GLfloat kCameraPitchLimit = 89.0f;
const GLfloat kDefaultCameraPitch = 0.0f;
GLfloat camera_pitch = kDefaultCameraPitch;

const GLfloat kPerspectiveFieldOfViewAngle = 45.0f;
const GLfloat kPerspectiveNearZ = 0.1f;
const GLfloat kPerspectiveFarZ = 100.0f;
const GLfloat kPerspectiveTranslateZ = 10.0f;

GLuint water_texture_id = 0;

GLuint boat_model_display_list_id = 0;
GLuint port_model_display_list_id = 0;
GLuint moon_model_display_list_id = 0;

GlbModel boat_model;
GlbModel port_model;
GlbModel moon_model;

const GLfloat kMoonAmbient[] = {
    0.38f, 0.40f, 0.52f, 1.0f};

const GLfloat kMoonDiffuse[] = {
    1.25f, 1.25f, 1.45f, 1.0f};

const GLfloat kMoonSpecular[] = {
    1.20f, 1.20f, 1.35f, 1.0f};

const GLfloat kMoonEmission[] = {
    1.0f, 1.0f, 1.0f, 1.0f};

const GLfloat kMoonConstantAttenuation = 0.25f;
const GLfloat kMoonLinearAttenuation = 0.008f;
const GLfloat kMoonQuadraticAttenuation = 0.0f;

// ============================================================
// MOVIMENTO AUTOMATICO DA LUA
// ============================================================

const GLfloat kMoonOrbitRadiusX = 10.0f;
const GLfloat kMoonOrbitRadiusZ = 10.0f;

const GLfloat kMoonOrbitCenterX = 0.0f;
const GLfloat kMoonOrbitCenterZ = 0.0f;

const GLfloat kMoonOrbitHeight = 8.0f;

const GLfloat kMoonOrbitSpeed = 0.3f;

GLfloat moon_orbit_angle = 0.0f;

const GLfloat kMoonScale = 1.5f;

// ============================================================
// POSICAO DA LUA
// ============================================================

// A posicao e atualizada automaticamente em draw().
GLfloat moon_position[] = {
    kMoonOrbitCenterX + kMoonOrbitRadiusX,
    kMoonOrbitHeight,
    kMoonOrbitCenterZ,
    1.0f};

const GLfloat kMoonTranslateIncrement = 0.4f;
const GLfloat kDefaultMoonPositionX = -6.0f;
const GLfloat kDefaultMoonPositionY = 8.0f;
const GLfloat kDefaultMoonPositionZ = -18.0f;
const GLfloat kMinimumMoonPositionY = 1.0f;

// ============================================================
// PORTO E CEU
// ============================================================

const GLfloat kPortX = 4.0f;
const GLfloat kPortY = 1.0f;
const GLfloat kPortZ = 0.0f;
const GLfloat kPortScale = 2.0f;

const GLfloat kSkyColorNight[] = {
    0.015f, 0.025f, 0.09f, 1.0f};

void create_model_display_list(
    GLuint &model_display_list_id,
    const GlbModel &model)
{
    model_display_list_id = glGenLists(1);

    glNewList(model_display_list_id, GL_COMPILE);
    draw_model(model);
    glEndList();
}

void load_water_texture(
    GLuint &texture_id,
    const std::string &filepath)
{
    int texture_width = 0;
    int texture_height = 0;
    int texture_channels = 0;

    stbi_uc *texture_data = stbi_load(
        filepath.c_str(),
        &texture_width,
        &texture_height,
        &texture_channels,
        0);

    if (!texture_data)
    {
        std::cerr << "Erro ao carregar a textura: "
                  << filepath << std::endl;

        std::cerr << stbi_failure_reason() << std::endl;

        std::exit(EXIT_FAILURE);
    }

    GLenum texture_format = GL_RGB;

    switch (texture_channels)
    {
    case 1:
        texture_format = GL_RED;
        break;

    case 2:
        texture_format = GL_RG;
        break;

    case 3:
        texture_format = GL_RGB;
        break;

    case 4:
        texture_format = GL_RGBA;
        break;

    default:
        std::cerr << "Formato de textura nao suportado."
                  << std::endl;

        stbi_image_free(texture_data);
        std::exit(EXIT_FAILURE);
    }

    const int bytes_per_pixel = texture_channels;
    const int row_size = texture_width * bytes_per_pixel;

    std::vector<stbi_uc> row_buffer(row_size);

    for (int y = 0; y < texture_height / 2; ++y)
    {
        stbi_uc *top_row =
            texture_data + y * row_size;

        stbi_uc *bottom_row =
            texture_data +
            (texture_height - 1 - y) * row_size;

        std::copy(
            top_row,
            top_row + row_size,
            row_buffer.begin());

        std::copy(
            bottom_row,
            bottom_row + row_size,
            top_row);

        std::copy(
            row_buffer.begin(),
            row_buffer.end(),
            bottom_row);
    }

    GLint previous_binding = 0;
    GLint previous_unpack_alignment = 4;

    glGetIntegerv(
        GL_TEXTURE_BINDING_2D,
        &previous_binding);

    glGetIntegerv(
        GL_UNPACK_ALIGNMENT,
        &previous_unpack_alignment);

    glGenTextures(1, &texture_id);

    glBindTexture(GL_TEXTURE_2D, texture_id);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        texture_format,
        texture_width,
        texture_height,
        0,
        texture_format,
        GL_UNSIGNED_BYTE,
        texture_data);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_S,
        GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_WRAP_T,
        GL_REPEAT);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR);

    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_2D);

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        previous_unpack_alignment);

    glBindTexture(
        GL_TEXTURE_2D,
        previous_binding);

    stbi_image_free(texture_data);
}

static void mouse_read(
    GLFWwindow *window,
    double mouse_x,
    double mouse_y)
{
    (void)window;

    static bool mouse_initialized = false;
    static double last_mouse_x = 0.0;
    static double last_mouse_y = 0.0;

    if (!mouse_initialized)
    {
        last_mouse_x = mouse_x;
        last_mouse_y = mouse_y;
        mouse_initialized = true;
        return;
    }

    GLfloat mouse_delta_x =
        static_cast<GLfloat>(mouse_x - last_mouse_x);

    GLfloat mouse_delta_y =
        static_cast<GLfloat>(last_mouse_y - mouse_y);

    last_mouse_x = mouse_x;
    last_mouse_y = mouse_y;

    camera_pitch += mouse_delta_y * kMouseSensitivity;

    if (camera_pitch > kCameraPitchLimit)
    {
        camera_pitch = kCameraPitchLimit;
    }

    if (camera_pitch < -kCameraPitchLimit)
    {
        camera_pitch = -kCameraPitchLimit;
    }

    camera_yaw += mouse_delta_x * kMouseSensitivity;

    const GLfloat radians = 3.14159265f / 180.0f;

    const GLfloat pitch_radians =
        camera_pitch * radians;

    const GLfloat yaw_radians =
        camera_yaw * radians;

    camera_direction_x =
        cosf(yaw_radians) * cosf(pitch_radians);

    camera_direction_y =
        sinf(pitch_radians);

    camera_direction_z =
        sinf(yaw_radians) * cosf(pitch_radians);

    camera_right_x =
        camera_direction_z * kCameraUpY -
        camera_direction_y * kCameraUpZ;

    camera_right_y =
        camera_direction_x * kCameraUpZ -
        camera_direction_z * kCameraUpX;

    camera_right_z =
        camera_direction_y * kCameraUpX -
        camera_direction_x * kCameraUpY;

    const GLfloat right_length = sqrtf(
        camera_right_x * camera_right_x +
        camera_right_y * camera_right_y +
        camera_right_z * camera_right_z);

    if (right_length > 0.0f)
    {
        camera_right_x /= right_length;
        camera_right_y /= right_length;
        camera_right_z /= right_length;
    }
}

void keyboard_read(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // W: mover a camera para frente.
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camera_eye_x += camera_direction_x * kCameraSpeed;
        camera_eye_y += camera_direction_y * kCameraSpeed;
        camera_eye_z += camera_direction_z * kCameraSpeed;
    }

    // S: mover a camera para tras.
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camera_eye_x -= camera_direction_x * kCameraSpeed;
        camera_eye_y -= camera_direction_y * kCameraSpeed;
        camera_eye_z -= camera_direction_z * kCameraSpeed;
    }

    // A: mover para a esquerda.
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camera_eye_x += camera_right_x * kCameraSpeed;
        camera_eye_y += camera_right_y * kCameraSpeed;
        camera_eye_z += camera_right_z * kCameraSpeed;
    }

    // D: mover para a direita.
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camera_eye_x -= camera_right_x * kCameraSpeed;
        camera_eye_y -= camera_right_y * kCameraSpeed;
        camera_eye_z -= camera_right_z * kCameraSpeed;
    }

    // Espaco: restaurar a posicao da camera.
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        camera_eye_x = kDefaultCameraEyePositionX;
        camera_eye_y = kDefaultCameraEyePositionY;
        camera_eye_z = kDefaultCameraEyePositionZ;

        camera_direction_x = kDefaultCameraDirectionX;
        camera_direction_y = kDefaultCameraDirectionY;
        camera_direction_z = kDefaultCameraDirectionZ;

        camera_yaw = kDefaultCameraYaw;
        camera_pitch = kDefaultCameraPitch;

        camera_right_x = 1.0f;
        camera_right_y = 0.0f;
        camera_right_z = 0.0f;
    }

    /*
    ============================================================
    CONTROLE MANUAL DA LUA - TEMPORARIAMENTE DESATIVADO
    ============================================================

    Para reativar o controle manual, remova os marcadores
    de comentario deste bloco.

    // Segure L e use as setas/Page Up/Page Down para mover a Lua.
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            moon_position[0] -= kMoonTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            moon_position[0] += kMoonTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        {
            if (moon_position[1] > kMinimumMoonPositionY)
                moon_position[1] -= kMoonTranslateIncrement;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            moon_position[1] += kMoonTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
            moon_position[2] -= kMoonTranslateIncrement;

        if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
            moon_position[2] += kMoonTranslateIncrement;
    }

    ============================================================
    */
}

void resize_window(GLFWwindow *window)
{
    int window_width = 0;
    int window_height = 0;

    glfwGetFramebufferSize(
        window,
        &window_width,
        &window_height);

    if (window_width > 0 && window_height > 0)
    {
        glViewport(
            0,
            0,
            window_width,
            window_height);
    }
}

void draw(GLFWwindow *window)
{
    int window_width = 0;
    int window_height = 0;

    glfwGetFramebufferSize(
        window,
        &window_width,
        &window_height);

    if (window_width <= 0 || window_height <= 0)
    {
        return;
    }

    const GLdouble aspect_ratio =
        static_cast<GLdouble>(window_width) /
        static_cast<GLdouble>(window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        kPerspectiveFieldOfViewAngle,
        aspect_ratio,
        kPerspectiveNearZ,
        kPerspectiveFarZ);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        camera_eye_x,
        camera_eye_y,
        camera_eye_z,
        camera_eye_x + camera_direction_x,
        camera_eye_y + camera_direction_y,
        camera_eye_z + camera_direction_z,
        kCameraUpX,
        kCameraUpY,
        kCameraUpZ);

    glTranslatef(
        0.0f,
        0.0f,
        -kPerspectiveTranslateZ);

    // --------------------------------------------------------
    // ATUALIZACAO AUTOMATICA DA ORBITA DA LUA
    // --------------------------------------------------------

    // Usa o tempo decorrido desde a inicializacao do GLFW.
    const GLfloat elapsed_time =
        static_cast<GLfloat>(glfwGetTime());

    // Calcula o angulo atual da orbita.
    moon_orbit_angle =
        kMoonOrbitSpeed * elapsed_time;

    // Calcula a posicao da Lua em uma orbita circular.
    moon_position[0] =
        kMoonOrbitCenterX +
        kMoonOrbitRadiusX * cosf(moon_orbit_angle);

    moon_position[1] = kMoonOrbitHeight;

    moon_position[2] =
        kMoonOrbitCenterZ +
        kMoonOrbitRadiusZ * sinf(moon_orbit_angle);

    moon_position[3] = 1.0f;

    // A luz acompanha a posicao atual da Lua.
    // Esta chamada ocorre depois da configuracao da camera.
    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        moon_position);

    // --------------------------------------------------------
    // AGUA
    // --------------------------------------------------------

    glPushAttrib(
        GL_ENABLE_BIT |
        GL_TEXTURE_BIT |
        GL_CURRENT_BIT |
        GL_PIXEL_MODE_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    glDisable(GL_BLEND);
    glDisable(GL_ALPHA_TEST);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(
        GL_TEXTURE_2D,
        water_texture_id);

    glTexEnvi(
        GL_TEXTURE_ENV,
        GL_TEXTURE_ENV_MODE,
        GL_MODULATE);

    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        1.0f);

    // Normal apontada para cima.
    glNormal3f(
        0.0f,
        1.0f,
        0.0f);

    glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(-50.0f, 0.0f, -50.0f);

    glTexCoord2f(3.0f, 0.0f);
    glVertex3f(50.0f, 0.0f, -50.0f);

    glTexCoord2f(3.0f, 3.0f);
    glVertex3f(50.0f, 0.0f, 50.0f);

    glTexCoord2f(0.0f, 3.0f);
    glVertex3f(-50.0f, 0.0f, 50.0f);

    glEnd();

    glPopAttrib();

    // --------------------------------------------------------
    // BARCO
    // --------------------------------------------------------

    glPushAttrib(
        GL_ENABLE_BIT |
        GL_TEXTURE_BIT |
        GL_CURRENT_BIT);

    glEnable(GL_DEPTH_TEST);

    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        1.0f);

    glPushMatrix();

    glTranslatef(
        0.0f,
        2.0f,
        0.0f);

    glRotatef(
        -30.0f,
        0.0f,
        1.0f,
        0.0f);

    glRotatef(
        -3.0f,
        0.0f,
        0.0f,
        1.0f);

    glScalef(
        2.0f,
        2.0f,
        2.0f);

    glCallList(boat_model_display_list_id);

    glPopMatrix();

    glPopAttrib();

    // --------------------------------------------------------
    // PORTO
    // --------------------------------------------------------

    glPushAttrib(
        GL_ENABLE_BIT |
        GL_TEXTURE_BIT |
        GL_CURRENT_BIT);

    glEnable(GL_DEPTH_TEST);

    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        1.0f);

    glPushMatrix();

    glTranslatef(
        kPortX,
        kPortY,
        kPortZ);

    glScalef(
        kPortScale,
        kPortScale,
        kPortScale);

    glCallList(port_model_display_list_id);

    glPopMatrix();

    glPopAttrib();

    // --------------------------------------------------------
    // LUA 3D
    // --------------------------------------------------------

    glPushAttrib(
        GL_ENABLE_BIT |
        GL_TEXTURE_BIT |
        GL_CURRENT_BIT |
        GL_LIGHTING_BIT);

    glEnable(GL_DEPTH_TEST);

    glColor4f(
        1.0f,
        1.0f,
        1.0f,
        1.0f);

    glPushMatrix();

    // Emissao faz a Lua parecer luminosa.
    // Ela nao ilumina os outros objetos automaticamente.
    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_EMISSION,
        kMoonEmission);

    // A Lua usa a mesma posicao calculada para a fonte luminosa.
    glTranslatef(
        moon_position[0],
        moon_position[1],
        moon_position[2]);

    glScalef(
        kMoonScale,
        kMoonScale,
        kMoonScale);

    glCallList(moon_model_display_list_id);

    glPopMatrix();

    glPopAttrib();
}

int main()
{
    if (!glfwInit())
    {
        std::cerr
            << "Falha ao inicializar GLFW"
            << std::endl;

        return EXIT_FAILURE;
    }

    GLFWwindow *window = glfwCreateWindow(
        800,
        800,
        "Cena 3D",
        nullptr,
        nullptr);

    if (!window)
    {
        std::cerr
            << "Falha ao criar a janela GLFW"
            << std::endl;

        glfwTerminate();

        return EXIT_FAILURE;
    }

    glfwSetWindowPos(
        window,
        0,
        0);

    glfwMakeContextCurrent(window);

    glfwSetInputMode(
        window,
        GLFW_CURSOR,
        GLFW_CURSOR_DISABLED);

    glfwSetCursorPosCallback(
        window,
        mouse_read);

    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK)
    {
        std::cerr
            << "Falha ao inicializar GLEW"
            << std::endl;

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    glGetError();

    load_water_texture(
        water_texture_id,
        "textures/water.jpg");

    if (!load_model(
            boat_model,
            "models/barco.glb"))
    {
        std::cerr
            << "Erro ao carregar models/barco.glb"
            << std::endl;

        glDeleteTextures(
            1,
            &water_texture_id);

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    create_model_display_list(
        boat_model_display_list_id,
        boat_model);

    if (!load_model(
            port_model,
            "models/porto.glb"))
    {
        std::cerr
            << "Erro ao carregar models/porto.glb"
            << std::endl;

        glDeleteLists(
            boat_model_display_list_id,
            1);

        destroy_model(boat_model);

        glDeleteTextures(
            1,
            &water_texture_id);

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    create_model_display_list(
        port_model_display_list_id,
        port_model);

    if (!load_model(
            moon_model,
            "models/moon_small.glb"))
    {
        std::cerr
            << "Erro ao carregar models/moon_small.glb"
            << std::endl;

        glDeleteLists(
            boat_model_display_list_id,
            1);

        glDeleteLists(
            port_model_display_list_id,
            1);

        destroy_model(boat_model);
        destroy_model(port_model);

        glDeleteTextures(
            1,
            &water_texture_id);

        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_FAILURE;
    }

    create_model_display_list(
        moon_model_display_list_id,
        moon_model);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Ativa a iluminacao e a fonte de luz.
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Iluminacao ambiente global da cena noturna.
    const GLfloat kNightAmbient[] = {
        0.16f, 0.18f, 0.28f, 1.0f};

    glLightModelfv(
        GL_LIGHT_MODEL_AMBIENT,
        kNightAmbient);

    // Propriedades da luz da Lua.
    glLightfv(
        GL_LIGHT0,
        GL_AMBIENT,
        kMoonAmbient);

    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        kMoonDiffuse);

    glLightfv(
        GL_LIGHT0,
        GL_SPECULAR,
        kMoonSpecular);

    // Atenuacao da luz conforme a distancia.
    glLightf(
        GL_LIGHT0,
        GL_CONSTANT_ATTENUATION,
        kMoonConstantAttenuation);

    glLightf(
        GL_LIGHT0,
        GL_LINEAR_ATTENUATION,
        kMoonLinearAttenuation);

    glLightf(
        GL_LIGHT0,
        GL_QUADRATIC_ATTENUATION,
        kMoonQuadraticAttenuation);

    glLightModeli(
        GL_LIGHT_MODEL_LOCAL_VIEWER,
        GL_TRUE);

    // Normaliza as normais dos modelos.
    glEnable(GL_NORMALIZE);

    // Permite que a cor corrente influencie o material.
    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE);

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(
            kSkyColorNight[0],
            kSkyColorNight[1],
            kSkyColorNight[2],
            kSkyColorNight[3]);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT);

        keyboard_read(window);

        resize_window(window);

        // A Lua e a luz sao atualizadas em draw().
        draw(window);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glDeleteLists(
        boat_model_display_list_id,
        1);

    glDeleteLists(
        port_model_display_list_id,
        1);

    glDeleteLists(
        moon_model_display_list_id,
        1);

    glDeleteTextures(
        1,
        &water_texture_id);

    destroy_model(boat_model);
    destroy_model(port_model);
    destroy_model(moon_model);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}