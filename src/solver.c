/*
AI USAGE DISCLOSURE

I used AI tools in the development of this BFS solver to help debug and adapt the implementation to this project's sequence-mutation state space.
I already understood BFS and the general idea of representing each sequence and remaining mutation inventory as a search state, but my initial implementation had problems when I applied the algorithm to this project.

I used AI assistance to investigate those implementation problems, reason about state representation, visited-state tracking, transition generation, and the solver's stopping behaviour.
Then I implemented, modified, and tested the resulting code myself.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "solver.h"
#include "challenge.h"
#include "sequence.h"
#include "mutation.h"
#include "levels.h"

#define QUEUE_MAX_STATES  100000
#define VISITED_CAPACITY  131072
#define VISITED_MASK      (VISITED_CAPACITY - 1)
#define VISITED_MAX_PROBE 16

static SolverState g_queue[QUEUE_MAX_STATES];
static int g_queue_head = 0;
static int g_queue_tail = 0;

static int g_visited_table[VISITED_CAPACITY];

static SolutionRecord g_solutions[100];
static int g_solution_count = 0;

/* Last result for external queries */
static SolverResult g_last_result;

static unsigned int hash_state(const SolverState *st)
{
    unsigned int h = 2166136261u;
    int i;
    for (i = 0; i < st->tape_len; i++)
        h = (h ^ (unsigned char)st->tape[i]) * 16777619u;
    h = (h ^ (unsigned int)st->del_remaining) * 16777619u;
    h = (h ^ (unsigned int)st->ins_remaining) * 16777619u;
    h = (h ^ (unsigned int)st->rev_remaining) * 16777619u;
    h = (h ^ (unsigned int)st->swp_remaining) * 16777619u;
    return h;
}

static void visited_reset(void)
{
    memset(g_visited_table, -1, sizeof(g_visited_table));
}

