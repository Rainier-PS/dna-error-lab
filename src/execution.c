#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "execution.h"
#include "codon.h"

/* Fast lookup table for DNA -> mRNA transcription */
static const char BASE_COMPLEMENT[256] = {
    ['A'] = 'U', ['a'] = 'U',
    ['T'] = 'A', ['t'] = 'A',
    ['C'] = 'G', ['c'] = 'G',
    ['G'] = 'C', ['g'] = 'C'
};

static inline char transcribe_base(char dna_base)
{
    char rna = BASE_COMPLEMENT[(unsigned char)dna_base];
    return rna ? rna : 'N';
}

static int iequals(const char *s1, const char *s2)
{
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (tolower((unsigned char)*s1) != tolower((unsigned char)*s2)) return 0;
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

const char *execution_diagnostic_name(DiagnosticType diag)
{
    static const char *names[] = {
        [DIAG_SUCCESS]       = "SUCCESS",
        [DIAG_FRAME_ERROR]   = "FRAME ERROR",
        [DIAG_PREMATURE_STOP]= "PREMATURE STOP",
        [DIAG_CODON_MISS]    = "CODON MISS",
        [DIAG_RUN_ON]        = "RUN-ON"
    };

    if (diag >= DIAG_SUCCESS && diag <= DIAG_RUN_ON) {
        return names[diag];
    }
    return "UNKNOWN";
}

void execution_result_reset(ExecutionResult *result)
{
    if (result) {
        memset(result, 0, sizeof(*result));
    }
}

ExecutionResult execution_run(const Sequence *tape,
                              const char *const *target,
                              int target_count)
{
    ExecutionResult result;
    execution_result_reset(&result);

    if (!tape) {
        result.diagnostic = DIAG_FRAME_ERROR;
        snprintf(result.message, sizeof(result.message), "Invalid execution parameters");
        return result;
    }

    /* When target is NULL, compute codon count from tape length. (Allows simulation mode to display output without a target.) */
    if (!target || target_count <= 0) {
        if ((int)tape->length % 3 != 0 || (int)tape->length == 0) {
            result.diagnostic = DIAG_FRAME_ERROR;
            snprintf(result.message, sizeof(result.message),
                     "FRAME ERROR\nSequence length must be divisible by 3.\nTape has %d bases",
                     (int)tape->length);
            return result;
        }
        target_count = (int)tape->length / 3;
    }

    result.target_length = 3 * target_count;
    result.actual_length = (int)tape->length;
    result.codon_count = target_count;

    /* Step 1: Validate frame length */
    if ((int)tape->length != result.target_length) {
        result.diagnostic = DIAG_FRAME_ERROR;
        snprintf(result.message, sizeof(result.message),
                 "FRAME ERROR\nSequence length must be divisible by 3.\nTape has %d bases (expected %d)",
                 (int)tape->length, result.target_length);
        return result;
    }

    int found_error = 0;

    /* Step 2: Evaluate codons sequentially */
    for (int i = 0; i < target_count; i++) {
        if (result.trace_count >= MAX_CODONS) break;

        size_t idx = (size_t)i * 3;
        char codon_dna[4] = { tape->data[idx], tape->data[idx + 1], tape->data[idx + 2], '\0' };
        char codon_mrna[4] = {
            transcribe_base(codon_dna[0]),
            transcribe_base(codon_dna[1]),
            transcribe_base(codon_dna[2]),
            '\0'
        };

        const char *amino = codon_translate(codon_mrna);

        CodonTrace *ct = &result.trace[result.trace_count++];
        ct->codon_index = i + 1;
        memcpy(ct->dna_triplet, codon_dna, 4);
        memcpy(ct->mrna_triplet, codon_mrna, 4);

        snprintf(ct->actual_output, sizeof(ct->actual_output), "%s", amino ? amino : "UNKNOWN");

        ct->is_stop = iequals(amino, "STOP");

        /* When target is NULL (simulation mode), skip comparison */
        if (!target) {
            snprintf(ct->expected_output, sizeof(ct->expected_output), "N/A");
            ct->is_match = 1;  /* No target to compare against */
        } else {
            snprintf(ct->expected_output, sizeof(ct->expected_output), "%s", target[i]);
            ct->is_match = iequals(amino, target[i]);

            /* Diagnostic checks in priority order - FRAME ERROR > PREMATURE STOP > CODON MISS > RUN-ON */
            if (!found_error) {
                int is_last = (i == target_count - 1);
                int target_is_stop = iequals(target[i], "STOP");

                if (!is_last && ct->is_stop) {
                    result.diagnostic = DIAG_PREMATURE_STOP;
                    snprintf(result.message, sizeof(result.message),
                             "PREMATURE STOP\nCODON %d\n%s -> %s -> %s\nTARGET: %s",
                             i + 1, codon_dna, codon_mrna, amino, target[i]);
                    found_error = 1;
                } else if (!ct->is_match) {
                    if (is_last && !ct->is_stop && target_is_stop) {
                        result.diagnostic = DIAG_RUN_ON;
                        snprintf(result.message, sizeof(result.message),
                                 "RUN-ON\nCODON %d\n%s -> %s -> %s\nTARGET: STOP",
                                 i + 1, codon_dna, codon_mrna, amino);
                    } else {
                        result.diagnostic = DIAG_CODON_MISS;
                        snprintf(result.message, sizeof(result.message),
                                 "CODON MISS\nCODON %d\n%s -> %s -> %s\nTARGET: %s",
                                 i + 1, codon_dna, codon_mrna, amino, target[i]);
                    }
                    found_error = 1;
                }
            }
        }  /* end else (target != NULL) */
    }

    /* All codons evaluated successfully */
    if (!found_error) {
        result.diagnostic = DIAG_SUCCESS;
        snprintf(result.message, sizeof(result.message), "SUCCESS");
    }

    return result;
}
