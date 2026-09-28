#ifndef LEVELS_H
#define LEVELS_H

#include "mutation.h"

#define MAX_TARGET_CODONS 32

/* Corruption type for level generation */
typedef enum {
    CORRUPTION_RANDOM = 0,
    CORRUPTION_INS_ONLY = 1,
    CORRUPTION_DEL_ONLY = 2,
    CORRUPTION_INS_DEL = 3,
    CORRUPTION_REV_ONLY = 4,
    CORRUPTION_SWP_ONLY = 5,
    CORRUPTION_DEL_REV = 6,
    CORRUPTION_DEL_INS_REV = 7
} CorruptionType;

#define CANONICAL_MAX_STEPS 8

/* Complete level definition */
typedef struct {
    int level_id;

    const char *objective;
    const char *description;
    const char *new_concept;

    int target_codon_count_min;
    int target_codon_count_max;

    /* Initial tape rules */
    int tape_length_min;
    int tape_length_max;

    /* Instruction inventory */
    int delete_budget;
    int insert_budget;
    int reverse_budget;
    int swap_budget;
    int difficulty;     // 1-15

    int intended_depth;
    CorruptionType corruption_type;

    const char *hint_level_1;
    const char *hint_level_2;
    const char *hint_level_3;
    const char *hint_level_4;

    const char *target_dna;
    const char *initial_dna;
    Instruction solution[CANONICAL_MAX_STEPS];
    int solution_count;
} LevelDefinition;

const LevelDefinition *level_get(int level_id);

int level_count(void);

void level_print(const LevelDefinition *level);

#endif