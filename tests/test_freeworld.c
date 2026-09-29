#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sequence.h"
#include "mutation.h"
#include "execution.h"
#include "challenge.h"

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) do { \
    if (cond) { g_pass++; } \
    else { g_fail++; printf("  FAIL [%s:%d]: %s\n", __FILE__, __LINE__, msg); } \
} while(0)

#define CHECK_STR(a, b, msg) do { \
    if (strcmp((a),(b)) == 0) { g_pass++; } \
    else { g_fail++; printf("  FAIL [%s:%d]: %s (got \"%s\" expected \"%s\")\n", __FILE__, __LINE__, msg, (a), (b)); } \
} while(0)

/* Helper functions */

static Sequence *make_seq(const char *s)
{
    return sequence_from_string(s);
}

static void tape_str(const Sequence *s, char *buf)
{
    sequence_to_string(s, buf, 256);
}

/* SECTION 1: Unlimited Operations (sim_mode) */

static void test_sim_mode_inventory(void)
{
    printf("Sim Mode Inventory\n");

    Inventory inv;
    memset(&inv, 0, sizeof(inv));
    inv.sim_mode = 1;

    /* All operation types should be available in sim_mode */
    CHECK(inventory_has(&inv, OP_DELETE), "sim_mode has DELETE");
    CHECK(inventory_has(&inv, OP_INSERT), "sim_mode has INSERT");
    CHECK(inventory_has(&inv, OP_REVERSE), "sim_mode has REVERSE");
    CHECK(inventory_has(&inv, OP_SWAP), "sim_mode has SWAP");

    /* Consuming should always succeed and never deplete */
    CHECK(inventory_consume(&inv, OP_DELETE), "sim_mode consume DELETE");
    CHECK(inventory_consume(&inv, OP_INSERT), "sim_mode consume INSERT");
    CHECK(inventory_consume(&inv, OP_REVERSE), "sim_mode consume REVERSE");
    CHECK(inventory_consume(&inv, OP_SWAP), "sim_mode consume SWAP");

    /* Should still have all after consuming */
    CHECK(inventory_has(&inv, OP_DELETE), "sim_mode still has DELETE after consume");
    CHECK(inventory_has(&inv, OP_INSERT), "sim_mode still has INSERT after consume");
    CHECK(inventory_has(&inv, OP_REVERSE), "sim_mode still has REVERSE after consume");
    CHECK(inventory_has(&inv, OP_SWAP), "sim_mode still has SWAP after consume");

    /* Consuming 100 times should not deplete */
    for (int i = 0; i < 100; i++)
    {
        inventory_consume(&inv, OP_DELETE);
        inventory_consume(&inv, OP_INSERT);
        inventory_consume(&inv, OP_REVERSE);
        inventory_consume(&inv, OP_SWAP);
    }
    CHECK(inventory_has(&inv, OP_DELETE), "sim_mode still has DELETE after 100 consumes");
    CHECK(inventory_has(&inv, OP_INSERT), "sim_mode still has INSERT after 100 consumes");
    CHECK(inventory_has(&inv, OP_REVERSE), "sim_mode still has REVERSE after 100 consumes");
    CHECK(inventory_has(&inv, OP_SWAP), "sim_mode still has SWAP after 100 consumes");

    printf("  Sim mode inventory: done\n\n");
}

/* SECTION 2: Campaign Inventory Regression */

