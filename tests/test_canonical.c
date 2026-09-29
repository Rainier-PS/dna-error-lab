#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "challenge.h"
#include "execution.h"
#include "codon.h"
#include "sequence.h"
#include "mutation.h"
#include "levels.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT(cond, msg) do { \
    if (cond) { tests_passed++; } \
    else { tests_failed++; printf("  FAIL: %s\n", msg); } \
} while(0)

/* Test the DNA->mRNA->amino acid pipeline for spot-check codons */
static void test_codons(void)
{
    printf("Codon Tests (64 codons)\n");

    /* Test that GENETIC_CODE returns UPPERCASE names */
    const char *r;

    /* MET: TAC -> AUG -> MET */
    r = codon_translate("AUG"); ASSERT(strcmp(r, "MET") == 0, "AUG -> MET");
    ASSERT(codon_is_start("AUG"), "AUG is start");

    /* GLY: CCC -> GGG -> GLY */
    r = codon_translate("GGG"); ASSERT(strcmp(r, "GLY") == 0, "GGG -> GLY");

    /* ASN: AAU -> ASN */
    r = codon_translate("AAU"); ASSERT(strcmp(r, "ASN") == 0, "AAU -> ASN");

    /* STOP: UAA -> STOP */
    r = codon_translate("UAA"); ASSERT(strcmp(r, "STOP") == 0, "UAA -> STOP");
    ASSERT(codon_is_stop("UAA"), "UAA is stop");

    /* STOP: UAG -> STOP */
    r = codon_translate("UAG"); ASSERT(strcmp(r, "STOP") == 0, "UAG -> STOP");
    ASSERT(codon_is_stop("UAG"), "UAG is stop");

    /* STOP: UGA -> STOP */
    r = codon_translate("UGA"); ASSERT(strcmp(r, "STOP") == 0, "UGA -> STOP");
    ASSERT(codon_is_stop("UGA"), "UGA is stop");

    /* TRP: UGG -> TRP */
    r = codon_translate("GGG"); ASSERT(strcmp(r, "GLY") == 0, "GGG -> GLY (2)");

    /* PHE: UUU -> PHE */
    r = codon_translate("UUU"); ASSERT(strcmp(r, "PHE") == 0, "UUU -> PHE");

    /* CYS: UGU -> CYS */
    r = codon_translate("UGU"); ASSERT(strcmp(r, "CYS") == 0, "UGU -> CYS");

    /* PRO: CCC -> PRO */
    r = codon_translate("CCC"); ASSERT(strcmp(r, "PRO") == 0, "CCC -> PRO");

    /* Test AminoAcid enum */
    AminoAcid aa = amino_acid_from_name("MET");
    ASSERT(aa == AA_MET, "amino_acid_from_name MET");
    ASSERT(strcmp(amino_acid_name(AA_STOP), "STOP") == 0, "amino_acid_name STOP");
    ASSERT(strcmp(amino_acid_name(AA_GLY), "GLY") == 0, "amino_acid_name GLY");

    aa = codon_translate_amino("AUG");
    ASSERT(aa == AA_MET, "codon_translate_amino MET");

    printf("  Codon tests: done\n\n");
}

/* Test that a canonical level replays correctly */
static void test_level(int level_num)
{
    const LevelDefinition *lv = level_get(level_num);
    ASSERT(lv != NULL, "level_get returns non-NULL");

    Challenge *ch = challenge_generate(42, level_num);
    if (ch == NULL) {
        printf("  FAIL: challenge_generate returned NULL for level %d\n", level_num);
        tests_failed++;
        return;
    }

    /* Check target starts with MET */
    ASSERT(ch->target_count > 0, "target_count > 0");
    ASSERT(strcmp(ch->target[0], "MET") == 0, "target starts with MET");

    /* Check target ends with STOP */
    ASSERT(strcmp(ch->target[ch->target_count - 1], "STOP") == 0, "target ends with STOP");

    /* Check no premature STOP */
    int no_premature = 1;
    for (int i = 0; i < ch->target_count - 1; i++) {
        if (strcmp(ch->target[i], "STOP") == 0) {
            no_premature = 0;
            break;
        }
    }
    ASSERT(no_premature, "no premature STOP");

    /* Check initial tape is not already solved */
    ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
    ASSERT(init_exec.diagnostic != DIAG_SUCCESS, "initial tape not already solved");

    /* Check final tape translates to target protein */
    ExecutionResult final_exec = challenge_execute(ch, ch->final_tape);
    ASSERT(final_exec.diagnostic == DIAG_SUCCESS, "final tape produces target protein");

    /* Replay solution */
    Sequence *replay = sequence_copy(ch->initial_tape);
    int replay_ok = 1;
    for (int i = 0; i < ch->solution.count; i++) {
        Instruction *inst = &ch->solution.instructions[i];
        if (mutation_apply(inst->type, inst->pos1, inst->pos2,
                           inst->base, replay) != APPLY_SUCCESS) {
            replay_ok = 0;
            break;
                           }
    }
    ASSERT(replay_ok, "replay applies all instructions");
    ASSERT(sequence_equal(replay, ch->final_tape), "replay reaches target DNA");
    sequence_destroy(replay);

    /* Check replay result translates correctly */
    ExecutionResult replay_exec = challenge_execute(ch, ch->final_tape);
    ASSERT(replay_exec.diagnostic == DIAG_SUCCESS, "replay result translates to target");

    /* Check inventory matches solution depth */
    int total_inv = ch->inventory.delete_count + ch->inventory.insert_count +
                    ch->inventory.reverse_count + ch->inventory.swap_count;
    ASSERT(total_inv >= ch->solution.count, "inventory >= solution depth");

    /* Check target DNA length is divisible by 3 */
    ASSERT(ch->final_tape->length % 3 == 0, "target DNA length divisible by 3");

    challenge_destroy(ch);
}

int main(void)
{
    printf("DNA Error Lab - Canonical Level Tests\n\n");

    test_codons();

    for (int level = 1; level <= 15; level++) {
        printf("Level %d\n", level);
        test_level(level);
    }

    printf("\nResults: %d passed, %d failed\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
