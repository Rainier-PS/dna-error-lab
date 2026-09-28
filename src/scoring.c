#include <stdio.h>
#include <string.h>
#include "scoring.h"

/* Calculate a score based on difficulty, hints, time, and correctness */
int scoring_calculate(int difficulty, int hints_used, int elapsed_seconds, int correct)
{
    int score;

    /* Wrong answers always get the penalty */
    if (!correct)
    {
        return SCORE_PENALTY;
    }

    score = 0;

    /* Base score for correct answer */
    score += SCORE_BASE;

    /* Difficulty bonus: more points for harder levels */
    score += difficulty * SCORE_DIFFICULTY_MULT;

    /* Hint bonus: extra points for using fewer hints */
    if (hints_used == 0)
    {
        score += SCORE_HINT_BONUS;
    }
    else if (hints_used == 1)
    {
        /* Half bonus for using just one hint */
        score += SCORE_HINT_BONUS / 2;
    }

    /* Speed bonus: solve quickly for extra points */
    if (elapsed_seconds <= SCORE_SPEED_THRESHOLD)
    {
        score += SCORE_SPEED_BONUS;
    }
    else if (elapsed_seconds <= SCORE_SPEED_THRESHOLD * 2)
    {
        /* Half bonus for moderate speed */
        score += SCORE_SPEED_BONUS / 2;
    }

    return score;
}

/* Write a formatted score string like "+750" into the buffer */
void scoring_format(int score, char *buf, size_t bufsize)
{
    if (buf == NULL || bufsize == 0)
        return;

    if (score >= 0)
    {
        snprintf(buf, bufsize, "+%d", score);
    }
    else
    {
        snprintf(buf, bufsize, "%d", score);
    }
}
