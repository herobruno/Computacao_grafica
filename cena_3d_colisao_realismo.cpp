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
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

#include "glb_model.h"
#include "island_collision.h"

// ============================================================
// CAMERA
// ============================================================

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

// ============================================================
// PERSPECTIVA
// ============================================================

const GLfloat kPerspectiveFieldOfViewAngle = 45.0f;
const GLfloat kPerspectiveNearZ = 0.1f;
const GLfloat kPerspectiveFarZ = 100.0f;
const GLfloat kPerspectiveTranslateZ = 10.0f;

// ============================================================
// TEXTURAS E MODELOS
// ============================================================

GLuint water_texture_id = 0;

GLuint boat_model_display_list_id = 0;
GLuint island_model_display_list_id = 0;
GLuint moon_model_display_list_id = 0;

GlbModel boat_model;
GlbModel island_model;
GlbModel moon_model;

// ============================================================
// ILUMINACAO DA LUA
// ============================================================

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
// POSICAO E CONTROLE MANUAL DA LUA
// ============================================================

const GLfloat kMoonScale = 1.5f;

GLfloat moon_position[] = {
    0.0f,
    1.5f,
    -25.0f,
    1.0f};

const GLfloat kMoonTranslateIncrement = 0.4f;
const GLfloat kMinimumMoonPositionY = 0.8f;

// ============================================================
// ILHA E CEU
// ============================================================

// Posicao e escala da ilha.
const GLfloat kIslandX = 4.0f;
const GLfloat kIslandY = 0.0f;
const GLfloat kIslandZ = 0.0f;
const GLfloat kIslandScale = 2.0f;

// Posicao do barco: afastado da ilha para permanecer sobre a agua.
const GLfloat kBoatX = -18.0f;
const GLfloat kBoatY = 2.0f;
const GLfloat kBoatZ = 0.0f;

const GLfloat kSkyColorNight[] = {
    0.015f, 0.025f, 0.09f, 1.0f};

// ============================================================
// ANIMACAO DO BARCO
// ============================================================

double last_time = 0.0;

const GLfloat kBoatAnimationSpeed = 0.50f;
const GLfloat kBoatRotationAmplitude = 1.5f;
const GLfloat kBoatVerticalAmplitude = 0.035f;

GLfloat boat_animation_time = 0.0f;
GLfloat boat_rotation = 0.0f;
GLfloat boat_vertical_offset = 0.0f;

// ============================================================
// COLISAO E CAMINHADA NA ILHA
// ============================================================

// true  = anda no chao da ilha, com colisao.
// false = camera livre (voo, sem colisao) - util para depurar.
bool walk_mode = true;
bool toggle_key_was_pressed = false;

// Altura dos olhos acima do chao (unidades do mundo).
const GLfloat kPlayerEyeHeight = 1.0f;

// Suavizacao da subida/descida do terreno (0 a 1).
const GLfloat kGroundFollowSmoothing = 0.25f;

// A camera usa gluLookAt + glTranslatef(0, 0, -kPerspectiveTranslateZ).
// Por isso, a posicao da camera no espaco da cena (onde estao a ilha,
// o barco e a Lua) e: olho + (0, 0, kPerspectiveTranslateZ).
bool can_walk_to(GLfloat eye_x, GLfloat eye_z)
{
    return island_is_walkable(
        eye_x,
        eye_z + kPerspectiveTranslateZ,
        kIslandX,
        kIslandZ,
        kIslandScale);
}

GLfloat ground_eye_height(GLfloat eye_x, GLfloat eye_z)
{
    return island_ground_height(
               eye_x,
               eye_z + kPerspectiveTranslateZ,
               kIslandX,
               kIslandY,
               kIslandZ,
               kIslandScale) +
           kPlayerEyeHeight;
}

// Move um eixo de cada vez: se um deles bate na parede,
// o outro continua livre e a camera "desliza" pela parede.
void move_camera_walking(GLfloat delta_x, GLfloat delta_z)
{
    if (can_walk_to(camera_eye_x + delta_x, camera_eye_z))
        camera_eye_x += delta_x;

    if (can_walk_to(camera_eye_x, camera_eye_z + delta_z))
        camera_eye_z += delta_z;
}

// Mantem os olhos a kPlayerEyeHeight acima do terreno.
void stick_camera_to_ground(bool instantly)
{
    const GLfloat target_y =
        ground_eye_height(camera_eye_x, camera_eye_z);

    if (instantly)
        camera_eye_y = target_y;
    else
        camera_eye_y +=
            (target_y - camera_eye_y) * kGroundFollowSmoothing;
}

