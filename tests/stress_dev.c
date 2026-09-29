#include <stdio.h>
#include <string.h>
#include "challenge.h"
#include "levels.h"
#include "sequence.h"
#include "mutation.h"
#include "execution.h"

int main(void) {
    int total = 0, passed = 0, failed = 0;
    int seed, level;

    for (seed = 0; seed < 1000; seed++) {
        for (level = 1; level <= 15; level++) {
            total++;
            Challenge *ch = challenge_generate(seed, level);
            if (!ch) {
                printf("FAIL: seed=%d level=%d challenge_generate returned NULL\n", seed, level);
                failed++;
                continue;
            }

            int ok = 1;

            /* Check target starts with MET, ends with STOP */
            if (strcmp(ch->target[0], "MET") != 0 ||
                strcmp(ch->target[ch->target_count - 1], "STOP") != 0) {
                ok = 0;
                }

            /* Check initial is not already solved */
            if (ok) {
                ExecutionResult er = challenge_execute(ch, ch->initial_tape);
                if (er.diagnostic == DIAG_SUCCESS) ok = 0;
            }

            /* Replay canonical solution */
            if (ok) {
                const LevelDefinition *def = ch->level_def;
                if (def && def->solution_count > 0) {
                    Sequence *replay = sequence_copy(ch->initial_tape);
                    int i;
                    for (i = 0; i < def->solution_count; i++) {
                        Instruction inst = def->solution[i];
                        if (mutation_apply(inst.type, inst.pos1, inst.pos2,
                                           inst.base, replay) != APPLY_SUCCESS) {
                            ok = 0;
                            break;
                                           }
                    }
                    if (ok) {
                        ExecutionResult er = challenge_execute(ch, replay);
                        if (er.diagnostic != DIAG_SUCCESS) ok = 0;
                    }
                    sequence_destroy(replay);
                }
            }

            if (ok) {
                passed++;
            } else {
                printf("FAIL: seed=%d level=%d\n", seed, level);
                failed++;
            }

            challenge_destroy(ch);
        }

        /* Progress indicator every 100 seeds */
        if ((seed + 1) % 100 == 0)
            printf("  Progress: %d/1000 seeds (%d/%d passed)\n", seed + 1, passed, total);
    }

    printf("\nSTRESS TEST RESULT: %d/%d passed (%d failed)\n", passed, total, failed);
    return failed > 0 ? 1 : 0;
}
