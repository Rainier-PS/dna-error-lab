#ifndef SCORING_H
#define SCORING_H

#include <stddef.h>

#define SCORE_BASE 500

#define SCORE_DIFFICULTY_MULT 100

#define SCORE_HINT_BONUS 100

#define SCORE_SPEED_BONUS 50

#define SCORE_SPEED_THRESHOLD 30

#define SCORE_EFFICIENCY_BONUS 100

#define SCORE_PENALTY -100

/* Calculate score based on difficulty, hints, time, and correctness */
int scoring_calculate(int difficulty, int hints_used, int elapsed_seconds, int correct);

void scoring_format(int score, char *buf, size_t bufsize);

#endif