// Coloca a pessoa em pe no chao da ilha, olhando para o castelo.
void reset_camera_to_island()
{
    camera_eye_x =
        kIslandX + kIslandSpawnLocalX * kIslandScale;

    camera_eye_z =
        kIslandZ + kIslandSpawnLocalZ * kIslandScale -
        kPerspectiveTranslateZ;

    camera_direction_x = kDefaultCameraDirectionX;
    camera_direction_y = kDefaultCameraDirectionY;
    camera_direction_z = kDefaultCameraDirectionZ;

    camera_yaw = kDefaultCameraYaw;
    camera_pitch = kDefaultCameraPitch;

    camera_right_x = 1.0f;
    camera_right_y = 0.0f;
    camera_right_z = 0.0f;

    walk_mode = true;

    stick_camera_to_ground(true);
}

// ============================================================
// SISTEMA DE PARTICULAS - CHUVA
// ============================================================

// Cada particula representa uma gota de chuva.
struct RainParticle
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
};

// Quantidade de particulas.
const int kRainParticleQuantity = 2500;

// Limites horizontais da chuva.
const GLfloat kRainMinimumX = -45.0f;
const GLfloat kRainMaximumX = 45.0f;

// Limites verticais da chuva.
const GLfloat kRainMinimumY = 0.0f;
const GLfloat kRainMaximumY = 30.0f;

// Limites de profundidade.
const GLfloat kRainMinimumZ = -45.0f;
const GLfloat kRainMaximumZ = 45.0f;

// Velocidade de queda das gotas.
const GLfloat kRainSpeed = 18.0f;

// Comprimento dos riscos.
const GLfloat kRainDropLength = 0.7f;

// Cor azulada e transparencia das gotas.
const GLfloat kRainColor[] = {
    0.65f, 0.78f, 0.95f, 0.65f};

// Espessura das linhas.
const GLfloat kRainLineWidth = 1.2f;

// Armazenamento das particulas.
std::vector<RainParticle> rain_particles;

// Display List compartilhada por todas as gotas.
GLuint rain_display_list_id = 0;

// ============================================================
// CRIACAO DE DISPLAY LIST DOS MODELOS
// ============================================================

void create_model_display_list(
    GLuint &model_display_list_id,
    const GlbModel &model)
{
    model_display_list_id = glGenLists(1);

    glNewList(model_display_list_id, GL_COMPILE);

    draw_model(model);

    glEndList();
}

// ============================================================
// CRIACAO DA DISPLAY LIST DA CHUVA
// ============================================================

void create_rain_display_list()
{
    rain_display_list_id = glGenLists(1);

    glNewList(rain_display_list_id, GL_COMPILE);

    // Cada particula e representada por uma linha.
    glBegin(GL_LINES);

    // Inicio do risco.
    glVertex3f(
        0.0f,
        0.0f,
        0.0f);

    // Final do risco.
    glVertex3f(
        0.06f,
        -kRainDropLength,
        0.0f);

    glEnd();

    glEndList();
}

// ============================================================
// GERACAO DE POSICOES ALEATORIAS
// ============================================================

GLfloat random_rain_value(
    GLfloat minimum,
    GLfloat maximum)
{
    return minimum +
        (static_cast<GLfloat>(rand()) / RAND_MAX) *
        (maximum - minimum);
}

// ============================================================
// INICIALIZACAO DAS PARTICULAS
// ============================================================

void initialize_rain()
{
    rain_particles.resize(kRainParticleQuantity);

    for (int i = 0; i < kRainParticleQuantity; ++i)
    {
        rain_particles[i].x =
            random_rain_value(
                kRainMinimumX,
                kRainMaximumX);

        rain_particles[i].y =
            random_rain_value(
                kRainMinimumY,
                kRainMaximumY);

        rain_particles[i].z =
            random_rain_value(
                kRainMinimumZ,
                kRainMaximumZ);
    }
}

// ============================================================
// ATUALIZACAO DAS PARTICULAS
// ============================================================

void update_rain(GLfloat delta_time)
{
    for (RainParticle &particle : rain_particles)
    {
        // Movimento vertical da gota.
        particle.y -= kRainSpeed * delta_time;

        // Quando a gota chega ao limite inferior,
        // ela retorna para uma nova posicao no topo.
        if (particle.y < kRainMinimumY)
        {
            particle.x =
                random_rain_value(
                    kRainMinimumX,
                    kRainMaximumX);

            particle.y =
                random_rain_value(
                    20.0f,
                    kRainMaximumY);

            particle.z =
                random_rain_value(
                    kRainMinimumZ,
                    kRainMaximumZ);
        }
    }
}

// ============================================================
// RENDERIZACAO DAS PARTICULAS
// ============================================================

