/**
* Author: Thu Vu
* Assignment: Lunar Lander
* Date due: 2023-07-07, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define PLATFORM_COUNT 5

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include <cstdlib>
#include <SDL_mixer.h>
#include "Entity.h"

// ––––– STRUCTS AND ENUMS ––––– //
struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* messages;
    Entity* background;
};

// ––––– CONSTANTS ––––– //
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

const float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char BACKGROUND_FILEPATH[]      = "assets/background.png";
const char SPRITESHEET_FILEPATH[]     = "assets/player_spritesheet.png";
const char WIN_PLATFORM_FILEPATH[]    = "assets/treasure_chest.png";
const char WIN_MESSAGE_FILEPATH[]     = "assets/win.png";
const char LOSE_PLATFORM_FILEPATH[]   = "assets/jellyfish.png";
const char LOSE_MESSAGE_FILEPATH[]    = "assets/lost.png";

const int NUMBER_OF_TEXTURES = 1;
const GLint LEVEL_OF_DETAIL  = 0;
const GLint TEXTURE_BORDER   = 0;

// ––––– GLOBAL VARIABLES ––––– //
GameState g_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;
bool g_player_win = false;
bool g_player_lost = false;

ShaderProgram g_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

// ––––– GENERAL FUNCTIONS ––––– //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    
    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    
    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_program.Load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_program.SetProjectionMatrix(g_projection_matrix);
    g_program.SetViewMatrix(g_view_matrix);
    
    glUseProgram(g_program.programID);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    // ––––– TEXTURE IDS ––––– //
    GLuint win_platform_texture_id = load_texture(WIN_PLATFORM_FILEPATH);
    GLuint win_message_texture_id = load_texture(WIN_MESSAGE_FILEPATH);
    GLuint lose_platform_texture_id = load_texture(LOSE_PLATFORM_FILEPATH);
    GLuint lose_message_texture_id = load_texture(LOSE_MESSAGE_FILEPATH);
    GLuint background_texture_id = load_texture(BACKGROUND_FILEPATH);
    
    // Background
    g_state.background = new Entity();
    g_state.background->m_texture_id = background_texture_id;
    g_state.background->set_position(glm::vec3(0.0f, -1.5f, 0.0f));
    g_state.background->set_size(glm::vec3(11.5f, 8.0f, 1.0f));
    
    
    g_state.platforms = new Entity[PLATFORM_COUNT];

    // Treasure chests
    g_state.platforms[3].m_texture_id = win_platform_texture_id;
    g_state.platforms[3].set_position(glm::vec3(-3.5f, -2.5f, 0.0f));
    g_state.platforms[3].set_width(1.75f);
    g_state.platforms[3].set_height(1.25f);
    g_state.platforms[3].set_entity_type(WIN_PLATFORM);
    g_state.platforms[3].update(0.0f, NULL, 0, g_player_win, g_player_lost);
    g_state.platforms[3].set_size(glm::vec3(1.75f, 1.25f, 1.0f));
    
    g_state.platforms[4].m_texture_id = win_platform_texture_id;
    g_state.platforms[4].set_position(glm::vec3(3.5f, -2.5f, 0.0f));
    g_state.platforms[4].set_width(1.75f);
    g_state.platforms[4].set_height(1.25f);
    g_state.platforms[4].set_entity_type(WIN_PLATFORM);
    g_state.platforms[4].update(0.0f, NULL, 0, g_player_win, g_player_lost);
    g_state.platforms[4].set_size(glm::vec3(1.75f, 1.25f, 1.0f));
    
    // Jellyfish
    g_state.platforms[0].m_texture_id = lose_platform_texture_id;
    g_state.platforms[0].set_position(glm::vec3(-3.5f, 2.5f, 0.0f));
    g_state.platforms[0].set_width(1.5f);
    g_state.platforms[0].set_height(2.0f);
    g_state.platforms[0].set_entity_type(LOSE_PLATFORM);
    g_state.platforms[0].update(0.0f, NULL, 0, g_player_win, g_player_lost);
    g_state.platforms[0].set_size(glm::vec3(1.5f, 2.0f, 1.0f));
    
    g_state.platforms[1].m_texture_id = lose_platform_texture_id;
    g_state.platforms[1].set_position(glm::vec3(3.5f, 2.5f, 0.0f));
    g_state.platforms[1].set_width(1.0f);
    g_state.platforms[1].set_height(1.5f);
    g_state.platforms[1].set_entity_type(LOSE_PLATFORM);
    g_state.platforms[1].update(0.0f, NULL, 0, g_player_win, g_player_lost);
    g_state.platforms[1].set_size(glm::vec3(1.0f, 1.5f, 1.0f));
    
    g_state.platforms[2].m_texture_id = lose_platform_texture_id;
    g_state.platforms[2].set_position(glm::vec3(1.5f, 0.0f, 0.0f));
    g_state.platforms[2].set_width(0.8f);
    g_state.platforms[2].set_height(2.0f);
    g_state.platforms[2].set_entity_type(LOSE_PLATFORM);
    g_state.platforms[2].update(0.0f, NULL, 0, g_player_win, g_player_lost);
    g_state.platforms[2].set_size(glm::vec3(0.8f, 2.0f, 1.0f));
    
    // ––––– MESSAGES ––––– //
    g_state.messages = new Entity[2];
    g_state.messages[0].m_texture_id = win_message_texture_id;
    g_state.messages[1].m_texture_id = lose_message_texture_id;
    for (int i = 0; i < 2; i++)
    {
        g_state.messages[i].set_position(glm::vec3(0.0f));
        g_state.messages[i].m_model_matrix = glm::scale(g_state.messages[i].m_model_matrix,
                                                        glm::vec3(5.0f, 3.0f, 1.0f));
        g_state.messages[i].set_entity_type(MESSAGE);
        g_state.messages[i].deactivate();
    }
    
    // ––––– PLAYER ––––– //
    // Existing
    g_state.player = new Entity();
    g_state.player->set_position(glm::vec3(0.0f));
    g_state.player->set_movement(glm::vec3(0.0f));
    g_state.player->set_entity_type(PLAYER);
    g_state.player->m_speed = 1.0f;
    g_state.player->set_acceleration(glm::vec3(0.0f, -4.905f, 0.0f));
    g_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);
    
    // Walking
    g_state.player->m_walking[g_state.player->LEFT]  = new int[4] { 4,   5,  6,  7 };
    g_state.player->m_walking[g_state.player->RIGHT] = new int[4] { 8,   9, 10, 12 };
    g_state.player->m_walking[g_state.player->UP]    = new int[4] { 12, 13, 14, 15 };
    g_state.player->m_walking[g_state.player->DOWN]  = new int[4] { 0,   1,  2,  3 };

    g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];  // start George looking left
    g_state.player->m_animation_frames = 4;
    g_state.player->m_animation_index  = 0;
    g_state.player->m_animation_time   = 0.0f;
    g_state.player->m_animation_cols   = 4;
    g_state.player->m_animation_rows   = 4;
    g_state.player->set_height(0.9f);
    g_state.player->set_width(0.9f);
    
    // Jumping
    g_state.player->m_jumping_power = 3.0f;
    
    // ––––– GENERAL ––––– //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    g_state.player->set_movement(glm::vec3(0.0f));
    
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_game_is_running = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        // Quit the game with a keystroke
                        g_game_is_running = false;
                        break;
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);

    // Movement
    if (key_state[SDL_SCANCODE_LEFT])
    {
        g_state.player->m_movement.x = -1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->LEFT];
    }
    else if (key_state[SDL_SCANCODE_RIGHT])
    {
        g_state.player->m_movement.x = 1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->RIGHT];
    }
    else if (key_state[SDL_SCANCODE_UP])
    {
        g_state.player->m_movement.y = 1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->UP];
    }
    else if (key_state[SDL_SCANCODE_DOWN])
    {
        g_state.player->m_movement.y = -1.0f;
        g_state.player->m_animation_indices = g_state.player->m_walking[g_state.player->DOWN];
    }
    
    // Normalize
    if (glm::length(g_state.player->m_movement) > 1.0f)
    {
        g_state.player->m_movement = glm::normalize(g_state.player->m_movement);
    }
}

void update()
{
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP)
    {
        g_state.player->update(FIXED_TIMESTEP, g_state.platforms, PLATFORM_COUNT,
                               g_player_win, g_player_lost);
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    if (g_player_win)
    {
        g_state.messages[0].activate();
    }
    if (g_player_lost)
    {
        g_state.messages[1].activate();
    }
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);
    
    g_state.background->render(&g_program);
    
    g_state.player->render(&g_program);
    
    for (int i = 0; i < PLATFORM_COUNT; i++) g_state.platforms[i].render(&g_program);
    
    for (int i = 0; i < 2; i++) g_state.messages[i].render(&g_program);
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    
    delete [] g_state.platforms;
    delete g_state.player;
}

// ––––– GAME LOOP ––––– //
int main(int argc, char* argv[])
{
    initialise();
    
    while (g_game_is_running)
    {
        process_input();
        if (g_player_win == 0 and g_player_lost == 0) {
            update();
        }
        render();
    }
    
    shutdown();
    return 0;
}
