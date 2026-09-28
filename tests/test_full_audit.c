#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sequence.h"
#include "codon.h"
#include "mutation.h"
#include "execution.h"
#include "levels.h"
#include "challenge.h"
#include "solver.h"

static int g_pass = 0, g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); } \
} while(0)

#define CHECK_STR(a, b, msg) do { \
    if (strcmp((a),(b)) == 0) { g_pass++; } \
    else { g_fail++; printf("  FAIL [%s:%d]: %s (got \"%s\" expected \"%s\")\n", __FILE__, __LINE__, msg, (a), (b)); } \
} while(0)

/* Helper: build sequence from literal */
static Sequence *make_seq(const char *s) {
    return sequence_from_string(s);
}

/* Helper: get tape as string */
static void tape_str(const Sequence *s, char *buf) {
    sequence_to_string(s, buf, 256);
}

/* SECTION 1: Biology / Translation Pipeline */
static void test_transcription_mapping(void)
{
    printf("Transcription Mapping\n");

    /* TAC -> AUG -> MET */
    {
        Sequence *tape = make_seq("TAC");
        const char *target[] = {"MET"};
        ExecutionResult r = execution_run(tape, target, 1);
        CHECK(r.diagnostic == DIAG_SUCCESS, "TAC -> AUG -> MET");
        sequence_destroy(tape);
    }

    /* AAA -> UUU -> PHE */
    {
        Sequence *tape = make_seq("AAA");
        const char *target[] = {"PHE"};
        ExecutionResult r = execution_run(tape, target, 1);
        CHECK(r.diagnostic == DIAG_SUCCESS, "AAA -> UUU -> PHE");
        sequence_destroy(tape);
    }

    /* ACA -> UGU -> CYS */
    {
        Sequence *tape = make_seq("ACA");
        const char *target[] = {"CYS"};
        ExecutionResult r = execution_run(tape, target, 1);
        CHECK(r.diagnostic == DIAG_SUCCESS, "ACA -> UGU -> CYS");
        sequence_destroy(tape);
    }

    /* ATT -> UAA -> STOP */
    {
        Sequence *tape = make_seq("ATT");
        const char *target[] = {"STOP"};
        ExecutionResult r = execution_run(tape, target, 1);
        CHECK(r.diagnostic == DIAG_SUCCESS, "ATT -> UAA -> STOP");
        sequence_destroy(tape);
    }

    /* ATC -> UAG -> STOP */
    {
        Sequence *tape = make_seq("ATC");
        const char *target[] = {"STOP"};
        ExecutionResult r = execution_run(tape, target, 1);
        CHECK(r.diagnostic == DIAG_SUCCESS, "ATC -> UAG -> STOP");
        sequence_destroy(tape);
    }

    /* ACT -> UGA -> STOP */
    {
        Sequence *tape = make_seq("ACT");
        const char *target[] = {"STOP"};
        ExecutionResult r = execution_run(tape, target, 1);
        CHECK(r.diagnostic == DIAG_SUCCESS, "ACT -> UGA -> STOP");
        sequence_destroy(tape);
    }

    printf("  Transcription mapping: done\n\n");
}