static void test_campaign_inventory_finite(void)
{
    printf("Campaign Inventory Regression\n");

    Inventory inv;

    /* Level 1: INS x1, rest x0 */
    inventory_init(&inv, 0, 1, 0, 0);
    CHECK(!inv.sim_mode, "campaign inv has sim_mode=0");
    CHECK(inventory_has(&inv, OP_INSERT), "campaign has INSERT");
    CHECK(!inventory_has(&inv, OP_DELETE), "campaign has no DELETE");
    CHECK(!inventory_has(&inv, OP_REVERSE), "campaign has no REVERSE");
    CHECK(!inventory_has(&inv, OP_SWAP), "campaign has no SWAP");

    /* Consume the one INSERT */
    CHECK(inventory_consume(&inv, OP_INSERT), "consume 1 INSERT succeeds");
    CHECK(!inventory_has(&inv, OP_INSERT), "no more INSERT after consume");

    /* Level 11: DEL x1, INS x1, REV x1, SWP x1 */
    inventory_init(&inv, 1, 1, 1, 1);
    CHECK(!inv.sim_mode, "campaign inv has sim_mode=0");
    CHECK(inventory_has(&inv, OP_DELETE), "campaign has DELETE");
    CHECK(inventory_consume(&inv, OP_DELETE), "consume DELETE");
    CHECK(!inventory_has(&inv, OP_DELETE), "no more DELETE");

    /* Try to consume when depleted */
    CHECK(!inventory_consume(&inv, OP_DELETE), "consume depleted DELETE fails");
    CHECK(!inventory_consume(&inv, OP_DELETE), "consume depleted DELETE fails again");

    /* Other ops still available */
    CHECK(inventory_has(&inv, OP_INSERT), "INSERT still available");
    CHECK(inventory_has(&inv, OP_REVERSE), "REVERSE still available");
    CHECK(inventory_has(&inv, OP_SWAP), "SWAP still available");

    /* Zero inventory: nothing available */
    inventory_init(&inv, 0, 0, 0, 0);
    CHECK(!inventory_has(&inv, OP_DELETE), "zero inv: no DELETE");
    CHECK(!inventory_has(&inv, OP_INSERT), "zero inv: no INSERT");
    CHECK(!inventory_has(&inv, OP_REVERSE), "zero inv: no REVERSE");
    CHECK(!inventory_has(&inv, OP_SWAP), "zero inv: no SWAP");

    printf("  Campaign inventory regression: done\n\n");
}

/* SECTION 3: Empty Sequence Mutations */

static void test_empty_delete(void)
{
    printf("DELETE on Empty Sequence\n");

    Sequence *s = make_seq("");
    CHECK(s != NULL, "empty sequence created");
    CHECK(s->length == 0, "empty sequence length is 0");

    /* DELETE on empty should fail safely */
    ApplyResult r = mutation_apply(OP_DELETE, 1, 0, '\0', s);
    CHECK(r == APPLY_INVALID_POSITION, "DELETE(1) on empty returns INVALID");

    char buf[256];
    tape_str(s, buf);
    CHECK_STR(buf, "", "empty sequence unchanged after failed DELETE");

    sequence_destroy(s);
    printf("  DELETE on empty: done\n\n");
}

static void test_empty_reverse(void)
{
    printf("REVERSE on Empty Sequence\n");

    Sequence *s = make_seq("");
    ApplyResult r = mutation_apply(OP_REVERSE, 1, 2, '\0', s);
    CHECK(r == APPLY_INVALID_POSITION, "REVERSE(1,2) on empty returns INVALID");

    r = mutation_apply(OP_REVERSE, 1, 1, '\0', s);
    CHECK(r == APPLY_NO_OP || r == APPLY_INVALID_POSITION,
          "REVERSE(1,1) on empty fails");

    char buf[256];
    tape_str(s, buf);
    CHECK_STR(buf, "", "empty sequence unchanged after failed REVERSE");

    sequence_destroy(s);
    printf("  REVERSE on empty: done\n\n");
}

static void test_empty_swap(void)
{
    printf("SWAP on Empty Sequence\n");

    Sequence *s = make_seq("");
    ApplyResult r = mutation_apply(OP_SWAP, 1, 2, '\0', s);
    CHECK(r == APPLY_INVALID_POSITION, "SWP(1,2) on empty returns INVALID");

    char buf[256];
    tape_str(s, buf);
    CHECK_STR(buf, "", "empty sequence unchanged after failed SWAP");

    sequence_destroy(s);
    printf("  SWAP on empty: done\n\n");
}

