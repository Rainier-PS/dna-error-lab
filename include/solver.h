#ifndef SOLVER_H
#define SOLVER_H

#include "mutation.h"

typedef struct Challenge Challenge;

#define SOLVER_MAX_DEPTH 16

#define SOLVER_MAX_STATES 100000

/* Result of solver validation */
typedef struct {
    int is_valid;
    int solution_count;
    int distinct_tape_count;
    int min_depth;
    char message[256];
} SolverResult;

typedef struct {
    char tape[256];
    int tape_len;
    int depth;
} SolutionRecord;

typedef struct {
    char tape[256];
    int tape_len;
    int del_remaining;
    int ins_remaining;
    int rev_remaining;
    int swp_remaining;
    int depth;
    int parent_index;
    Instruction last_inst;
} SolverState;

int solver_validate_challenge(const Challenge *ch);

/* Run the BFS solver and return the number of solutions found. */
int solver_run(Challenge *ch, SolverState *initial_state, int max_depth);

const SolverResult *solver_get_last_result(void);

#endif