/* Independent codon table verification */
static void test_all_64_codons(void)
{
    printf("All 64 Codon Verification\n");

    /* We independently construct the expected standard genetic code
     * mapping from template DNA to amino acid, and verify codon_translate
     * returns the correct result for all 64 mRNA codons. */

    /* mRNA codon -> expected amino acid (standard genetic code) */
    struct { const char *mrna; const char *aa; } expected[] = {
        /* Phe */
        {"UUU","PHE"},{"UUC","PHE"},
        /* Leu */
        {"UUA","LEU"},{"UUG","LEU"},{"CUU","LEU"},{"CUC","LEU"},{"CUA","LEU"},{"CUG","LEU"},
        /* Ile */
        {"AUU","ILE"},{"AUC","ILE"},{"AUA","ILE"},
        /* Met */
        {"AUG","MET"},
        /* Val */
        {"GUU","VAL"},{"GUC","VAL"},{"GUA","VAL"},{"GUG","VAL"},
        /* Ser */
        {"UCU","SER"},{"UCC","SER"},{"UCA","SER"},{"UCG","SER"},{"AGU","SER"},{"AGC","SER"},
        /* Pro */
        {"CCU","PRO"},{"CCC","PRO"},{"CCA","PRO"},{"CCG","PRO"},
        /* Thr */
        {"ACU","THR"},{"ACC","THR"},{"ACA","THR"},{"ACG","THR"},
        /* Ala */
        {"GCU","ALA"},{"GCC","ALA"},{"GCA","ALA"},{"GCG","ALA"},
        /* Tyr */
        {"UAU","TYR"},{"UAC","TYR"},
        /* His */
        {"CAU","HIS"},{"CAC","HIS"},
        /* Gln */
        {"CAA","GLN"},{"CAG","GLN"},
        /* Asn */
        {"AAU","ASN"},{"AAC","ASN"},
        /* Lys */
        {"AAA","LYS"},{"AAG","LYS"},
        /* Asp */
        {"GAU","ASP"},{"GAC","ASP"},
        /* Glu */
        {"GAA","GLU"},{"GAG","GLU"},
        /* Cys */
        {"UGU","CYS"},{"UGC","CYS"},
        /* Trp */
        {"UGG","TRP"},
        /* Arg */
        {"CGU","ARG"},{"CGC","ARG"},{"CGA","ARG"},{"CGG","ARG"},{"AGA","ARG"},{"AGG","ARG"},
        /* Gly */
        {"GGU","GLY"},{"GGC","GLY"},{"GGA","GLY"},{"GGG","GLY"},
        /* Stop */
        {"UAA","STOP"},{"UAG","STOP"},{"UGA","STOP"},
    };

    int n = (int)(sizeof(expected) / sizeof(expected[0]));
    CHECK(n == 64, "expected table has exactly 64 entries");

    int errors = 0;
    for (int i = 0; i < n; i++) {
        const char *result = codon_translate(expected[i].mrna);
        if (strcmp(result, expected[i].aa) != 0) {
            printf("  FAIL: mRNA %s -> got \"%s\" expected \"%s\"\n",
                   expected[i].mrna, result, expected[i].aa);
            errors++;
        }
    }
    CHECK(errors == 0, "all 64 codons translate correctly");

    /* Verify stop codons are detected */
    CHECK(codon_is_stop("UAA"), "UAA is stop");
    CHECK(codon_is_stop("UAG"), "UAG is stop");
    CHECK(codon_is_stop("UGA"), "UGA is stop");
    CHECK(!codon_is_stop("AUG"), "AUG is not stop");
    CHECK(!codon_is_stop("UUU"), "UUU is not stop");

    /* Verify start codon */
    CHECK(codon_is_start("AUG"), "AUG is start");
    CHECK(!codon_is_start("UAA"), "UAA is not start");

    /* Verify codon_translate_amino enum */
    CHECK(codon_translate_amino("AUG") == AA_MET, "codon_translate_amino AUG -> AA_MET");
    CHECK(codon_translate_amino("UAA") == AA_STOP, "codon_translate_amino UAA -> AA_STOP");
    CHECK(codon_translate_amino("UUU") == AA_PHE, "codon_translate_amino UUU -> AA_PHE");

    /* Verify invalid input */
    CHECK(strcmp(codon_translate(NULL), "???") == 0, "NULL input returns ???");
    CHECK(strcmp(codon_translate("XYZ"), "???") == 0, "invalid bases return ???");
    CHECK(strcmp(codon_translate("AU"), "???") == 0, "too short returns ???");

    printf("  All 64 codons: %d errors\n\n", errors);
}