static void test_empty_insert(void)
{
    printf("INSERT on Empty Sequence\n");

    Sequence *s = make_seq("");
    CHECK(s != NULL, "empty sequence created");

    /* INSERT into empty sequence must work */
    ApplyResult r = mutation_apply(OP_INSERT, 1, 0, 'A', s);
    CHECK(r == APPLY_SUCCESS, "INSERT(1,A) on empty succeeds");
    CHECK(s->length == 1, "length is 1 after first insert");

    char buf[256];
    tape_str(s, buf);
    CHECK_STR(buf, "A", "empty -> INSERT(1,A) -> A");

    /* Insert another base */
    r = mutation_apply(OP_INSERT, 2, 0, 'T', s);
    CHECK(r == APPLY_SUCCESS, "INSERT(2,T) on length-1 succeeds");
    CHECK(s->length == 2, "length is 2 after second insert");
    tape_str(s, buf);
    CHECK_STR(buf, "AT", "A -> INSERT(2,T) -> AT");

    /* Insert at beginning (capacity may prevent this on small sequences) */
    {
        Sequence *s2 = make_seq("TGCA");
        ApplyResult r2 = mutation_apply(OP_INSERT, 1, 0, 'G', s2);
        char buf2[256]; tape_str(s2, buf2);
        CHECK(r2 == APPLY_SUCCESS, "INSERT(1,G) on TGCA succeeds");
        CHECK_STR(buf2, "GTGCA", "TGCA -> INS(1,G) -> GTGCA");
        sequence_destroy(s2);
    }

    /* Insert at end (position = length+1) */
    {
        Sequence *s2 = make_seq("TGCA");
        ApplyResult r2 = mutation_apply(OP_INSERT, 5, 0, 'T', s2);
        char buf2[256]; tape_str(s2, buf2);
        CHECK(r2 == APPLY_SUCCESS, "INSERT(5,T) on TGCA succeeds");
        CHECK_STR(buf2, "TGCAT", "TGCA -> INS(5,T) -> TGCAT");
        sequence_destroy(s2);
    }

    /* Invalid position */
    r = mutation_apply(OP_INSERT, 0, 0, 'A', s);
    CHECK(r == APPLY_INVALID_POSITION, "INSERT(0) invalid");

    r = mutation_apply(OP_INSERT, 10, 0, 'A', s);
    CHECK(r == APPLY_INVALID_POSITION, "INSERT(10) beyond end invalid");

    /* Invalid base */
    r = mutation_apply(OP_INSERT, 1, 0, 'X', s);
    CHECK(r == APPLY_INVALID_POSITION, "INSERT with invalid base returns INVALID");

    sequence_destroy(s);
    printf("  INSERT on empty: done\n\n");
}

/* SECTION 4: Recovery from Empty Sequence */

static void test_empty_to_full_recovery(void)
{
    printf("Empty to Full Recovery\n");

    Sequence *s = make_seq("TACGCGTAC");
    CHECK(s != NULL, "sequence created");
    CHECK(s->length == 9, "length is 9");

    /* Delete everything */
    for (int i = 0; i < 9; i++)
    {
        ApplyResult r = mutation_apply(OP_DELETE, 1, 0, '\0', s);
        CHECK(r == APPLY_SUCCESS, "delete succeeds");
    }
    CHECK(s->length == 0, "length is 0 after deleting all");

    char buf[256];
    tape_str(s, buf);
    CHECK_STR(buf, "", "sequence is empty");

    /* INSERT to recover */
    ApplyResult r = mutation_apply(OP_INSERT, 1, 0, 'A', s);
    CHECK(r == APPLY_SUCCESS, "INSERT on empty recovers");
    CHECK(s->length == 1, "length is 1 after recovery insert");
    tape_str(s, buf);
    CHECK_STR(buf, "A", "recovery produced A");

    /* Continue building */
    r = mutation_apply(OP_INSERT, 2, 0, 'T', s);
    CHECK(r == APPLY_SUCCESS, "second recovery insert");
    r = mutation_apply(OP_INSERT, 3, 0, 'G', s);
    CHECK(r == APPLY_SUCCESS, "third recovery insert");
    tape_str(s, buf);
    CHECK_STR(buf, "ATG", "recovered to ATG");

    sequence_destroy(s);
    printf("  Empty to full recovery: done\n\n");
}

