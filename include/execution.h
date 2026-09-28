#ifndef EXECUTION_H
#define EXECUTION_H

#include "sequence.h"

#define MAX_CODONS 64

#define MAX_AA_NAME 8

/* Diagnostic types in priority order */
typedef enum {
    DIAG_SUCCESS = 0,
    DIAG_FRAME_ERROR = 1,
    DIAG_PREMATURE_STOP = 2,
    DIAG_CODON_MISS = 3,
    DIAG_RUN_ON = 4
} DiagnosticType;

typedef struct {
    int codon_index;
    char dna_triplet[4];
    char mrna_triplet[4];
    char actual_output[MAX_AA_NAME];
    char expected_output[MAX_AA_NAME];
    int is_match;
    int is_stop;
} CodonTrace;

typedef struct {
    DiagnosticType diagnostic;
    int codon_count;
    int target_length;
    int actual_length;
    CodonTrace trace[MAX_CODONS];
    int trace_count;
    char message[256];
} ExecutionResult;

ExecutionResult execution_run(const Sequence *tape,
                              const char *const *target,
                              int target_count);

void execution_result_reset(ExecutionResult *result);

const char *execution_diagnostic_name(DiagnosticType diag);

#endif
