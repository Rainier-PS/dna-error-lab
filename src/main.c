#include <stdio.h>
#include <string.h>
#include "game.h"
#include "tui.h"
#include "levels.h"
#include "sequence.h"
#include "mutation.h"
#include "execution.h"
#include "codon.h"

static void dev_mode(void)
{
    int pass_count = 0;
    int total = 15;
    int i;

    printf("Developer mode activated.\n");

    for (i = 1; i <= total; i++)
    {
        const LevelDefinition *def = level_get(i);
        Challenge *ch;
        int ok = 1;
        char fail_msg[256] = "";

        printf("Level %02d\n", i);

        if (!def)
        {
            printf("Fail: level_get(%d) returned NULL\n", i);
            ok = 0;
        }
        else
        {
            printf("Title: %s\n", def->objective);
            printf("Target: %s\n", def->target_dna ? def->target_dna : "(null)");
            printf("Initial: %s\n", def->initial_dna ? def->initial_dna : "(null)");
            printf("Inv: D%d I%d R%d S%d\n", def->delete_budget, def->insert_budget, def->reverse_budget, def->swap_budget);
            printf("Depth: %d\n", def->intended_depth);
            printf("Steps: %d\n", def->solution_count);
        }
        
        if (ok)
        {
            // Generate through the runtime loader
            ch = challenge_generate(0, i);
            if (!ch)
            {
                sprintf(fail_msg, "challenge_generate returned NULL");
                ok = 0;
            }
            else
            {
                // Check initial matches level data
                if (def->initial_dna && strcmp(ch->initial_tape->data, def->initial_dna) != 0)
                {
                    sprintf(fail_msg, "Initial tape mismatch: runtime=%s, canonical=%s", ch->initial_tape->data, def->initial_dna);
                    ok = 0;
                }

                // Check target protein
                if (ok)
                {
                    printf("Protein: ");
                    { int j; for(j = 0; j < ch->target_count; j++)
                        printf("%s%s", j > 0 ? "->" : "", ch->target[j]);}
                    printf("\n");
                    
                    // Verify target starts with MET, ends with STOP codon
                    if (strcmp(ch->target[0], "MET") != 0)
                    {
                        sprintf(fail_msg, "Target does not start with MET");
                        ok = 0;
                    }
                    else if (strcmp(ch->target[ch->target_count - 1], "STOP") != 0)
                    {
                        sprintf(fail_msg, "target does not end with STOP");
                        ok = 0;
                    }
                }
                
                // Check initial is not already solved
                if (ok)
                {
                    ExecutionResult init_er = challenge_execute(ch, ch->initial_tape);
                    if (init_er.diagnostic == DIAG_SUCCESS)
                    {
                        sprintf(fail_msg, "Initial tape is already solved");
                        ok = 0;
                    }
                }

                // Replay canonical solution
                if (ok && def->solution_count > 0)
                {
                    Sequence *replay = sequence_copy(ch->initial_tape);
                    int j;
                    int replay_ok = 1;
                    for (j = 0; j< def->solution_count; j++)
                    {
                        Instruction inst = def->solution[j];
                        if (mutation_apply(inst.type, inst.pos1, inst.pos2, inst.base, replay) != APPLY_SUCCESS)
                        {
                            char ibuf[64];
                            mutation_format_instruction(&inst, ibuf, sizeof(ibuf));
                            sprintf(fail_msg, "solution step %d failed: %s", j + 1, ibuf);
                            replay_ok = 0;
                            break;
                        }
                    }
                    if (replay_ok)
                    {
                        ExecutionResult final_er = challenge_execute(ch, replay);
                        if (final_er.diagnostic != DIAG_SUCCESS)
                        {
                            sprintf(fail_msg, "solution result is %s (expected SUCCESS)", execution_diagnostic_name(final_er.diagnostic));
                            ok = 0;
                        }
                    }
                    sequence_destroy(replay);
                }

                // Check inventory matches solution
                if (ok)
                {
                    int sol_del = 0, sol_ins = 0, sol_rev = 0, sol_swp = 0;
                    int j;
                    for (j = 0; j < def->solution_count; j++)
                    {
                        switch (def->solution[j].type)
                        {
                            case OP_DELETE: sol_del++; break;
                            case OP_INSERT: sol_ins++; break;
                            case OP_REVERSE: sol_rev++; break;
                            case OP_SWAP: sol_swp++; break;
                            default: break;
                        }
                    }
                    if (sol_del > ch->inventory.delete_count ||
                        sol_ins > ch->inventory.insert_count ||
                        sol_rev > ch->inventory.reverse_count ||
                        sol_swp > ch->inventory.swap_count)
                    {
                        sprintf(fail_msg, "Inventory insufficient for solution");
                        ok = 0;
                    }
                }

                // Check reproducibility
                if (ok)
                {
                    Challenge *ch2 = challenge_generate(0, i);
                    if (!ch2 || strcmp(ch->initial_tape->data, ch2->initial_tape->data) != 0)
                    {
                        sprintf(fail_msg, "not reproducible with the same seed");
                        ok = 0;
                    }
                    if (ch2) challenge_destroy(ch2);
                }

                challenge_destroy(ch);
            }
        }

        if (ok)
        {
            printf("PASS\n");
            pass_count++;
        }
        else
        {
            printf("Status: FAIL\n");
            printf("Reason: %s\n", fail_msg);
        }
        printf("\n");
    }

    printf("RUNTIME LEVEL AUDIT\n");
    for (i = 1; i <= total; i++)
        printf("Level %02d %s\n", i, i<= pass_count ? "PASS" : "FAIL");
    printf("\n%d/%d runtime levels valid\n", pass_count, total);
}

int main(int argc, char *argv[])
{
    // Check for developer mode
    if (argc > 1 && (strcmp(argv[1], "--dev") == 0 || strcmp(argv[1], "--developer") == 0))
    {
        dev_mode();
        return 0;
    }

    {
        GameState state;
        game_init(&state);
        tui_init();
        tui_run(&state);
        tui_shutdown();
        game_destroy(&state);

        return 0;
    }
}