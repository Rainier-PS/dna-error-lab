#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "challenge.h"
#include "solver.h"
#include "levels.h"
#include "mutation.h"
#include "sequence.h"

/* Validate Level 15 by replaying canonical solution */
static int validate_level15(Challenge *ch) {
    const LevelDefinition *def = ch->level_def;
    if (!def || def->solution_count <= 0) return 0;

    /* Check initial is not already solved */
    ExecutionResult init_er = challenge_execute(ch, ch->initial_tape);
    if (init_er.diagnostic == DIAG_SUCCESS) return 0;

    /* Replay canonical solution */
    Sequence *replay = sequence_copy(ch->initial_tape);
    int i;
    for (i = 0; i < def->solution_count; i++) {
        Instruction inst = def->solution[i];
        if (mutation_apply(inst.type, inst.pos1, inst.pos2,
                           inst.base, replay) != APPLY_SUCCESS) {
            sequence_destroy(replay);
            return 0;
                           }
    }

    /* Check final result */
    ExecutionResult final_er = challenge_execute(ch, replay);
    int ok = (final_er.diagnostic == DIAG_SUCCESS);
    sequence_destroy(replay);
    return ok;
}

int main(void)
{
    int level;
    int total_pass = 0;
    int total_fail = 0;
    int seeds_to_try[] = {42, 100, 999, 12345, 54321};
    int num_seeds = 5;

    printf("Challenge Generation Validation\n\n");

    for (level = 1; level <= 15; level++)
    {
        int level_pass = 0;
        int level_fail = 0;
        int s;

        for (s = 0; s < num_seeds; s++)
        {
            Challenge *ch;
            int seed = seeds_to_try[s];

            ch = challenge_generate(seed, level);
            if (ch == NULL)
            {
                printf("Level %2d [seed=%5d]: FAILED (generation returned NULL)\n",
                       level, seed);
                level_fail++;
                total_fail++;
                continue;
            }

            /* Level 15: validate by canonical solution replay */
            if (level == 15) {
                if (validate_level15(ch)) {
                    printf("Level %2d [seed=%5d]: PASS (canonical replay verified)\n",
                           level, seed);
                    level_pass++;
                    total_pass++;
                } else {
                    printf("Level %2d [seed=%5d]: FAILED (canonical replay failed)\n",
                           level, seed);
                    level_fail++;
                    total_fail++;
                }
                challenge_destroy(ch);
                continue;
            }

            /* Levels 1-14: validate with solver */
            if (!solver_validate_challenge(ch))
            {
                const SolverResult *sr = solver_get_last_result();
                printf("Level %2d [seed=%5d]: FAILED solver: %s\n",
                       level, seed, sr->message);
                level_fail++;
                total_fail++;
            }
            else
            {
                const SolverResult *sr = solver_get_last_result();
                const LevelDefinition *lv = level_get(level);

                /* Verify depth matches */
                if (lv != NULL && sr->min_depth != lv->intended_depth)
                {
                    /* Allow range for level 7 */
                    int depth_ok = 0;
                    if (level == 7 && sr->min_depth >= 1 && sr->min_depth <= 2)
                        depth_ok = 1;
                    if (!depth_ok)
                    {
                        printf("Level %2d [seed=%5d]: FAILED depth=%d expected=%d\n",
                               level, seed, sr->min_depth, lv->intended_depth);
                        level_fail++;
                        total_fail++;
                        challenge_destroy(ch);
                        continue;
                    }
                }

                /* Verify initial tape is not already solved */
                {
                    ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
                    if (init_exec.diagnostic == DIAG_SUCCESS)
                    {
                        printf("Level %2d [seed=%5d]: FAILED (initial tape already solved)\n",
                               level, seed);
                        level_fail++;
                        total_fail++;
                        challenge_destroy(ch);
                        continue;
                    }
                }

                printf("Level %2d [seed=%5d]: PASS (depth=%d, solutions=%d)\n",
                       level, seed, sr->min_depth, sr->solution_count);
                level_pass++;
                total_pass++;
            }

            challenge_destroy(ch);
        }

        printf("  Level %2d summary: %d/%d passed\n\n",
               level, level_pass, num_seeds);
    }

    printf("TOTAL: %d/%d passed\n", total_pass, total_pass + total_fail);

    return (total_fail > 0) ? 1 : 0;
}
