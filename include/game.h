#ifndef GAME_H
#define GAME_H

#include <time.h>
#include "challenge.h"
#include "execution.h"
#include "mutation.h"

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

typedef enum
{
    MENU_CAMPAIGN       = 0,
    MENU_SIMULATION     = 1,
    MENU_LEARN          = 2,
    MENU_HELP           = 3,
} MainMenuOption;