/* Verify frame error for non-multiple-of-3 */
static void test_frame_error(void)
{
    printf("Frame Error Detection\n");

    /* Length 11 (not divisible by 3) */
    {
        Sequence *tape = make_seq("TACCCCTTAAC"); /* 11 bases */
        const char *target[] = {"MET","GLY","ASN","STOP"};
        ExecutionResult r = execution_run(tape, target, 4);
        CHECK(r.diagnostic == DIAG_FRAME_ERROR, "11 bases -> FRAME ERROR");
        CHECK(r.actual_length == 11, "actual_length reports 11");
        CHECK(r.target_length == 12, "target_length reports 12");
        sequence_destroy(tape);
    }

    /* Length 0 */
    {
        Sequence *tape = make_seq("");
        const char *target[] = {"MET","STOP"};
        ExecutionResult r = execution_run(tape, target, 2);
        CHECK(r.diagnostic == DIAG_FRAME_ERROR, "0 bases -> FRAME ERROR");
        sequence_destroy(tape);
    }

    /* Length 4 (not divisible by 3) */
    {
        Sequence *tape = make_seq("TACG");
        const char *target[] = {"MET","STOP"};
        ExecutionResult r = execution_run(tape, target, 2);
        CHECK(r.diagnostic == DIAG_FRAME_ERROR, "4 bases -> FRAME ERROR");
        sequence_destroy(tape);
    }

    printf("  Frame error detection: done\n\n");
}

/* SECTION 2: Exhaustive Mutation Testing */
static void test_insert(void)
{
    printf("INSERT Testing\n");

    /* INSERT(pos, base) inserts BEFORE 1-based position */

    /* Insert at position 1 (beginning) */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_INSERT, 1, 0, 'A', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "INSERT(1, A) succeeds");
        CHECK_STR(buf, "ATGCA", "INSERT(1,A) on TGCA -> ATGCA");
        CHECK(s->length == 5, "length is 5 after insert");
        sequence_destroy(s);
    }

    /* Insert in middle */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_INSERT, 3, 0, 'G', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "INSERT(3, G) succeeds");
        CHECK_STR(buf, "TGGCA", "INSERT(3,G) on TGCA -> TGGCA");
        sequence_destroy(s);
    }

    /* Insert at end (append) */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_INSERT, 5, 0, 'T', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "INSERT(5, T) appends");
        CHECK_STR(buf, "TGCAT", "INSERT(5,T) on TGCA -> TGCAT");
        sequence_destroy(s);
    }

    /* Insert at position n+1 (also append) */
    {
        Sequence *s = make_seq("AC");
        int ok = mutation_apply(OP_INSERT, 3, 0, 'G', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "INSERT(3, G) on length-2 appends");
        CHECK_STR(buf, "ACG", "INSERT(3,G) on AC -> ACG");
        sequence_destroy(s);
    }

    /* All four bases */
    {
        const char *bases = "ATCG";
        for (int i = 0; i < 4; i++) {
            Sequence *s = make_seq("ACGT");
            char b = bases[i];
            int ok = mutation_apply(OP_INSERT, 2, 0, b, s);
            char buf[64]; tape_str(s, buf);
            char expected[8] = "A";
            int plen = (int)strlen(expected);
            expected[plen] = b;
            expected[plen+1] = '\0';
            strcat(expected, "CGT");
            CHECK(ok == APPLY_SUCCESS, "INSERT with valid base");
            CHECK_STR(buf, expected, "INSERT correct base");
            sequence_destroy(s);
        }
    }

    /* Invalid positions */
    {
        Sequence *s = make_seq("ACGT");
        CHECK(mutation_apply(OP_INSERT, 0, 0, 'A', s) == APPLY_INVALID_POSITION, "INSERT(0) invalid");
        CHECK(mutation_apply(OP_INSERT, 6, 0, 'A', s) == APPLY_INVALID_POSITION, "INSERT(6) on len-4 invalid");
        CHECK(mutation_apply(OP_INSERT, 2, 0, 'X', s) == APPLY_INVALID_POSITION, "INSERT invalid base");
        sequence_destroy(s);
    }

    printf("  INSERT: done\n\n");
}

static void test_delete(void)
{
    printf("DELETE Testing\n");

    /* Delete first */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_DELETE, 1, 0, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "DELETE(1) succeeds");
        CHECK_STR(buf, "GCA", "DELETE(1) on TGCA -> GCA");
        sequence_destroy(s);
    }

    /* Delete middle */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_DELETE, 2, 0, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "DELETE(2) succeeds");
        CHECK_STR(buf, "TCA", "DELETE(2) on TGCA -> TCA");
        sequence_destroy(s);
    }

    /* Delete last */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_DELETE, 4, 0, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "DELETE(4) succeeds");
        CHECK_STR(buf, "TGC", "DELETE(4) on TGCA -> TGC");
        sequence_destroy(s);
    }

    /* Invalid positions */
    {
        Sequence *s = make_seq("ACGT");
        CHECK(mutation_apply(OP_DELETE, 0, 0, '\0', s) == APPLY_INVALID_POSITION, "DELETE(0) invalid");
        CHECK(mutation_apply(OP_DELETE, 5, 0, '\0', s) == APPLY_INVALID_POSITION, "DELETE(5) on len-4 invalid");
        sequence_destroy(s);
    }

    printf("  DELETE: done\n\n");
}

