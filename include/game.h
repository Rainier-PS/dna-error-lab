#ifndef GAME_H
#define GAME_H

#include <time.h>
#include "challenge.h"
#include "execution.h"
#include "mutation.h"

// Screens the game can display
typedef enum
{
    SCREEN_SPLASH       = 0,
    SCREEN_MAINMENU     = 1,
    SCREEN_TUTORIAL     = 2,
    SCREEN_CAMPAIGN     = 3,
    SCREEN_LEVELS       = 4,
    SCREEN_PLAYING      = 5,
    SCREEN_RESULT       = 6,
    SCREEN_GAMEOVER     = 7,
    SCREEN_SIMULATION   = 8,
    SCREEN_SETTINGS     = 9,
    SCREEN_HELP         = 10,
} GameScreen;

// Main menu options
typedef enum
{
    MENU_CAMPAIGN       = 0,
    MENU_SIMULATION     = 1,
    MENU_LEARN          = 2,
    MENU_HELP           = 3,
} MainMenuOption;

// Simulation sub-modes
typedef enum
{
    SIM_MENU            = 0,
    SIM_INPUT           = 1,
    SIM_PLAYING         = 2,
} SimMode;

// Instruction input modes during the gameplay
typedef enum
{
    INPUT_MODE_NONE         = 0,
    INPUT_MODE_TYPE         = 1,
    INPUT_MODE_POS1         = 2,
    INPUT_MODE_POS2         = 3,
    INPUT_MODE_BASE         = 4,
    INPUT_MODE_CONFIRM      = 5,
} InputMode;

// Execution display mode
typedef enum
{
    EXEC_MODE_NONE          = 0,
    EXEC_MODE_RUN           = 1,
    EXEC_MODE_STEP          = 2,
} ExecMode;

// The complete game state
typedef struct
{
    // Screen Management
    GameScreen screen;
    int selected_option;
    int quit;

    // Progress Tracking
    int current_level;
    int selected_level;
    int max_level;
    int total_score;
    int level_score;

    Challenge *current_challenge;
    int seed_counter;

    // Current working tape (modified by the player instructions)
    Sequence *working_tape;

    PlayerProgram program;

    Inventory remaining_inventory;

    // Instruction input state
    InputMode input_mode;
    InstructionType pending_type;
    int pending_pos1;
    int pending_pos2;
    char pending_base;
    char input_buf[32];
    int input_len;

    // Execution state
    ExecMode exec_mode;
    ExecutionResult exec_result;
    int current_codon;

    time_t challenge_start;

    int tutorial_page;
    int tutorial_count;

    // Simulation state
    SimMode sim_mode;
    char sim_dna_buf[256];
    int sim_dna_len;
    int sim_menu_option;
    char sim_target[32][8];
    int sim_target_count;

    char message[256];
    int message_timer;

    // Settings
    int color_mode;
    int animation_mode;
    int sound_on;
    int settings_cursor;

    int help_scroll;
    int lab_ref_active;
} GameState;

#define GAME_KEY_UP         65517
#define GAME_KEY_DOWN       65516
#define GAME_KEY_LEFT       65515
#define GAME_KEY_RIGHT      65514
#define GAME_KEY_ENTER      13
#define GAME_KEY_ESC        27
#define GAME_KEY_BACKSPACE  8

void game_init(GameState *state);

void game_destroy(GameState *state);

void game_new_challenge(GameState *state);

void game_handle_key(GameState *state, int key);

const char *game_get_tutorial_title(int page);

const char *game_get_tutorial_body(int page);

int game_main_menu_count(void);

const char *game_main_menu_label(int index);

#endif