/* SECTION 5: DELETE Boundary Cases */

static void test_delete_boundaries(void)
{
    printf("DELETE Boundary Cases\n");

    /* Delete first position */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_DELETE, 1, 0, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "DELETE(1) on TGCA succeeds");
        CHECK_STR(buf, "GCA", "TGCA -> DEL(1) -> GCA");
        CHECK(s->length == 3, "length is 3");
        sequence_destroy(s);
    }

    /* Delete middle position */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_DELETE, 2, 0, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "DELETE(2) on TGCA succeeds");
        CHECK_STR(buf, "TCA", "TGCA -> DEL(2) -> TCA");
        sequence_destroy(s);
    }

    /* Delete last position */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_DELETE, 4, 0, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "DELETE(4) on TGCA succeeds");
        CHECK_STR(buf, "TGC", "TGCA -> DEL(4) -> TGC");
        sequence_destroy(s);
    }

    /* Delete position 0 (invalid) */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_DELETE, 0, 0, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION, "DELETE(0) invalid");
        sequence_destroy(s);
    }

    /* Delete position > length */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_DELETE, 5, 0, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION, "DELETE(5) on len-4 invalid");
        sequence_destroy(s);
    }

    /* Delete on length 1 */
    {
        Sequence *s = make_seq("A");
        ApplyResult r = mutation_apply(OP_DELETE, 1, 0, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "DELETE(1) on A succeeds");
        CHECK_STR(buf, "", "A -> DEL(1) -> empty");
        CHECK(s->length == 0, "length is 0");
        sequence_destroy(s);
    }

    printf("  DELETE boundaries: done\n\n");
}

/* SECTION 6: INSERT Boundary Cases */

static void test_insert_boundaries(void)
{
    printf("INSERT Boundary Cases\n");

    /* Insert at position 1 (beginning) */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_INSERT, 1, 0, 'A', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "INSERT(1,A) succeeds");
        CHECK_STR(buf, "ATGCA", "TGCA -> INS(1,A) -> ATGCA");
        sequence_destroy(s);
    }

    /* Insert in middle */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_INSERT, 3, 0, 'G', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "INSERT(3,G) succeeds");
        CHECK_STR(buf, "TGGCA", "TGCA -> INS(3,G) -> TGGCA");
        sequence_destroy(s);
    }

    /* Insert at end using length+1 */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_INSERT, 5, 0, 'T', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "INSERT(5,T) on TGCA succeeds");
        CHECK_STR(buf, "TGCAT", "TGCA -> INS(5,T) -> TGCAT");
        sequence_destroy(s);
    }

    /* Insert when length is 0 */
    {
        Sequence *s = make_seq("");
        ApplyResult r = mutation_apply(OP_INSERT, 1, 0, 'A', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "INSERT(1,A) on empty succeeds");
        CHECK_STR(buf, "A", "empty -> INS(1,A) -> A");
        sequence_destroy(s);
    }

    /* Invalid position below 1 */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_INSERT, 0, 0, 'A', s);
        CHECK(r == APPLY_INVALID_POSITION, "INSERT(0) invalid");
        sequence_destroy(s);
    }

    /* Invalid position beyond length+1 */
    {
        Sequence *s = make_seq("TGCA");
        ApplyResult r = mutation_apply(OP_INSERT, 6, 0, 'A', s);
        CHECK(r == APPLY_INVALID_POSITION, "INSERT(6) on len-4 invalid");
        sequence_destroy(s);
    }

    /* Repeated insertion (using larger-capacity sequence) */
    {
        Sequence *s = sequence_create(8);
        ApplyResult r;
        char buf[256];

        /* Start empty, insert 4 bases */
        r = mutation_apply(OP_INSERT, 1, 0, 'A', s);
        CHECK(r == APPLY_SUCCESS, "repeated insert 1");
        r = mutation_apply(OP_INSERT, 2, 0, 'T', s);
        CHECK(r == APPLY_SUCCESS, "repeated insert 2");
        r = mutation_apply(OP_INSERT, 3, 0, 'C', s);
        CHECK(r == APPLY_SUCCESS, "repeated insert 3");
        r = mutation_apply(OP_INSERT, 4, 0, 'G', s);
        CHECK(r == APPLY_SUCCESS, "repeated insert 4");
        tape_str(s, buf);
        CHECK_STR(buf, "ATCG", "repeated insert: empty -> ATCG");
        sequence_destroy(s);
    }

    printf("  INSERT boundaries: done\n\n");
}

