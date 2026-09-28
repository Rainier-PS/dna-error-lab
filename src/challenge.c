#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "challenge.h"
#include "solver.h"
#include "codon.h"

static const char *AMINO_ACIDS[] = {
    "MET", "PRO", "VAL", "ALA", "GLY", "SER", "THR",
    "LEU", "ILE", "CYS", "TYR", "HIS", "GLN", "ASN",
    "ASP", "GLU", "LYS", "ARG", "PHE", "TRP"
};
#define NUM_AMINO_ACIDS 20

typedef struct {
    const char *rna;
    const char *aa;
    const char *dna;
} CodonMap;

/* Pre-computed translation table: mRNA codon -> amino acid -> DNA template */
static const CodonMap CODON_TABLE[] = {
    {"UUU", "PHE", "AAA"}, {"UUC", "PHE", "AAG"}, {"UUA", "LEU", "AAT"}, {"UUG", "LEU", "AAC"},
    {"UCU", "SER", "AGA"}, {"UCC", "SER", "AGG"}, {"UCA", "SER", "AGT"}, {"UCG", "SER", "AGC"},
    {"UAU", "TYR", "ATA"}, {"UAC", "TYR", "ATG"}, {"UAA", "STOP", "ATT"}, {"UAG", "STOP", "ATC"},
    {"UGU", "CYS", "ACA"}, {"UGC", "CYS", "ACG"}, {"UGA", "STOP", "ACT"}, {"UGG", "TRP", "ACC"},
    {"CUU", "LEU", "GAA"}, {"CUC", "LEU", "GAG"}, {"CUA", "LEU", "GAT"}, {"CUG", "LEU", "GAC"},
    {"CCU", "PRO", "GGA"}, {"CCC", "PRO", "GGG"}, {"CCA", "PRO", "GGT"}, {"CCG", "PRO", "GGC"},
    {"CAU", "HIS", "GTA"}, {"CAC", "HIS", "GTG"}, {"CAA", "GLN", "GTT"}, {"CAG", "GLN", "GTC"},
    {"CGU", "ARG", "GCA"}, {"CGC", "ARG", "GCG"}, {"CGA", "ARG", "GCT"}, {"CGG", "ARG", "GCC"},
    {"AUU", "ILE", "TAA"}, {"AUC", "ILE", "TAG"}, {"AUA", "ILE", "TAT"}, {"AUG", "MET", "TAC"},
    {"ACU", "THR", "TGA"}, {"ACC", "THR", "TGG"}, {"ACA", "THR", "TGT"}, {"ACG", "THR", "TGC"},
    {"AAU", "ASN", "TTA"}, {"AAC", "ASN", "TTG"}, {"AAA", "LYS", "TTT"}, {"AAG", "LYS", "TTC"},
    {"AGU", "SER", "TCA"}, {"AGC", "SER", "TCG"}, {"AGA", "ARG", "TCT"}, {"AGG", "ARG", "TCC"},
    {"GUU", "VAL", "CAA"}, {"GUC", "VAL", "CAG"}, {"GUA", "VAL", "CAT"}, {"GUG", "VAL", "CAC"},
    {"GCU", "ALA", "CGA"}, {"GCC", "ALA", "CGG"}, {"GCA", "ALA", "CGT"}, {"GCG", "ALA", "CGC"},
    {"GAU", "ASP", "CTA"}, {"GAC", "ASP", "CTG"}, {"GAA", "GLU", "CTT"}, {"GAG", "GLU", "CTC"},
    {"GGU", "GLY", "CCA"}, {"GGC", "GLY", "CCG"}, {"GGA", "GLY", "CCT"}, {"GGG", "GLY", "CCC"}
};

/* Backward generation operation types (solution direction) */
#define SOL_OP_INSERT  0   // solution=INSERT, corruption=DELETE
#define SOL_OP_DELETE  1   // solution=DELETE, corruption=INSERT
#define SOL_OP_REVERSE 2   // solution=REV,    corruption=REV (self-inverse)
#define SOL_OP_SWAP    3   // solution=SWP,    corruption=SWP (self-inverse)

