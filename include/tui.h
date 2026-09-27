#ifndef TUI_H
#define TUI_H

#include "game.h"

/* Initialize the terminal and termbox2 library */
void tui_init(void);

/* Shut down termbox2 and restore the terminal */
void tui_shutdown(void);

/* Draw the current screen based on game state */
void tui_draw(GameState *state);

/* Run the main event loop (blocks until game quits) */
void tui_run(GameState *state);

#endif