static int visited_contains(const SolverState *st)
{
    unsigned int h = hash_state(st) & VISITED_MASK;
    int probe;
    for (probe = 0; probe < VISITED_MAX_PROBE; probe++)
    {
        int idx = (int)((h + (unsigned int)probe) & VISITED_MASK);
        SolverState *cand;
        if (g_visited_table[idx] == -1) return 0;
        cand = &g_queue[g_visited_table[idx]];
        if (cand->tape_len == st->tape_len &&
            cand->del_remaining == st->del_remaining &&
            cand->ins_remaining == st->ins_remaining &&
            cand->rev_remaining == st->rev_remaining &&
            cand->swp_remaining == st->swp_remaining &&
            memcmp(cand->tape, st->tape, (size_t)st->tape_len) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static void visited_insert(const SolverState *st, int q_idx)
{
    unsigned int h = hash_state(st) & VISITED_MASK;
    int probe;
    for (probe = 0; probe < VISITED_MAX_PROBE; probe++)
    {
        int idx = (int)((h + (unsigned int)probe) & VISITED_MASK);
        if (g_visited_table[idx] == -1)
        {
            g_visited_table[idx] = q_idx;
            return;
        }
    }
    /* Table full at this probe chain - skip insertion */
}

static void record_solution(const char *tape, int len, int depth)
{
    SolutionRecord *rec;
    if (g_solution_count >= 100) return;

    rec = &g_solutions[g_solution_count++];
    strncpy(rec->tape, tape, sizeof(rec->tape) - 1);
    rec->tape[sizeof(rec->tape) - 1] = '\0';
    rec->tape_len = len;
    rec->depth = depth;
}

static void try_enqueue_transition(
    Challenge *ch,
    int parent_idx,
    const SolverState *curr,
    Instruction inst,
    InstructionType op)
{
    SolverState next;
    Sequence *tape_copy;
    Sequence *mutated;

    tape_copy = sequence_from_string(curr->tape);
    if (!tape_copy) return;

    if (!mutation_validate(&inst, tape_copy) || mutation_is_noop(&inst, tape_copy))
    {
        sequence_destroy(tape_copy);
        return;
    }

    mutated = sequence_copy(tape_copy);
    sequence_destroy(tape_copy);
    if (!mutated) return;

    mutation_apply(op, inst.pos1, inst.pos2, inst.base, mutated);

    memset(&next, 0, sizeof(next));
    next.tape_len = (int)mutated->length;
    next.del_remaining = curr->del_remaining - (op == OP_DELETE ? 1 : 0);
    next.ins_remaining = curr->ins_remaining - (op == OP_INSERT ? 1 : 0);
    next.rev_remaining = curr->rev_remaining - (op == OP_REVERSE ? 1 : 0);
    next.swp_remaining = curr->swp_remaining - (op == OP_SWAP ? 1 : 0);
    next.depth = curr->depth + 1;
    next.parent_index = parent_idx;
    next.last_inst = inst;
    sequence_to_string(mutated, next.tape, sizeof(next.tape));

    /* Check visited BEFORE the expensive challenge_execute. */
    if (visited_contains(&next))
    {
        sequence_destroy(mutated);
        return;
    }

    {
        ExecutionResult exec = challenge_execute(ch, mutated);
        if (exec.diagnostic == DIAG_SUCCESS)
        {
            record_solution(next.tape, next.tape_len, next.depth);
        }
    }
    sequence_destroy(mutated);

    if (g_queue_tail < QUEUE_MAX_STATES)
    {
        g_queue[g_queue_tail] = next;
        visited_insert(&next, g_queue_tail);
        g_queue_tail++;
    }
}

int solver_run(Challenge *ch, SolverState *initial_state, int max_depth)
{
    int curr_idx;
    int target_depth = -1;
    const char bases[] = {'A', 'C', 'G', 'T'};

    g_queue_head = 0;
    g_queue_tail = 0;
    g_solution_count = 0;
    memset(g_solutions, 0, sizeof(g_solutions));
    visited_reset();

    g_queue[g_queue_tail] = *initial_state;
    visited_insert(initial_state, g_queue_tail);
    g_queue_tail++;

    while (g_queue_head < g_queue_tail)
    {
        SolverState *curr;
        int tape_len;
        int p, b, len;

        curr_idx = g_queue_head++;
        curr = &g_queue[curr_idx];
        tape_len = curr->tape_len;

        /* Stop if we've reached a solution at the target depth */
        if (target_depth != -1 && curr->depth >= target_depth) break;
        /* Stop if we've exceeded the max depth budget */
        if (max_depth > 0 && curr->depth >= max_depth) continue;

        /* OP_INSERT: 1-based positions [1..tape_len+1] */
        if (curr->ins_remaining > 0)
        {
            for (p = 1; p <= tape_len + 1; p++)
            {
                Sequence *tape_copy;
                Instruction base_inst;
                base_inst.type = OP_INSERT;
                base_inst.pos1 = p;
                base_inst.pos2 = 0;
                base_inst.base = 'A';

                tape_copy = sequence_from_string(curr->tape);
                if (!tape_copy) continue;
                if (!mutation_validate(&base_inst, tape_copy))
                {
                    sequence_destroy(tape_copy);
                    continue;
                }

                for (b = 0; b < 4; b++)
                {
                    Instruction inst = base_inst;
                    Sequence *mutated;
                    SolverState next;
                    inst.base = bases[b];
                    if (mutation_is_noop(&inst, tape_copy)) continue;

                    mutated = sequence_copy(tape_copy);
                    if (!mutated) continue;
                    mutation_apply(OP_INSERT, p, 0, bases[b], mutated);

                    memset(&next, 0, sizeof(next));
                    sequence_to_string(mutated, next.tape, sizeof(next.tape));
                    next.tape_len = (int)mutated->length;
                    next.del_remaining = curr->del_remaining;
                    next.ins_remaining = curr->ins_remaining - 1;
                    next.rev_remaining = curr->rev_remaining;
                    next.swp_remaining = curr->swp_remaining;
                    next.depth = curr->depth + 1;
                    next.parent_index = curr_idx;
                    next.last_inst = inst;

                    if (!visited_contains(&next))
                    {
                        ExecutionResult exec = challenge_execute(ch, mutated);
                        if (exec.diagnostic == DIAG_SUCCESS)
                            record_solution(next.tape, next.tape_len, next.depth);
                        if (g_queue_tail < QUEUE_MAX_STATES)
                        {
                            g_queue[g_queue_tail] = next;
                            visited_insert(&next, g_queue_tail);
                            g_queue_tail++;
                        }
                    }
                    sequence_destroy(mutated);
                }
                sequence_destroy(tape_copy);
            }
        }

        /* OP_SWAP: 1-based positions [1..tape_len], p1 != p2 */
        if (curr->swp_remaining > 0)
        {
            int p2;
            for (p = 1; p <= tape_len; p++)
            {
                for (p2 = p + 1; p2 <= tape_len; p2++)
                {
                    Instruction inst;
                    inst.type = OP_SWAP;
                    inst.pos1 = p;
                    inst.pos2 = p2;
                    inst.base = '\0';
                    try_enqueue_transition(ch, curr_idx, curr, inst, OP_SWAP);
                }
            }
        }

        /* OP_REVERSE: 1-based positions, length >= 2 */
        if (curr->rev_remaining > 0)
        {
            int max_len = tape_len;
            for (len = 2; len <= max_len; len++)
            {
                for (p = 1; p + len - 1 <= tape_len; p++)
                {
                    Instruction inst;
                    inst.type = OP_REVERSE;
                    inst.pos1 = p;
                    inst.pos2 = len;
                    inst.base = '\0';
                    try_enqueue_transition(ch, curr_idx, curr, inst, OP_REVERSE);
                }
            }
        }

        /* OP_DELETE: 1-based positions [1..tape_len] */
        if (curr->del_remaining > 0)
        {
            for (p = 1; p <= tape_len; p++)
            {
                Instruction inst;
                inst.type = OP_DELETE;
                inst.pos1 = p;
                inst.pos2 = 0;
                inst.base = '\0';
                try_enqueue_transition(ch, curr_idx, curr, inst, OP_DELETE);
            }
        }

        if (g_solution_count > 0 && target_depth == -1)
        {
            target_depth = g_solutions[0].depth;
        }
    }

    return g_solution_count;
}

int solver_validate_challenge(const Challenge *ch)
{
    SolverState initial;
    int total_budget;
    const LevelDefinition *level;
    int count;

    /* Reset result */
    memset(&g_last_result, 0, sizeof(SolverResult));

    if (ch == NULL || ch->initial_tape == NULL || ch->final_tape == NULL)
    {
        strcpy(g_last_result.message, "Invalid challenge");
        return 0;
    }

    level = ch->level_def;
    if (level == NULL)
    {
        strcpy(g_last_result.message, "Missing level definition");
        return 0;
    }

    /* Check that initial tape is not already solved */
    {
        ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
        if (init_exec.diagnostic == DIAG_SUCCESS)
        {
            strcpy(g_last_result.message, "Initial tape is already solved");
            return 0;
        }
    }

    /* Calculate total instruction budget */
    total_budget = ch->inventory.delete_count + ch->inventory.insert_count +
                   ch->inventory.reverse_count + ch->inventory.swap_count;
    if (total_budget > SOLVER_MAX_DEPTH)
        total_budget = SOLVER_MAX_DEPTH;

    /* Initialize initial state */
    memset(&initial, 0, sizeof(SolverState));
    sequence_to_string(ch->initial_tape, initial.tape, sizeof(initial.tape));
    initial.tape_len = (int)ch->initial_tape->length;
    initial.del_remaining = ch->inventory.delete_count;
    initial.ins_remaining = ch->inventory.insert_count;
    initial.rev_remaining = ch->inventory.reverse_count;
    initial.swp_remaining = ch->inventory.swap_count;
    initial.depth = 0;
    initial.parent_index = -1;

    /* Run BFS solver */
    count = solver_run((Challenge *)ch, &initial, total_budget);

    /* Evaluate results */
    if (count == 0)
    {
        g_last_result.is_valid = 0;
        g_last_result.solution_count = 0;
        g_last_result.distinct_tape_count = 0;
        g_last_result.min_depth = -1;
        strcpy(g_last_result.message, "No solution exists");
        return 0;
    }

    g_last_result.min_depth = g_solutions[0].depth;
    g_last_result.solution_count = count;
    g_last_result.distinct_tape_count = count;
    g_last_result.is_valid = 1;
    snprintf(g_last_result.message, sizeof(g_last_result.message),
             "Valid: %d solution(s) at depth %d", count, g_last_result.min_depth);
    return 1;
}

const SolverResult *solver_get_last_result(void)
{
    return &g_last_result;
}