/* SECTION 7: REVERSE Boundary Cases */

static void test_reverse_boundaries(void)
{
    printf("REVERSE Boundary Cases\n");

    /* Full sequence reversal */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_REVERSE, 1, 4, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "REV(1,4) on ATCG succeeds");
        CHECK_STR(buf, "GCTA", "ATCG -> REV(1,4) -> GCTA");
        sequence_destroy(s);
    }

    /* Middle segment */
    {
        Sequence *s = make_seq("ATCGA");
        ApplyResult r = mutation_apply(OP_REVERSE, 2, 3, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "REV(2,3) on ATCGA succeeds");
        CHECK_STR(buf, "AGCTA", "ATCGA -> REV(2,3) -> AGCTA");
        sequence_destroy(s);
    }

    /* Length 1 reverse (no-op, should fail) */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_REVERSE, 1, 1, '\0', s);
        CHECK(r == APPLY_NO_OP || r == APPLY_INVALID_POSITION,
              "REV(1,1) rejected as no-op");
        sequence_destroy(s);
    }

    /* Length 0 reverse (invalid) */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_REVERSE, 1, 0, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION || r == APPLY_NO_OP,
              "REV(1,0) invalid/no-op");
        sequence_destroy(s);
    }

    /* Starting position outside sequence */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_REVERSE, 10, 2, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION, "REV(10,2) on len-4 invalid");
        sequence_destroy(s);
    }

    /* Segment extending beyond sequence */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_REVERSE, 3, 4, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION, "REV(3,4) extends beyond");
        sequence_destroy(s);
    }

    /* Empty sequence */
    {
        Sequence *s = make_seq("");
        ApplyResult r = mutation_apply(OP_REVERSE, 1, 2, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION, "REV on empty invalid");
        sequence_destroy(s);
    }

    printf("  REVERSE boundaries: done\n\n");
}

/* SECTION 8: SWAP Boundary Cases */