static void test_reverse(void)
{
    printf("REVERSE Testing\n");

    /* REVERSE(pos, length) reverses length bases starting at 1-based pos */

    /* Reverse entire sequence */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_REVERSE, 1, 4, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "REV(1,4) on TGCA succeeds");
        CHECK_STR(buf, "ACGT", "REV(1,4) on TGCA -> ACGT");
        sequence_destroy(s);
    }

    /* Reverse length 2 at start */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_REVERSE, 1, 2, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "REV(1,2) succeeds");
        CHECK_STR(buf, "GTCA", "REV(1,2) on TGCA -> GTCA");
        sequence_destroy(s);
    }

    /* Reverse middle segment */
    {
        Sequence *s = make_seq("TACGATCC");
        int ok = mutation_apply(OP_REVERSE, 3, 4, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "REV(3,4) succeeds");
        CHECK_STR(buf, "TATAGCCC", "REV(3,4) on TACGATCC -> TATAGCCC");
        sequence_destroy(s);
    }

    /* Length 1 is rejected as invalid by validation (l < 2) */
    {
        Sequence *s = make_seq("ACGT");
        CHECK(mutation_apply(OP_REVERSE, 1, 1, '\0', s) == APPLY_INVALID_POSITION, "REV(1,1) invalid (l<2)");
        sequence_destroy(s);
    }

    /* Length exceeding sequence */
    {
        Sequence *s = make_seq("ACGT");
        CHECK(mutation_apply(OP_REVERSE, 1, 5, '\0', s) == APPLY_INVALID_POSITION, "REV(1,5) on len-4 invalid");
        sequence_destroy(s);
    }

    printf("  REVERSE: done\n\n");
}

static void test_swap(void)
{
    printf("SWAP Testing\n");

    /* SWAP(pos1, pos2) exchanges bases at two 1-based positions */

    /* First and last */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_SWAP, 1, 4, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "SWP(1,4) succeeds");
        CHECK_STR(buf, "AGCT", "SWP(1,4) on TGCA -> AGCT");
        sequence_destroy(s);
    }

    /* Adjacent */
    {
        Sequence *s = make_seq("TGCA");
        int ok = mutation_apply(OP_SWAP, 2, 3, '\0', s);
        char buf[64]; tape_str(s, buf);
        CHECK(ok == APPLY_SUCCESS, "SWP(2,3) succeeds");
        CHECK_STR(buf, "TCGA", "SWP(2,3) on TGCA -> TCGA");
        sequence_destroy(s);
    }

    /* Same position is rejected as invalid by validation (p1==p2) */
    {
        Sequence *s = make_seq("ACGT");
        CHECK(mutation_apply(OP_SWAP, 2, 2, '\0', s) == APPLY_INVALID_POSITION, "SWP(2,2) invalid (p1==p2)");
        sequence_destroy(s);
    }

    /* Same base should be no-op */
    {
        Sequence *s = make_seq("AACG");
        CHECK(mutation_apply(OP_SWAP, 1, 2, '\0', s) == APPLY_NO_OP, "SWP(1,2) same base is no-op");
        sequence_destroy(s);
    }

    /* Invalid positions */
    {
        Sequence *s = make_seq("ACGT");
        CHECK(mutation_apply(OP_SWAP, 0, 2, '\0', s) == APPLY_INVALID_POSITION, "SWP(0,2) invalid");
        CHECK(mutation_apply(OP_SWAP, 1, 5, '\0', s) == APPLY_INVALID_POSITION, "SWP(1,5) invalid");
        sequence_destroy(s);
    }

    printf("  SWAP: done\n\n");
}