static unsigned int challenge_rand(unsigned int *seed)
{
    *seed = (*seed * 1103515245 + 12345) & 0x7fffffff;
    return *seed;
}

/* Get all DNA codons that translate to the given amino acid */
static int get_dna_codons(const char *amino, const char *out_codons[8])
{
    int count = 0;
    for (size_t i = 0; i < sizeof(CODON_TABLE) / sizeof(CODON_TABLE[0]); i++) {
        if (strcmp(CODON_TABLE[i].aa, amino) == 0) {
            out_codons[count++] = CODON_TABLE[i].dna;
            if (count >= 8) break;
        }
    }
    return count;
}

/* Translate a DNA template string into its protein output. (Returns the number of codons written, or -1 on error.) */
static int translate_dna_to_target(const char *dna, char target[][MAX_AA_NAME], int max_codons)
{
    size_t len = strlen(dna);
    if (len == 0 || len % 3 != 0) return -1;

    int codon_count = (int)(len / 3);
    if (codon_count > max_codons) return -1;

    for (int i = 0; i < codon_count; i++)
    {
        const char *dna_triplet = &dna[i * 3];

        /* Transcribe DNA template to mRNA: A->U, T->A, C->G, G->C */
        char mrna[4];
        for (int j = 0; j < 3; j++)
        {
            switch (dna_triplet[j])
            {
                case 'A': mrna[j] = 'U'; break;
                case 'T': mrna[j] = 'A'; break;
                case 'C': mrna[j] = 'G'; break;
                case 'G': mrna[j] = 'C'; break;
                default:  mrna[j] = 'N'; break;
            }
        }
        mrna[3] = '\0';

        /* Look up amino acid from mRNA codon */
        int found = 0;
        for (size_t k = 0; k < sizeof(CODON_TABLE) / sizeof(CODON_TABLE[0]); k++)
        {
            if (strcmp(CODON_TABLE[k].rna, mrna) == 0)
            {
                strcpy(target[i], CODON_TABLE[k].aa);
                found = 1;
                break;
            }
        }
        if (!found) return -1;
    }
    return codon_count;
}

/* Build a challenge from the level's fixed data. (Returns 1 on success, 0 on failure.) */
static int build_level_challenge(Challenge *ch, const LevelDefinition *level)
{
    if (!level->target_dna || !level->initial_dna)
        return 0;
    if (level->solution_count <= 0 || level->solution_count > CANONICAL_MAX_STEPS)
        return 0;

    size_t target_len = strlen(level->target_dna);
    size_t initial_len = strlen(level->initial_dna);

    /* Validate DNA strings */
    if (target_len == 0 || target_len % 3 != 0) return 0;
    if (initial_len == 0 || initial_len > MAX_SEQ_LEN) return 0;

    /* Validate characters are valid DNA */
    for (size_t i = 0; i < target_len; i++)
    {
        char c = level->target_dna[i];
        if (c != 'A' && c != 'C' && c != 'G' && c != 'T') return 0;
    }
    for (size_t i = 0; i < initial_len; i++)
    {
        char c = level->initial_dna[i];
        if (c != 'A' && c != 'C' && c != 'G' && c != 'T') return 0;
    }

    /* Translate target DNA to protein */
    int tc = translate_dna_to_target(level->target_dna,
                                     ch->target, MAX_TARGET_CODONS);
    if (tc < 0) return 0;
    ch->target_count = tc;

    /* Verify target starts with MET and ends with STOP */
    if (strcmp(ch->target[0], "MET") != 0) return 0;
    if (strcmp(ch->target[tc - 1], "STOP") != 0) return 0;

    /* No premature STOP */
    for (int i = 0; i < tc - 1; i++)
    {
        if (strcmp(ch->target[i], "STOP") == 0) return 0;
    }

    /* Build final tape from canonical target DNA */
    ch->final_tape = sequence_create(target_len);
    if (!ch->final_tape) return 0;
    memcpy(ch->final_tape->data, level->target_dna, target_len);
    ch->final_tape->data[target_len] = '\0';
    ch->final_tape->length = target_len;

    /* Verify final tape produces target protein */
    ExecutionResult test_exec = challenge_execute(ch, ch->final_tape);
    if (test_exec.diagnostic != DIAG_SUCCESS) return 0;

    /* Build initial tape from canonical initial DNA */
    ch->initial_tape = sequence_create(initial_len);
    if (!ch->initial_tape) return 0;
    memcpy(ch->initial_tape->data, level->initial_dna, initial_len);
    ch->initial_tape->data[initial_len] = '\0';
    ch->initial_tape->length = initial_len;

    /* Verify initial tape is NOT already solved */
    ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
    if (init_exec.diagnostic == DIAG_SUCCESS) return 0;

    /* Copy solution from level definition */
    program_init(&ch->solution);
    for (int i = 0; i < level->solution_count; i++)
    {
        program_add(&ch->solution, &level->solution[i]);
    }

    /* Replay solution and verify it produces the target DNA */
    Sequence *replay = sequence_copy(ch->initial_tape);
    if (!replay) return 0;

    for (int i = 0; i < ch->solution.count; i++)
    {
        Instruction *inst = &ch->solution.instructions[i];
        if (mutation_apply(inst->type, inst->pos1, inst->pos2,
                           inst->base, replay) != APPLY_SUCCESS)
        {
            sequence_destroy(replay);
            return 0;  // BUG: canonical solution replay failed
        }
    }

    if (!sequence_equal(replay, ch->final_tape))
    {
        sequence_destroy(replay);
        return 0;  // BUG: canonical solution doesn't reach target DNA
    }
    sequence_destroy(replay);

    /* Set inventory from level budget */
    inventory_init(&ch->inventory,
                   level->delete_budget,
                   level->insert_budget,
                   level->reverse_budget,
                   level->swap_budget);

    return 1;
}