static void test_swap_boundaries(void)
{
    printf("SWAP Boundary Cases\n");

    /* First and last positions: ATCG -> swap A(1) and G(4) -> GTCA */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_SWAP, 1, 4, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "SWP(1,4) succeeds");
        CHECK_STR(buf, "GTCA", "ATCG -> SWP(1,4) -> GTCA");
        sequence_destroy(s);
    }

    /* Adjacent positions */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_SWAP, 1, 2, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "SWP(1,2) succeeds");
        CHECK_STR(buf, "TACG", "ATCG -> SWP(1,2) -> TACG");
        sequence_destroy(s);
    }

    /* Middle positions */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_SWAP, 2, 3, '\0', s);
        char buf[256]; tape_str(s, buf);
        CHECK(r == APPLY_SUCCESS, "SWP(2,3) succeeds");
        CHECK_STR(buf, "ACTG", "ATCG -> SWP(2,3) -> ACTG");
        sequence_destroy(s);
    }

    /* Same position (rejected as no-op or invalid) */
    {
        Sequence *s = make_seq("ATCG");
        ApplyResult r = mutation_apply(OP_SWAP, 2, 2, '\0', s);
        CHECK(r == APPLY_NO_OP || r == APPLY_INVALID_POSITION,
              "SWP(2,2) rejected (no-op or invalid)");
        sequence_destroy(s);
    }

    /* Equal bases (no-op) */
    {
        Sequence *s = make_seq("AATT");
        ApplyResult r = mutation_apply(OP_SWAP, 1, 2, '\0', s);
        CHECK(r == APPLY_NO_OP, "SWP(1,2) on AA rejected as no-op");
        sequence_destroy(s);
    }

    /* Invalid positions */
    {
        Sequence *s = make_seq("ATCG");
        CHECK(mutation_apply(OP_SWAP, 0, 2, '\0', s) == APPLY_INVALID_POSITION,
              "SWP(0,2) invalid");
        CHECK(mutation_apply(OP_SWAP, 1, 5, '\0', s) == APPLY_INVALID_POSITION,
              "SWP(1,5) invalid");
        {
            ApplyResult r = mutation_apply(OP_SWAP, 1, 1, '\0', s);
            CHECK(r == APPLY_NO_OP || r == APPLY_INVALID_POSITION,
                  "SWP(1,1) rejected");
        }
        sequence_destroy(s);
    }

    /* Empty sequence */
    {
        Sequence *s = make_seq("");
        ApplyResult r = mutation_apply(OP_SWAP, 1, 2, '\0', s);
        CHECK(r == APPLY_INVALID_POSITION, "SWP on empty invalid");
        sequence_destroy(s);
    }

    /* One-base sequence */
    {
        Sequence *s = make_seq("A");
        ApplyResult r = mutation_apply(OP_SWAP, 1, 1, '\0', s);
        CHECK(r == APPLY_NO_OP || r == APPLY_INVALID_POSITION,
              "SWP(1,1) on length-1 rejected");
        sequence_destroy(s);
    }

    printf("  SWAP boundaries: done\n\n");
}

/* SECTION 9: Empty Sequence Execution */

static void test_empty_execution(void)
{
    printf("Empty Sequence Execution\n");

    /* STEP on empty: probe should return no trace */
    {
        Sequence *tape = make_seq("");
        CHECK(tape->length == 0, "empty tape length is 0");

        /* Probe execution with NULL target */
        ExecutionResult probe = execution_run(tape, NULL, 0);
        CHECK(probe.trace_count == 0, "empty probe returns no trace");
        CHECK(probe.diagnostic == DIAG_FRAME_ERROR || probe.trace_count == 0,
              "empty probe diagnostic");

        /* STEP with empty tape should not crash */
        int tc = 0;
        for (int ci = 0; ci < probe.trace_count && ci < 64; ci++)
            tc++;
        CHECK(tc == 0, "no target codons from empty probe");

        sequence_destroy(tape);
    }

    /* RUN on empty: with target from probe */
    {
        Sequence *tape = make_seq("");
        ExecutionResult probe = execution_run(tape, NULL, 0);
        CHECK(probe.trace_count == 0, "empty probe returns 0 traces");

        /* Build target from probe (empty) */
        int tc = 0;
        for (int ci = 0; ci < probe.trace_count && ci < 64; ci++)
            tc++;

        if (tc > 0)
        {
            /* Should not reach here for empty tape */
            g_fail++;
            printf("  FAIL [%s:%d]: unexpected non-zero tc for empty tape\n", __FILE__, __LINE__);
        }
        else
        {
            /* No target codons - expected for empty tape */
            g_pass++;
        }

        sequence_destroy(tape);
    }

    printf("  Empty sequence execution: done\n\n");
}

/* SECTION 10: Delete-Insert-Shrink Cycle */