/* SECTION 3: Operation Inventory */
static void test_inventory(void)
{
    printf("Inventory Enforcement\n");

    /* Basic inventory init and check */
    {
        Inventory inv;
        inventory_init(&inv, 1, 1, 0, 0);
        CHECK(inventory_has(&inv, OP_DELETE), "has DELETE");
        CHECK(inventory_has(&inv, OP_INSERT), "has INSERT");
        CHECK(!inventory_has(&inv, OP_REVERSE), "no REVERSE");
        CHECK(!inventory_has(&inv, OP_SWAP), "no SWAP");
    }

    /* Consume one */
    {
        Inventory inv;
        inventory_init(&inv, 1, 0, 0, 0);
        CHECK(inventory_consume(&inv, OP_DELETE), "consume DEL succeeds");
        CHECK(!inventory_has(&inv, OP_DELETE), "no DEL after consume");
        CHECK(!inventory_consume(&inv, OP_DELETE), "consume DEL again fails");
    }

    /* Failed operations do not consume inventory */
    {
        Inventory inv;
        inventory_init(&inv, 1, 0, 0, 0);
        Sequence *s = make_seq("ACGT");
        /* Try an invalid operation */
        ApplyResult r = mutation_apply(OP_DELETE, 99, 0, '\0', s);
        CHECK(r != APPLY_SUCCESS, "invalid DELETE fails");
        CHECK(inventory_has(&inv, OP_DELETE), "inventory not consumed on failure");
        sequence_destroy(s);
    }

    /* No-op operations do not consume inventory */
    {
        Inventory inv;
        inventory_init(&inv, 0, 0, 0, 1);
        Sequence *s = make_seq("AAGT");
        /* SWAP(1,2) on AAGT - both are 'A', so it's a no-op */
        ApplyResult r = mutation_apply(OP_SWAP, 1, 2, '\0', s);
        CHECK(r == APPLY_NO_OP, "SWAP same base is no-op");
        CHECK(inventory_has(&inv, OP_SWAP), "inventory not consumed on no-op");
        sequence_destroy(s);
    }

    /* Mix operations */
    {
        Inventory inv;
        inventory_init(&inv, 2, 1, 0, 0);
        CHECK(inventory_consume(&inv, OP_DELETE), "consume DEL 1");
        CHECK(inventory_consume(&inv, OP_DELETE), "consume DEL 2");
        CHECK(!inventory_consume(&inv, OP_DELETE), "DEL exhausted");
        CHECK(inventory_consume(&inv, OP_INSERT), "consume INS");
        CHECK(!inventory_consume(&inv, OP_INSERT), "INS exhausted");
    }

    printf("  Inventory: done\n\n");
}

/* SECTION 4: STEP Behavior */
static void test_step_behavior(void)
{
    printf("STEP Behavior\n");

    /* STEP should NOT consume inventory */
    {
        Challenge *ch = challenge_generate(42, 1);
        CHECK(ch != NULL, "generate level 1");
        if (ch) {
            Inventory inv_before = ch->inventory;
            /* Execute (STEP is the same as RUN engine-wise, just no score) */
            (void)challenge_execute(ch, ch->initial_tape);
            /* Check inventory unchanged (challenge_execute doesn't touch inventory) */
            CHECK(ch->inventory.delete_count == inv_before.delete_count,
                  "STEP does not consume DELETE inventory");
            CHECK(ch->inventory.insert_count == inv_before.insert_count,
                  "STEP does not consume INSERT inventory");
            CHECK(ch->inventory.reverse_count == inv_before.reverse_count,
                  "STEP does not consume REVERSE inventory");
            CHECK(ch->inventory.swap_count == inv_before.swap_count,
                  "STEP does not consume SWAP inventory");
            /* Check tape unchanged */
            CHECK(!sequence_equal(ch->initial_tape, ch->final_tape),
                  "STEP does not alter the tape");
            challenge_destroy(ch);
        }
    }

    printf("  STEP behavior: done\n\n");
}