/* Choose a valid solution operation type based on level constraints. (Returns the chosen type, or -1 if no type is available.) */
static int choose_sol_type(const LevelDefinition *level,
                           int sol_del, int sol_ins,
                           int sol_rev, int sol_swp,
                           unsigned int *rng)
{
    int allowed[4], n = 0;

    switch (level->corruption_type)
    {
        case CORRUPTION_DEL_ONLY:
            if (sol_ins < level->insert_budget) allowed[n++] = SOL_OP_INSERT;
            break;

        case CORRUPTION_INS_ONLY:
            if (sol_del < level->delete_budget) allowed[n++] = SOL_OP_DELETE;
            break;

        case CORRUPTION_INS_DEL:
            if (sol_ins < level->insert_budget) allowed[n++] = SOL_OP_INSERT;
            if (sol_del < level->delete_budget) allowed[n++] = SOL_OP_DELETE;
            break;

        case CORRUPTION_REV_ONLY:
            if (sol_rev < level->reverse_budget) allowed[n++] = SOL_OP_REVERSE;
            break;

        case CORRUPTION_SWP_ONLY:
            if (sol_swp < level->swap_budget) allowed[n++] = SOL_OP_SWAP;
            break;

        case CORRUPTION_DEL_REV:
            if (sol_del < level->delete_budget)  allowed[n++] = SOL_OP_DELETE;
            if (sol_rev < level->reverse_budget) allowed[n++] = SOL_OP_REVERSE;
            break;

        case CORRUPTION_DEL_INS_REV:
            if (sol_del < level->delete_budget)  allowed[n++] = SOL_OP_DELETE;
            if (sol_ins < level->insert_budget)  allowed[n++] = SOL_OP_INSERT;
            if (sol_rev < level->reverse_budget) allowed[n++] = SOL_OP_REVERSE;
            break;

        case CORRUPTION_RANDOM:
        default:
            if (sol_ins < level->insert_budget) allowed[n++] = SOL_OP_INSERT;
            if (sol_del < level->delete_budget)  allowed[n++] = SOL_OP_DELETE;
            if (sol_rev < level->reverse_budget) allowed[n++] = SOL_OP_REVERSE;
            if (sol_swp < level->swap_budget)    allowed[n++] = SOL_OP_SWAP;
            break;
    }

    if (n == 0) return -1;
    return allowed[challenge_rand(rng) % n];
}