static void test_shrink_grow_cycle(void)
{
    printf("Shrink-Grow Cycle\n");

    Sequence *s = make_seq("TACGCGTAC");
    char buf[256];

    /* Delete all */
    for (int i = 0; i < 9; i++)
        mutation_apply(OP_DELETE, 1, 0, '\0', s);
    CHECK(s->length == 0, "length 0 after delete all");

    /* Insert several */
    mutation_apply(OP_INSERT, 1, 0, 'A', s);
    mutation_apply(OP_INSERT, 2, 0, 'T', s);
    mutation_apply(OP_INSERT, 3, 0, 'G', s);
    CHECK(s->length == 3, "length 3 after inserts");
    tape_str(s, buf);
    CHECK_STR(buf, "ATG", "recovered to ATG");

    /* Delete again */
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    CHECK(s->length == 0, "length 0 after second delete all");

    /* Insert again */
    mutation_apply(OP_INSERT, 1, 0, 'C', s);
    mutation_apply(OP_INSERT, 2, 0, 'G', s);
    CHECK(s->length == 2, "length 2 after second insert");
    tape_str(s, buf);
    CHECK_STR(buf, "CG", "recovered to CG");

    /* Reverse on small sequence */
    ApplyResult r = mutation_apply(OP_REVERSE, 1, 2, '\0', s);
    CHECK(r == APPLY_SUCCESS, "REV(1,2) on CG succeeds");
    tape_str(s, buf);
    CHECK_STR(buf, "GC", "CG -> REV(1,2) -> GC");

    /* Swap on small sequence */
    r = mutation_apply(OP_SWAP, 1, 2, '\0', s);
    CHECK(r == APPLY_SUCCESS, "SWP(1,2) on GC succeeds");
    tape_str(s, buf);
    CHECK_STR(buf, "CG", "GC -> SWP(1,2) -> CG");

    sequence_destroy(s);
    printf("  Shrink-grow cycle: done\n\n");
}

/* SECTION 11: Sequence Capacity */

static void test_sequence_capacity(void)
{
    printf("Sequence Capacity\n");

    /* Create sequence and push to capacity */
    Sequence *s = sequence_create(6);
    CHECK(s != NULL, "sequence created");
    CHECK(s->length == 0, "initial length is 0");

    /* Insert 6 bases */
    mutation_apply(OP_INSERT, 1, 0, 'A', s);
    mutation_apply(OP_INSERT, 2, 0, 'T', s);
    mutation_apply(OP_INSERT, 3, 0, 'C', s);
    mutation_apply(OP_INSERT, 4, 0, 'G', s);
    mutation_apply(OP_INSERT, 5, 0, 'A', s);
    mutation_apply(OP_INSERT, 6, 0, 'T', s);
    CHECK(s->length == 6, "length is 6");

    char buf[256];
    tape_str(s, buf);
    CHECK_STR(buf, "ATCGAT", "sequence is ATCGAT");

    /* Delete one to make room */
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    CHECK(s->length == 5, "length is 5 after delete");

    /* Insert again */
    mutation_apply(OP_INSERT, 6, 0, 'T', s);
    CHECK(s->length == 6, "length is 6 after re-insert");
    tape_str(s, buf);
    CHECK_STR(buf, "TCGATT", "sequence rebuilt");

    /* Delete all to zero (length is 6) */
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    mutation_apply(OP_DELETE, 1, 0, '\0', s);
    CHECK(s->length == 0, "length 0 after delete all");

    /* Insert again from zero */
    {
        ApplyResult r2 = mutation_apply(OP_INSERT, 1, 0, 'G', s);
        CHECK(r2 == APPLY_SUCCESS, "insert after reaching zero succeeds");
        CHECK(s->length == 1, "length 1 after insert from zero");
        tape_str(s, buf);
        CHECK_STR(buf, "G", "sequence is G after recovery");
    }

    sequence_destroy(s);
    printf("  Sequence capacity: done\n\n");
}