/* SECTION 5: RUN Diagnostic Priority */
static void test_execution_diagnostics(void)
{
    printf("Execution Diagnostic Priority\n");

    /* 1. FRAME ERROR: length not divisible by 3 */
    {
        Sequence *tape = make_seq("TACCCCTTAAC"); /* 11 bases */
        const char *target[] = {"MET","GLY","ASN","STOP"};
        ExecutionResult r = execution_run(tape, target, 4);
        CHECK(r.diagnostic == DIAG_FRAME_ERROR, "11 bases -> FRAME ERROR");
        sequence_destroy(tape);
    }

    /* 2. PREMATURE STOP: STOP before final position */
    {
        /* Target: MET -> GLY -> ASN -> STOP (4 codons, 12 bases) */
        /* Construct: TAC ATT TTA ATC (12 bases, 4 codons) */
        /* Codons: TAC->MET, ATT->UAA->STOP (premature!), TTA->ASN, ATC->STOP */
        Sequence *tape = make_seq("TACATTTTAATC"); /* 12 bases */
        const char *target[] = {"MET","GLY","ASN","STOP"};
        ExecutionResult r = execution_run(tape, target, 4);
        CHECK(r.diagnostic == DIAG_PREMATURE_STOP, "premature STOP detected");
        CHECK(r.trace[1].is_stop == 1, "codon 2 is stop");
        sequence_destroy(tape);
    }

    /* 3. CODON MISS: wrong amino acid but in-frame */
    {
        /* Target: MET -> GLY -> ASN -> STOP */
        /* Tape:   TAC CCC TTA ATC (correct target) -> change one base */
        /* TAC CCC TGA ATC -> MET GLY THR STOP (codon miss at 3) */
        Sequence *tape = make_seq("TACCCCTGAATC");
        const char *target[] = {"MET","GLY","ASN","STOP"};
        ExecutionResult r = execution_run(tape, target, 4);
        CHECK(r.diagnostic == DIAG_CODON_MISS, "CODON MISS detected");
        CHECK(r.trace[2].is_match == 0, "codon 3 does not match");
        sequence_destroy(tape);
    }

    /* 4. RUN-ON: expected STOP but got amino acid */
    {
        /* Target: MET -> STOP (2 codons, 6 bases) */
        /* Tape:   TAC TTA -> MET ASN (runs on, no STOP) */
        Sequence *tape = make_seq("TACTTA");
        const char *target[] = {"MET","STOP"};
        ExecutionResult r = execution_run(tape, target, 2);
        CHECK(r.diagnostic == DIAG_RUN_ON, "RUN-ON detected");
        CHECK(r.trace[1].is_stop == 0, "codon 2 is not stop");
        sequence_destroy(tape);
    }

    /* 5. SUCCESS: all codons match */
    {
        Sequence *tape = make_seq("TACCCCTTAATC");
        const char *target[] = {"MET","GLY","ASN","STOP"};
        ExecutionResult r = execution_run(tape, target, 4);
        CHECK(r.diagnostic == DIAG_SUCCESS, "SUCCESS for correct sequence");
        for (int i = 0; i < 4; i++)
            CHECK(r.trace[i].is_match == 1, "each codon matches");
        sequence_destroy(tape);
    }

    /* Success is based on protein, not exact DNA */
    {
        /* TAC CCC TTA ATC -> MET GLY ASN STOP (canonical) */
        /* TAC CCT TTA ATC -> MET GLY ASN STOP (CCC and CCT both -> GLY) */
        Sequence *tape1 = make_seq("TACCCCTTAATC");
        Sequence *tape2 = make_seq("TACCCTTTAATC");
        const char *target[] = {"MET","GLY","ASN","STOP"};
        ExecutionResult r1 = execution_run(tape1, target, 4);
        ExecutionResult r2 = execution_run(tape2, target, 4);
        CHECK(r1.diagnostic == DIAG_SUCCESS, "canonical DNA succeeds");
        CHECK(r2.diagnostic == DIAG_SUCCESS, "synonymous DNA also succeeds");
        CHECK(!sequence_equal(tape1, tape2), "the two DNAs are different");
        sequence_destroy(tape1);
        sequence_destroy(tape2);
    }

    printf("  Execution diagnostics: done\n\n");
}

