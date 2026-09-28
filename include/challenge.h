#ifndef CHALLENGE_H
#define CHALLENGE_H

#include "sequence.h"
#include "mutation.h"
#include "execution.h"
#include "levels.h"

#define MAX_DIFFICULTY 15

/* A complete challenge instance */
typedef struct Challenge {
    /* The target output the machine must produce */
    char target[MAX_TARGET_CODONS][MAX_AA_NAME];
    int target_count;

    /* The final (solved) DNA tape */
    Sequence *final_tape;

    /* The initial (corrupted) DNA tape the player starts with */
    Sequence *initial_tape;

    const LevelDefinition *level_def;

    Inventory inventory;

    PlayerProgram solution;

    int difficulty;
    int seed;
} Challenge;

Challenge *challenge_generate(int seed, int difficulty);

void challenge_destroy(Challenge *challenge);

ExecutionResult challenge_execute(const Challenge *ch, const Sequence *tape);

#endif