/*
 * Generate one solution operation and apply its corruption (inverse) to tape.

 * The corruption is the inverse of the solution operation:
 *   Solution INSERT -> Corruption DELETE (remove base from tape)
 *   Solution DELETE -> Corruption INSERT (add random base to tape)
 *   Solution REVERSE -> Corruption REVERSE (self-inverse)
 *   Solution SWAP -> Corruption SWAP (self-inverse)

 * Positions are chosen relative to the current tape state, which ensures
 * the forward solution positions are correct for replay.
 
 * The forward operation is stored in *fwd.
 * Returns 1 on success, 0 if the operation can't be applied.
 */
static int generate_one_backward(int sol_type, Sequence *tape,
                                 Instruction *fwd, unsigned int *rng)
{
    switch (sol_type)
    {
        case SOL_OP_INSERT:
        {
            /* Solution = INSERT -> Corruption = DELETE
             * Pick a position, record the base there, then delete it. */
            if (tape->length < 1) return 0;
            int pos = 1 + (int)(challenge_rand(rng) % tape->length);
            char base = tape->data[pos - 1];
            *fwd = (Instruction){.type = OP_INSERT, .pos1 = pos, .base = base};
            if (mutation_apply(OP_DELETE, pos, 0, '\0', tape) != APPLY_SUCCESS)
                return 0;
            return 1;
        }

        case SOL_OP_DELETE:
        {
            /* Solution = DELETE -> Corruption = INSERT
             * Pick a position and a random base, insert it. */
            if (tape->length + 1 > MAX_SEQ_LEN) return 0;
            int pos = 1 + (int)(challenge_rand(rng) % (tape->length + 1));
            char bases[] = {'A', 'T', 'C', 'G'};
            char base = bases[challenge_rand(rng) % 4];
            *fwd = (Instruction){.type = OP_DELETE, .pos1 = pos};
            if (mutation_apply(OP_INSERT, pos, 0, base, tape) != APPLY_SUCCESS)
                return 0;
            return 1;
        }

        case SOL_OP_REVERSE:
        {
            /* Solution = REV -> Corruption = REV (self-inverse) */
            if (tape->length < 2) return 0;
            int max_len = (int)tape->length;
            int len = 2 + (int)(challenge_rand(rng) % (max_len - 1));
            int pos = 1 + (int)(challenge_rand(rng) % (max_len - len + 1));
            *fwd = (Instruction){.type = OP_REVERSE, .pos1 = pos, .pos2 = len};
            if (mutation_apply(OP_REVERSE, pos, len, '\0', tape) != APPLY_SUCCESS)
                return 0;
            return 1;
        }

        case SOL_OP_SWAP:
        {
            /* Solution = SWP -> Corruption = SWP (self-inverse)
             * Need two different positions with different bases. */
            if (tape->length < 2) return 0;
            int slen = (int)tape->length;
            for (int tries = 0; tries < 30; tries++)
            {
                int p1 = 1 + (int)(challenge_rand(rng) % slen);
                int p2 = 1 + (int)(challenge_rand(rng) % slen);
                if (p1 == p2) continue;
                if (tape->data[p1 - 1] == tape->data[p2 - 1]) continue;
                *fwd = (Instruction){.type = OP_SWAP, .pos1 = p1, .pos2 = p2};
                if (mutation_apply(OP_SWAP, p1, p2, '\0', tape) != APPLY_SUCCESS)
                    continue;
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

/* Generate a random challenge using backward constructive generation. (Retries up to 50 times before giving up.) */
static int build_random_challenge(Challenge *ch, const LevelDefinition *level, int seed)
{
    for (int attempt = 0; attempt < 50; attempt++)
    {
        unsigned int rng = (unsigned int)(seed + attempt * 137 + level->level_id * 42);

        /* Step 1: Generate target protein */
        int range = level->target_codon_count_max - level->target_codon_count_min + 1;
        int target_count = level->target_codon_count_min
                         + (int)(challenge_rand(&rng) % range);

        strcpy(ch->target[0], "MET");
        for (int i = 1; i < target_count - 1; i++)
            strcpy(ch->target[i],
                   AMINO_ACIDS[challenge_rand(&rng) % NUM_AMINO_ACIDS]);
        strcpy(ch->target[target_count - 1], "STOP");
        ch->target_count = target_count;

        /* Step 2: Generate valid target DNA from protein */
        int tape_len = 3 * target_count;
        Sequence *final_tape = sequence_create((size_t)tape_len);
        if (!final_tape) continue;

        bool build_failed = false;
        for (int i = 0; i < target_count; i++)
        {
            const char *options[8];
            int n_opts = get_dna_codons(ch->target[i], options);
            if (n_opts == 0) { build_failed = true; break; }
            const char *chosen = options[challenge_rand(&rng) % n_opts];
            memcpy(&final_tape->data[i * 3], chosen, 3);
        }
        if (build_failed) { sequence_destroy(final_tape); continue; }

        final_tape->data[tape_len] = '\0';
        final_tape->length = (size_t)tape_len;

        /* Verify target DNA actually produces the target protein */
        ExecutionResult test_exec = challenge_execute(ch, final_tape);
        if (test_exec.diagnostic != DIAG_SUCCESS)
        {
            sequence_destroy(final_tape);
            continue;
        }

        /* Step 3: Determine number of solution operations */
        int num_ops = level->intended_depth;
        if (level->level_id == 7)
            num_ops = 1 + (int)(challenge_rand(&rng) % 2);
        else if (level->level_id == 15)
            num_ops = 4;  // Level 15 must require all four tools

        /* Step 4: Backward constructive generation */
        size_t capacity = (size_t)(tape_len + MAX_PROGRAM_LENGTH + 1);
        Sequence *tape = sequence_create(capacity);
        if (!tape) { sequence_destroy(final_tape); continue; }
        memcpy(tape->data, final_tape->data, (size_t)tape_len + 1);
        tape->length = (size_t)tape_len;

        Instruction temp_ops[MAX_PROGRAM_LENGTH];
        int temp_count = 0;
        int sol_del = 0, sol_ins = 0, sol_rev = 0, sol_swp = 0;
        bool gen_ok = true;

        for (int i = 0; i < num_ops; i++)
        {
            int sol_type = choose_sol_type(level,
                                           sol_del, sol_ins, sol_rev, sol_swp,
                                           &rng);
            if (sol_type < 0) { gen_ok = false; break; }

            Instruction fwd;
            if (!generate_one_backward(sol_type, tape, &fwd, &rng))
            {
                gen_ok = false;
                break;
            }

            /* Store in generation order (last solution op first) */
            temp_ops[temp_count++] = fwd;

            switch (sol_type)
            {
                case SOL_OP_INSERT:  sol_ins++; break;
                case SOL_OP_DELETE:  sol_del++; break;
                case SOL_OP_REVERSE: sol_rev++; break;
                case SOL_OP_SWAP:    sol_swp++; break;
            }
        }

        if (!gen_ok || temp_count == 0)
        {
            sequence_destroy(tape);
            sequence_destroy(final_tape);
            continue;
        }

        /* Build the solution program in forward order (reverse of generation) */
        PlayerProgram solution;
        program_init(&solution);
        for (int i = temp_count - 1; i >= 0; i--)
            program_add(&solution, &temp_ops[i]);

        /* tape is now the initial (corrupted) DNA */
        ch->initial_tape = tape;
        ch->final_tape = final_tape;

        /* Step 5: Verify forward solution replays correctly */
        Sequence *replay = sequence_copy(ch->initial_tape);
        bool replay_ok = true;
        for (int i = 0; i < solution.count; i++)
        {
            Instruction *inst = &solution.instructions[i];
            if (mutation_apply(inst->type, inst->pos1, inst->pos2,
                               inst->base, replay) != APPLY_SUCCESS)
            {
                replay_ok = false;
                break;
            }
        }
        if (!replay_ok || !sequence_equal(replay, ch->final_tape))
        {
            sequence_destroy(replay);
            sequence_destroy(ch->initial_tape);
            sequence_destroy(ch->final_tape);
            ch->initial_tape = NULL;
            ch->final_tape = NULL;
            continue;
        }
        sequence_destroy(replay);

        /* Check that initial tape is not already solved */
        {
            ExecutionResult init_exec = challenge_execute(ch, ch->initial_tape);
            if (init_exec.diagnostic == DIAG_SUCCESS)
            {
                sequence_destroy(ch->initial_tape);
                sequence_destroy(ch->final_tape);
                ch->initial_tape = NULL;
                ch->final_tape = NULL;
                continue;
            }
        }

        /* Step 6: Calculate inventory from solution */
        int slack_del = (sol_del > 0 && level->level_id <= 6) ? 1 : 0;
        int slack_ins = (sol_ins > 0 && level->level_id <= 6) ? 1 : 0;
        int slack_rev = (sol_rev > 0 && level->level_id <= 6) ? 1 : 0;
        int slack_swp = (sol_swp > 0 && level->level_id <= 6) ? 1 : 0;

        Inventory inv;
        inventory_init(&inv,
                       sol_del + slack_del,
                       sol_ins + slack_ins,
                       sol_rev + slack_rev,
                       sol_swp + slack_swp);

        /* Cap at level budget */
        if (inv.delete_count > level->delete_budget)
            inv.delete_count = level->delete_budget;
        if (inv.insert_count > level->insert_budget)
            inv.insert_count = level->insert_budget;
        if (inv.reverse_count > level->reverse_budget)
            inv.reverse_count = level->reverse_budget;
        if (inv.swap_count > level->swap_budget)
            inv.swap_count = level->swap_budget;

        ch->inventory = inv;
        ch->solution = solution;

        /* Step 7: Validate depth with BFS solver */
        if (solver_validate_challenge(ch))
        {
            const SolverResult *sr = solver_get_last_result();

            /* Enforce intended depth */
            if (sr->min_depth == level->intended_depth)
                return 1;

            /* Allow flexible depth for levels 7 and 15 */
            if (level->level_id == 7
                && sr->min_depth >= 1 && sr->min_depth <= 2)
                return 1;

            if (level->level_id == 15
                && sr->min_depth >= 4 && sr->min_depth <= 4)
                return 1;
        }

        /* Cleanup before retry */
        sequence_destroy(ch->initial_tape);
        sequence_destroy(ch->final_tape);
        ch->initial_tape = NULL;
        ch->final_tape = NULL;
    }

    return 0;
}

Challenge *challenge_generate(int seed, int difficulty)
{
    if (difficulty < 1 || difficulty > MAX_DIFFICULTY)
        return NULL;

    const LevelDefinition *level = level_get(difficulty);
    if (!level) return NULL;

    Challenge *ch = calloc(1, sizeof(Challenge));
    if (!ch) return NULL;

    ch->difficulty = difficulty;
    ch->seed = seed;
    ch->level_def = level;

    /* Try canonical mode first */
    if (build_level_challenge(ch, level))
        return ch;

    /* Fall back to random mode */
    if (build_random_challenge(ch, level, seed))
        return ch;

    /* Both modes failed */
    challenge_destroy(ch);
    return NULL;
}

ExecutionResult challenge_execute(const Challenge *ch, const Sequence *tape)
{
    ExecutionResult result;

    if (!ch || !tape)
    {
        execution_result_reset(&result);
        result.diagnostic = DIAG_FRAME_ERROR;
        strcpy(result.message, "Invalid challenge or tape");
        return result;
    }

    const char *target_ptrs[MAX_TARGET_CODONS];
    for (int i = 0; i < ch->target_count; i++)
        target_ptrs[i] = ch->target[i];

    return execution_run(tape, target_ptrs, ch->target_count);
}

void challenge_destroy(Challenge *challenge)
{
    if (!challenge) return;
    if (challenge->final_tape)   sequence_destroy(challenge->final_tape);
    if (challenge->initial_tape) sequence_destroy(challenge->initial_tape);
    free(challenge);
}