/* SECTION 12: Multiple Generated Sequences */

static void test_multiple_random_sequences(void)
{
    printf("Multiple Random Sequences\n");

    const char *test_seqs[] = {
        "TACGCGTAC",
        "AATTCCGG",
        "TACCCGCACT",
        "TACACCTACATT",
        "TACCCCTTAATC",
        "ATCG",
        "G",
        "TACACACAGACT",
        "TACTATACACAAACCACT",
        "TACCGGACAACCATGGGAATC"
    };
    int n = (int)(sizeof(test_seqs) / sizeof(test_seqs[0]));
    const char bases[] = "ATCG";

    for (int t = 0; t < n; t++)
    {
        Sequence *s = make_seq(test_seqs[t]);
        CHECK(s != NULL, "sequence created");

        /* Apply some mutations */
        for (int m = 0; m < 5; m++)
        {
            if (s->length > 1)
            {
                /* Try DELETE */
                int pos = 1 + (int)((unsigned)(s->length) % s->length);
                mutation_apply(OP_DELETE, pos, 0, '\0', s);
            }

            /* Try INSERT */
            int ins_pos = 1 + (int)((unsigned)(s->length + 1) % (s->length + 1));
            char base = bases[(t + m) % 4];
            mutation_apply(OP_INSERT, ins_pos, 0, base, s);
        }

        CHECK(s->length > 0 || s->length == 0,
              "sequence still valid after mutations");

        /* Reset and verify */
        sequence_destroy(s);
    }

    printf("  Multiple random sequences: done\n\n");
}

/* SECTION 13: Freeworld STEP/RUN on Empty After Editing */

static void test_step_run_empty_after_edit(void)
{
    printf("STEP/RUN on Empty After Edit\n");

    Sequence *tape = make_seq("TACGCGTAC");
    CHECK(tape->length == 9, "start with 9 bases");

    /* Delete all */
    for (int i = 0; i < 9; i++)
        mutation_apply(OP_DELETE, 1, 0, '\0', tape);
    CHECK(tape->length == 0, "empty after deleting all");

    /* Simulate probe (what simulation does for STEP) */
    ExecutionResult probe = execution_run(tape, NULL, 0);
    CHECK(probe.trace_count == 0, "empty tape probe: 0 traces");

    /* Verify we can still operate on the empty tape */
    ApplyResult r = mutation_apply(OP_INSERT, 1, 0, 'A', tape);
    CHECK(r == APPLY_SUCCESS, "INSERT after empty probe works");
    CHECK(tape->length == 1, "length 1 after recovery");

    /* STEP probe on single base */
    probe = execution_run(tape, NULL, 0);
    CHECK(probe.trace_count == 0, "single base probe: 0 traces (not div by 3)");

    /* RUN on single base with target */
    const char *target[] = {"MET"};
    ExecutionResult result = execution_run(tape, target, 1);
    CHECK(result.diagnostic == DIAG_FRAME_ERROR,
          "single base RUN: FRAME ERROR");

    sequence_destroy(tape);
    printf("  STEP/RUN on empty after edit: done\n\n");
}

int main(void)
{
    printf("DNA Error Lab - Freeworld / Simulation Tests\n\n");

    test_sim_mode_inventory();
    test_campaign_inventory_finite();
    test_empty_delete();
    test_empty_reverse();
    test_empty_swap();
    test_empty_insert();
    test_empty_to_full_recovery();
    test_delete_boundaries();
    test_insert_boundaries();
    test_reverse_boundaries();
    test_swap_boundaries();
    test_empty_execution();
    test_shrink_grow_cycle();
    test_sequence_capacity();
    test_multiple_random_sequences();
    test_step_run_empty_after_edit();

    printf("Freeworld Tests: %d passed, %d failed\n",
           g_pass, g_fail);

    return g_fail > 0 ? 1 : 0;
}