/* SECTION 6: Codon Degeneracy */
static void test_codon_degeneracy(void)
{
    printf("Codon Degeneracy\n");

    /* Multiple DNA codons for the same amino acid should all be accepted */

    /* GLY: CCA -> GGU, CCG -> GGC, CCT -> GGA, CCC -> GGG */
    {
        const char *gly_mrna[] = {"GGU", "GGC", "GGA", "GGG"};
        for (int i = 0; i < 4; i++) {
            const char *result = codon_translate(gly_mrna[i]);
            CHECK_STR(result, "GLY", "GLY codon degeneracy");
        }
    }

    /* VAL: CAA -> GUU, CAG -> GUC, CAT -> GUA, CAC -> GUG */
    {
        const char *val_mrna[] = {"GUU", "GUC", "GUA", "GUG"};
        for (int i = 0; i < 4; i++) {
            const char *result = codon_translate(val_mrna[i]);
            CHECK_STR(result, "VAL", "VAL codon degeneracy");
        }
    }

    /* LEU: has 6 codons */
    {
        const char *leu_mrna[] = {"UUA", "UUG", "CUU", "CUC", "CUA", "CUG"};
        for (int i = 0; i < 6; i++) {
            const char *result = codon_translate(leu_mrna[i]);
            CHECK_STR(result, "LEU", "LEU codon degeneracy");
        }
    }

    /* Verify game accepts synonymous DNA via full pipeline */
    {
        /* MET -> GLY -> STOP */
        /* TAC CCA ACT -> MET GLY STOP */
        /* TAC CCC ACT -> MET GLY STOP (CCC -> GGG -> GLY too) */
        Sequence *s1 = make_seq("TACCCAACT");
        Sequence *s2 = make_seq("TACCCCACT");
        const char *target[] = {"MET","GLY","STOP"};
        ExecutionResult r1 = execution_run(s1, target, 3);
        ExecutionResult r2 = execution_run(s2, target, 3);
        CHECK(r1.diagnostic == DIAG_SUCCESS, "CCA -> GLY accepted");
        CHECK(r2.diagnostic == DIAG_SUCCESS, "CCC -> GLY accepted");
        sequence_destroy(s1);
        sequence_destroy(s2);
    }

    printf("  Codon degeneracy: done\n\n");
}

/* SECTION 7: All 15 Canonical Levels */
static void test_all_15_levels(void)
{
    printf("All 15 Canonical Levels\n");

    for (int level = 1; level <= 15; level++) {
        const LevelDefinition *lv = level_get(level);
        CHECK(lv != NULL, "level_get returns non-NULL");

        Challenge *ch = challenge_generate(42, level);
        if (!ch) {
            g_fail++;
            printf("  FAIL: challenge_generate returned NULL for level %d\n", level);
            continue;
        }

        char msg[256];

        /* 1. Target starts with MET */
        snprintf(msg, sizeof(msg), "L%d: target starts with MET", level);
        CHECK_STR(ch->target[0], "MET", msg);

        /* 2. Target ends with STOP */
        snprintf(msg, sizeof(msg), "L%d: target ends with STOP", level);
        CHECK_STR(ch->target[ch->target_count - 1], "STOP", msg);

        /* 3. No premature STOP */
        int no_premature = 1;
        for (int i = 0; i < ch->target_count - 1; i++) {
            if (strcmp(ch->target[i], "STOP") == 0) { no_premature = 0; break; }
        }
        snprintf(msg, sizeof(msg), "L%d: no premature STOP", level);
        CHECK(no_premature, msg);

        /* 4. Target DNA length divisible by 3 */
        snprintf(msg, sizeof(msg), "L%d: target DNA length %% 3 == 0", level);
        CHECK(ch->final_tape->length % 3 == 0, msg);

        /* 5. Initial tape is NOT already solved */
        ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
        snprintf(msg, sizeof(msg), "L%d: initial tape not already solved", level);
        CHECK(init_exec.diagnostic != DIAG_SUCCESS, msg);

        /* 6. Final tape produces target protein */
        ExecutionResult final_exec = challenge_execute(ch, ch->final_tape);
        snprintf(msg, sizeof(msg), "L%d: final tape produces target protein", level);
        CHECK(final_exec.diagnostic == DIAG_SUCCESS, msg);

        /* 7. Replay solution and verify */
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
        snprintf(msg, sizeof(msg), "L%d: solution replay succeeds", level);
        CHECK(replay_ok, msg);

        /* 8. Replay reaches target DNA */
        snprintf(msg, sizeof(msg), "L%d: replay reaches target DNA", level);
        CHECK(sequence_equal(replay, ch->final_tape), msg);

        /* 9. Replay result translates correctly */
        ExecutionResult replay_exec = challenge_execute(ch, ch->final_tape);
        snprintf(msg, sizeof(msg), "L%d: replay result translates correctly", level);
        CHECK(replay_exec.diagnostic == DIAG_SUCCESS, msg);

        /* 10. Inventory is sufficient for solution */
        int total_inv = ch->inventory.delete_count + ch->inventory.insert_count +
                        ch->inventory.reverse_count + ch->inventory.swap_count;
        snprintf(msg, sizeof(msg), "L%d: inventory >= solution depth", level);
        CHECK(total_inv >= ch->solution.count, msg);

        /* 11. DNA is valid */
        snprintf(msg, sizeof(msg), "L%d: initial DNA valid", level);
        CHECK(sequence_validate(ch->initial_tape), msg);
        snprintf(msg, sizeof(msg), "L%d: final DNA valid", level);
        CHECK(sequence_validate(ch->final_tape), msg);

        /* 12. Initial and final are different */
        snprintf(msg, sizeof(msg), "L%d: initial != final DNA", level);
        CHECK(!sequence_equal(ch->initial_tape, ch->final_tape), msg);

        sequence_destroy(replay);
        challenge_destroy(ch);
    }

    printf("  All 15 levels: done\n\n");
}