void draw_rain()
{
    if (rain_display_list_id == 0)
        return;

    glPushAttrib(
        GL_ENABLE_BIT |
        GL_CURRENT_BIT |
        GL_LINE_BIT |
        GL_DEPTH_BUFFER_BIT);

    // Teste de profundidade.
    glEnable(GL_DEPTH_TEST);

    // Desativa iluminacao nas particulas.
    glDisable(GL_LIGHTING);

    // Ativa transparencia.
    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA);

    // Evita alterar o buffer de profundidade.
    glDepthMask(GL_FALSE);

    // Define a cor da chuva.
    glColor4fv(kRainColor);

    // Define a espessura dos riscos.
    glLineWidth(kRainLineWidth);

    // Percorre todas as particulas.
    for (const RainParticle &particle : rain_particles)
    {
        glPushMatrix();

        // Posiciona a particula no ambiente 3D.
        glTranslatef(
            particle.x,
            particle.y,
            particle.z);

        // Desenha o risco armazenado na Display List.
        glCallList(rain_display_list_id);

        glPopMatrix();
    }

    // Restaura a escrita no buffer de profundidade.
    glDepthMask(GL_TRUE);

    glPopAttrib();
}

// ============================================================
// CARREGAMENTO DA TEXTURA DA AGUA
// ============================================================

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
        std::cerr
            << "Erro ao carregar a textura: "
            << filepath << std::endl;

        std::cerr
            << stbi_failure_reason()
            << std::endl;

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
        std::cerr
            << "Formato de textura nao suportado."
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

    glBindTexture(
        GL_TEXTURE_2D,
        texture_id);

    glPixelStorei(
        GL_UNPACK_ALIGNMENT,
        1);

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

// ============================================================
// CONTROLE DA CAMERA PELO MOUSE
// ============================================================

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

// ============================================================
// CONTROLE DO TECLADO
// ============================================================

void keyboard_read(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // F: alterna entre caminhar na ilha (com colisao) e camera livre.
    const bool toggle_key_pressed =
        glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;

    if (toggle_key_pressed && !toggle_key_was_pressed)
    {
        walk_mode = !walk_mode;

        // Ao voltar a caminhar fora da ilha, volta para o ponto inicial.
        if (walk_mode &&
            !can_walk_to(camera_eye_x, camera_eye_z))
        {
            reset_camera_to_island();
        }
    }

    toggle_key_was_pressed = toggle_key_pressed;

    const bool key_w = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    const bool key_s = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    const bool key_a = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    const bool key_d = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

    if (walk_mode)
    {
        // Caminhada: usa so a direcao horizontal (nao "voa" olhando pra cima).
        GLfloat forward_x = camera_direction_x;
        GLfloat forward_z = camera_direction_z;

        const GLfloat forward_length =
            sqrtf(forward_x * forward_x + forward_z * forward_z);

        if (forward_length > 0.0001f)
        {
            forward_x /= forward_length;
            forward_z /= forward_length;
        }

        GLfloat move_x = 0.0f;
        GLfloat move_z = 0.0f;

        if (key_w) { move_x += forward_x; move_z += forward_z; }
        if (key_s) { move_x -= forward_x; move_z -= forward_z; }
        if (key_a) { move_x += camera_right_x; move_z += camera_right_z; }
        if (key_d) { move_x -= camera_right_x; move_z -= camera_right_z; }

        const GLfloat move_length =
            sqrtf(move_x * move_x + move_z * move_z);

        if (move_length > 0.0001f)
        {
            move_camera_walking(
                move_x / move_length * kCameraSpeed,
                move_z / move_length * kCameraSpeed);
        }

        stick_camera_to_ground(false);
    }
    else
    {
        // Camera livre (comportamento original).
        if (key_w)
        {
            camera_eye_x += camera_direction_x * kCameraSpeed;
            camera_eye_y += camera_direction_y * kCameraSpeed;
            camera_eye_z += camera_direction_z * kCameraSpeed;
        }

        if (key_s)
        {
            camera_eye_x -= camera_direction_x * kCameraSpeed;
            camera_eye_y -= camera_direction_y * kCameraSpeed;
            camera_eye_z -= camera_direction_z * kCameraSpeed;
        }

        if (key_a)
        {
            camera_eye_x += camera_right_x * kCameraSpeed;
            camera_eye_y += camera_right_y * kCameraSpeed;
            camera_eye_z += camera_right_z * kCameraSpeed;
        }

        if (key_d)
        {
            camera_eye_x -= camera_right_x * kCameraSpeed;
            camera_eye_y -= camera_right_y * kCameraSpeed;
            camera_eye_z -= camera_right_z * kCameraSpeed;
        }
    }

    // Espaco: volta para o ponto inicial, em pe na ilha.
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        reset_camera_to_island();
    }

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
}

// ============================================================
// REDIMENSIONAMENTO DA JANELA
// ============================================================

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

// ============================================================
// ANIMACAO
// ============================================================

void animate()
{
    const double current_time = glfwGetTime();
    const double delta_time = current_time - last_time;

    last_time = current_time;

    // Atualizacao da animacao do barco.
    boat_animation_time +=
        static_cast<GLfloat>(delta_time);

    boat_rotation =
        kBoatRotationAmplitude *
        sinf(boat_animation_time * kBoatAnimationSpeed);

    boat_vertical_offset =
        kBoatVerticalAmplitude *
        sinf(boat_animation_time * kBoatAnimationSpeed * 2.0f);

    // Atualizacao do sistema de particulas.
    update_rain(static_cast<GLfloat>(delta_time));
}

// ============================================================
// DESENHO DA CENA
// ============================================================

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

    // A luz acompanha a posicao atual da Lua.
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
        kBoatX,
        kBoatY + boat_vertical_offset,
        kBoatZ);

    glRotatef(
        -30.0f,
        0.0f,
        1.0f,
        0.0f);

    glRotatef(
        -3.0f + boat_rotation,
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
    // ILHA
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
        kIslandX,
        kIslandY,
        kIslandZ);

    glScalef(
        kIslandScale,
        kIslandScale,
        kIslandScale);

    glCallList(island_model_display_list_id);

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

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_EMISSION,
        kMoonEmission);

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

    // --------------------------------------------------------
    // CHUVA - SISTEMA DE PARTICULAS
    // --------------------------------------------------------

    draw_rain();
}

// ============================================================
// MAIN
// ============================================================

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

    // --------------------------------------------------------
    // INICIALIZACAO DO SISTEMA DE PARTICULAS
    // --------------------------------------------------------

    srand(static_cast<unsigned int>(time(nullptr)));

    create_rain_display_list();

    initialize_rain();

    // --------------------------------------------------------
    // CARREGAMENTO DA TEXTURA
    // --------------------------------------------------------

    load_water_texture(
        water_texture_id,
        "textures/water.jpg");

    // --------------------------------------------------------
    // CARREGAMENTO DO BARCO
    // --------------------------------------------------------

    if (!load_model(
            boat_model,
            "models/barco.glb"))
    {
        std::cerr
            << "Erro ao carregar models/barco.glb"
            << std::endl;

        glDeleteLists(rain_display_list_id, 1);

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

    // --------------------------------------------------------
    // CARREGAMENTO DA ILHA
    // --------------------------------------------------------

    if (!load_model(
            island_model,
            "models/ilha_medieval_realista.glb"))
    {
        std::cerr
            << "Erro ao carregar models/ilha_medieval_realista.glb"
            << std::endl;

        glDeleteLists(
            boat_model_display_list_id,
            1);

        glDeleteLists(
            rain_display_list_id,
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
        island_model_display_list_id,
        island_model);

    // --------------------------------------------------------
    // CARREGAMENTO DA LUA
    // --------------------------------------------------------

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
            island_model_display_list_id,
            1);

        glDeleteLists(
            rain_display_list_id,
            1);

        destroy_model(boat_model);
        destroy_model(island_model);

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

    // --------------------------------------------------------
    // CONFIGURACAO DO OPENGL
    // --------------------------------------------------------

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Iluminacao ambiente global.
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

    glEnable(GL_NORMALIZE);

    glEnable(GL_COLOR_MATERIAL);

    glColorMaterial(
        GL_FRONT_AND_BACK,
        GL_AMBIENT_AND_DIFFUSE);

    // --------------------------------------------------------
    // POSICAO INICIAL: EM PE NO CHAO DA ILHA
    // --------------------------------------------------------

    reset_camera_to_island();

    // --------------------------------------------------------
    // INICIALIZACAO DO TEMPO
    // --------------------------------------------------------

    last_time = glfwGetTime();

    // --------------------------------------------------------
    // LOOP PRINCIPAL
    // --------------------------------------------------------

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

        // Atualiza barco e particulas.
        animate();

        // Desenha todos os elementos da cena.
        draw(window);

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    // --------------------------------------------------------
    // LIBERACAO DOS RECURSOS
    // --------------------------------------------------------

    glDeleteLists(
        boat_model_display_list_id,
        1);

    glDeleteLists(
        island_model_display_list_id,
        1);

    glDeleteLists(
        moon_model_display_list_id,
        1);

    // Libera a Display List da chuva.
    glDeleteLists(
        rain_display_list_id,
        1);

    glDeleteTextures(
        1,
        &water_texture_id);

    destroy_model(boat_model);
    destroy_model(island_model);
    destroy_model(moon_model);

    rain_particles.clear();

    glfwDestroyWindow(window);

    glfwTerminate();

    return EXIT_SUCCESS;
}