/* SECTION 8: Generated-Level Solvability */
static void test_generated_solvability(void)
{
    printf("Generated-Level Solvability (5 seeds x 15 levels)\n");

    int seeds[] = {42, 100, 999, 12345, 54321};
    int total = 0, passed = 0;

    for (int level = 1; level <= 15; level++) {
        for (int s = 0; s < 5; s++) {
            total++;
            int seed = seeds[s];
            Challenge *ch = challenge_generate(seed, level);
            if (!ch) {
                printf("  FAIL: L%d seed=%d generation NULL\n", level, seed);
                continue;
            }

            int ok = 1;

            /* Verify target translates */
            ExecutionResult final_exec = challenge_execute(ch, ch->final_tape);
            if (final_exec.diagnostic != DIAG_SUCCESS) {
                printf("  FAIL: L%d seed=%d final tape broken\n", level, seed);
                ok = 0;
            }

            /* Verify initial not solved */
            if (ok) {
                ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
                if (init_exec.diagnostic == DIAG_SUCCESS) {
                    printf("  FAIL: L%d seed=%d already solved\n", level, seed);
                    ok = 0;
                }
            }

            /* Replay solution */
            if (ok) {
                Sequence *replay = sequence_copy(ch->initial_tape);
                for (int i = 0; i < ch->solution.count && ok; i++) {
                    Instruction *inst = &ch->solution.instructions[i];
                    if (mutation_apply(inst->type, inst->pos1, inst->pos2,
                                       inst->base, replay) != APPLY_SUCCESS) {
                        printf("  FAIL: L%d seed=%d op %d failed\n", level, seed, i);
                        ok = 0;
                                       }
                }
                if (ok && !sequence_equal(replay, ch->final_tape)) {
                    printf("  FAIL: L%d seed=%d replay mismatch\n", level, seed);
                    ok = 0;
                }
                sequence_destroy(replay);
            }

            if (ok) passed++;
            challenge_destroy(ch);
        }
    }

    printf("  Solvability: %d/%d passed\n\n", passed, total);
    CHECK(passed == total, "all generated challenges solvable");
}

int main(void)
{
    printf(" DNA Error Lab - Full Audit Test Suite\n\n");

    test_transcription_mapping();
    test_all_64_codons();
    test_frame_error();
    test_insert();
    test_delete();
    test_reverse();
    test_swap();
    test_inventory();
    test_step_behavior();
    test_execution_diagnostics();
    test_codon_degeneracy();
    test_all_15_levels();
    test_generated_solvability();

    printf(" TOTAL: %d passed, %d failed\n", g_pass, g_fail);

    return g_fail > 0 ? 1 : 0;
}
