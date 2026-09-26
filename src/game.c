#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#include "game.h"
#include "scoring.h"
#include "sequence.h"
#include "mutation.h"
#include "execution.h"
#include "solver.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void game_play_tone(int freq_hz, int duration_ms)
{
    const char *tmpfile = "/tmp/dna_error_lab_tone.wav";
    FILE *f;
    int sample_rate = 22050;
    int num_samples, data_size, chunk_size, byte_rate;
    short fmt = 1, channels = 1, bits = 16, block_align = 2;
    int rate = sample_rate;
    int i;
    double amp = 14000.0;
    char cmd[256];

    f = fopen(tmpfile, "wb");
    if (!f)
    {
        (void)write(STDOUT_FILENO, "\a", 1);
        return;
    }

    num_samples = sample_rate * duration_ms / 1000;
    data_size = num_samples * (int)sizeof(short);
    byte_rate = sample_rate * (int)sizeof(short);
    chunk_size = 36 + data_size;

    fwrite("RIFF", 4, 1, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVE", 4, 1, f);
    fwrite("fmt ", 4, 1, f);
    {
        int s16 = 16;
        fwrite(&s16, 4, 1, f);
    }
    fwrite(&fmt, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits, 2, 1, f);
    fwrite("data", 4, 1, f);
    fwrite(&data_size, 4, 1, f);

    for (i = 0; i < num_samples; i++)
    {
        double t = (double)i / (double)sample_rate;
        double env = 1.0;
        short sample;
        if (i < 80)
            env = (double)i / 80.0;
        if (i > num_samples - 80)
            env = (double)(num_samples - i) / 80.0;
        sample = (short)(amp * env * sin(2.0 * M_PI * freq_hz * t));
        fwrite(&sample, sizeof(short), 1, f);
    }

    fclose(f);

    snprintf(cmd, sizeof(cmd),
             "(aplay -q '%s' 2>/dev/null || paplay '%s' 2>/dev/null) &",
             tmpfile, tmpfile);
    (void)system(cmd);
}

static void game_beep(const GameState *state)
{
    if (state != NULL && state->sound_on)
        game_play_tone(300, 100);
}

static void game_beep_correct(const GameState *state)
{
    if (state != NULL && state->sound_on)
    {
        game_play_tone(523, 80);
        usleep(90000);
        game_play_tone(659, 80);
        usleep(90000);
        game_play_tone(784, 120);
    }
}

typedef struct
{
    const char *title;
    const char *body;
} Lesson;

static const Lesson TUTORIAL_LESSONS[] = {
    {"DNA Basics",
     "DNA is a sequence of four bases:\n"
     "  A (Adenine)\n  T (Thymine)\n  C (Cytosine)\n  G (Guanine)\n\n"
     "The bases pair up: A with T, C with G.\n"
     "This is called complementary pairing."},
    {"Codons",
     "Bases are read in groups of three called codons.\n"
     "Each codon translates to an amino acid.\n\n"
     "  ATG = Met (Start)\n  CCA = Pro\n  TTG = Leu\n\n"
     "There are 64 possible codons and 20 amino acids."},
    {"Transcription",
     "DNA template strand is transcribed into mRNA.\n"
     "The pairing rules are:\n"
     "  DNA A -> mRNA U\n"
     "  DNA T -> mRNA A\n"
     "  DNA C -> mRNA G\n"
     "  DNA G -> mRNA C\n\n"
     "Example:\n"
     "  DNA:  TAC GGC CAG ACT\n"
     "  mRNA: AUG CCG GUC UGA"},
    {"Translation",
     "mRNA codons are translated into amino acids.\n"
     "  AUG -> MET (Start)\n"
     "  CCG -> PRO\n"
     "  GUC -> VAL\n"
     "  UGA -> STOP\n\n"
     "The machine reads codons left to right.\n"
     "STOP terminates the output."},
    {"The Codon Circuit",
     "The Codon Circuit is a deterministic machine.\n\n"
     "You receive:\n"
     "  1. A DNA template tape\n"
     "  2. A target translation\n"
     "  3. A limited instruction inventory\n\n"
     "You modify the DNA using instructions.\n"
     "The machine transcribes and translates.\n"
     "Your goal: make the output match the target."},
    {"DELETE",
     "DEL(p) removes the base at position p.\n\n"
     "Example:\n"
     "  ATGCCA\n"
     "  DEL(4)\n"
     "  ATGCA\n\n"
     "This shifts all bases after position p to the left.\n"
     "It changes the reading frame for downstream codons."},
    {"INSERT",
     "INS(p,b) inserts base b before position p.\n\n"
     "Example:\n"
     "  ATGCA\n"
     "  INS(4,G)\n"
     "  ATGGCA\n\n"
     "Valid bases: A, T, C, G\n"
     "This shifts all bases from position p onward to the right."},
    {"REVERSE",
     "REV(p,l) reverses a segment of length l starting at p.\n\n"
     "Example:\n"
     "  ATCGA\n"
     "  REV(2,3)\n"
     "  AGCTA\n\n"
     "This is a string reversal, not a reverse-complement.\n"
     "It does not change the tape length."},
    {"SWAP",
     "SWP(p1,p2) exchanges the bases at two positions.\n\n"
     "Example:\n"
     "  ATCG\n"
     "  SWP(2,4)\n"
     "  AGCT\n\n"
     "The two positions must be different.\n"
     "It does not change the tape length."},
    {"How to Play",
     "CAMPAIGN MODE - Edit DNA to reach the target!\n\n"
     "  1. Read the TARGET output.\n"
     "  2. Inspect the DNA TAPE.\n"
     "  3. Check your INVENTORY.\n"
     "  4. Enter instructions to edit the tape.\n"
     "  5. Press RUN or STEP to execute.\n\n"
     "KEYBOARD SHORTCUTS:\n"
     "  [D] Delete   [I] Insert   [R] Reverse   [S] Swap\n"
     "  [Enter] RUN  [Space] STEP  [X] RESET\n"
     "  [H] Hint     [Esc] Back    [Q] Quit"},
    {"Reading Frames",
     "The machine reads DNA in groups of 3 from the start.\n\n"
     "  [1-3] [4-6] [7-9] [10-12] ...\n\n"
     "Each group is one codon.\n"
     "If the tape length is not a multiple of 3,\n"
     "the machine reports FRAME ERROR.\n\n"
     "Insertions and deletions shift the reading frame\n"
     "for all downstream codons."},
    {"No-Op Rule",
     "Some instructions leave the tape unchanged.\n"
     "These are called no-ops and are not allowed.\n\n"
     "Examples:\n"
     "  REV(p,1) - reversing a single character\n"
     "  SWP(p,p) - swapping a position with itself\n"
     "  SWP(p1,p2) when S[p1] = S[p2]\n\n"
     "No-ops do not consume inventory."}};

#define TUTORIAL_COUNT (sizeof(TUTORIAL_LESSONS) / sizeof(TUTORIAL_LESSONS[0]))

const char *game_get_tutorial_title(int page)
{
    if (page < 0 || page >= (int)TUTORIAL_COUNT)
        return NULL;
    return TUTORIAL_LESSONS[page].title;
}

const char *game_get_tutorial_body(int page)
{
    if (page < 0 || page >= (int)TUTORIAL_COUNT)
        return NULL;
    return TUTORIAL_LESSONS[page].body;
}

static const char *MAIN_MENU_LABELS[] = {
    "Levels",
    "Simulation",
    "Learn",
    "Help"};

#define MAIN_MENU_COUNT (sizeof(MAIN_MENU_LABELS) / sizeof(MAIN_MENU_LABELS[0]))

int game_main_menu_count(void) { return (int)MAIN_MENU_COUNT; }

const char *game_main_menu_label(int index)
{
    if (index < 0 || index >= (int)MAIN_MENU_COUNT)
        return "";
    return MAIN_MENU_LABELS[index];
}

void game_init(GameState *state)
{
    if (state == NULL)
        return;
    memset(state, 0, sizeof(GameState));
    state->screen = SCREEN_SPLASH;
    state->current_level = 1;
    state->selected_level = 1;
    state->max_level = 15;
    state->tutorial_count = (int)TUTORIAL_COUNT;
    state->settings_cursor = 0;
    state->sim_mode = SIM_MENU;
    state->sound_on = 1;
}

void game_destroy(GameState *state)
{
    if (state == NULL)
        return;
    if (state->current_challenge != NULL)
    {
        challenge_destroy(state->current_challenge);
        state->current_challenge = NULL;
    }
    if (state->working_tape != NULL)
    {
        sequence_destroy(state->working_tape);
        state->working_tape = NULL;
    }
}

void game_new_challenge(GameState *state)
{
    int diff;
    if (state == NULL)
        return;

    /* Clean up previous state */
    if (state->current_challenge != NULL)
    {
        challenge_destroy(state->current_challenge);
        state->current_challenge = NULL;
    }
    if (state->working_tape != NULL)
    {
        sequence_destroy(state->working_tape);
        state->working_tape = NULL;
    }

    /* Reset game state */
    program_init(&state->program);
    state->input_mode = INPUT_MODE_NONE;
    state->exec_mode = EXEC_MODE_NONE;
    memset(&state->exec_result, 0, sizeof(ExecutionResult));
    state->current_codon = 0;
    state->hints_used = 0;
    state->hint_level = 0;
    state->level_score = 0;
    state->lab_ref_active = 0;
    state->help_scroll = 0;
    state->message[0] = '\0';

    diff = state->current_level;
    if (diff > MAX_DIFFICULTY)
        diff = MAX_DIFFICULTY;

    /* Generate the challenge */
    state->current_challenge = challenge_generate(state->seed_counter, diff);
    state->seed_counter++;

    if (state->current_challenge == NULL)
    {
        strcpy(state->message, "Challenge generation failed! Try again.");
        state->message_timer = 120;
        return;
    }

    /* Copy initial tape as working tape */
    state->working_tape = sequence_copy(state->current_challenge->initial_tape);
    if (state->working_tape == NULL)
    {
        strcpy(state->message, "Failed to initialize tape!");
        state->message_timer = 50;
        return;
    }

    /* Copy inventory */
    state->remaining_inventory = state->current_challenge->inventory;

    state->challenge_start = time(NULL);
    state->screen = SCREEN_PLAYING;

    strcpy(state->message, "Challenge loaded! Edit the tape to reach the target.");
    state->message_timer = 40;
}

static void use_hint(GameState *state)
{
    const LevelDefinition *lvl;
    char msg[256];
    int hint_to_show;

    if (state->current_challenge == NULL)
        return;
    lvl = state->current_challenge->level_def;
    if (lvl == NULL)
        return;

    /* Progressive hint system:
     *   - First 4 presses: reveal next hint (1→2→3→4)
     *   - After all revealed: cycle through them for re-reading
     *   - After RESET: start over from hint 1 */
    if (state->hint_level < 4)
    {
        /* Reveal the next hint */
        state->hint_level++;
        state->hints_used++;
    }
    else
    {
        /* All hints revealed - cycle for re-reading.
         * hint_level stays at 4; we show the next one cyclically.
         * Track which hint to show next using hints_used mod 4. */
        state->hints_used++;
    }

    /* Determine which hint to show (1-based, cycles after 4) */
    hint_to_show = ((state->hints_used - 1) % 4) + 1;

    switch (hint_to_show)
    {
    case 1:
        sprintf(msg, "Hint 1: %s", lvl->hint_level_1);
        break;
    case 2:
        sprintf(msg, "Hint 2: %s", lvl->hint_level_2);
        break;
    case 3:
        sprintf(msg, "Hint 3: %s", lvl->hint_level_3);
        break;
    case 4:
        sprintf(msg, "Hint 4: %s", lvl->hint_level_4);
        break;
    default:
        strcpy(msg, "No more hints!");
        break;
    }

    strcpy(state->message, msg);
    state->message_timer = 80;
}

static void start_instruction(GameState *state, InstructionType type)
{
    state->input_mode = INPUT_MODE_POS1;
    state->pending_type = type;
    state->pending_pos1 = 0;
    state->pending_pos2 = 0;
    state->pending_base = '\0';
    state->input_buf[0] = '\0';
    state->input_len = 0;

    switch (type)
    {
    case OP_DELETE:
        strcpy(state->message, "DEL: Enter position (1-based)");
        break;
    case OP_INSERT:
        strcpy(state->message, "INS: Enter position (1-based)");
        break;
    case OP_REVERSE:
        strcpy(state->message, "REV: Enter start position (1-based)");
        break;
    case OP_SWAP:
        strcpy(state->message, "SWP: Enter first position (1-based)");
        break;
    }
    state->message_timer = 60;
}

static void apply_instruction_from_pending(GameState *state)
{
    ApplyResult result;
    Instruction inst;

    if (state->working_tape == NULL)
        return;

    inst.type = state->pending_type;
    inst.pos1 = state->pending_pos1;
    inst.pos2 = state->pending_pos2;
    inst.base = state->pending_base;

    /* Validate */
    if (!mutation_validate(&inst, state->working_tape))
    {
        /* Educational error message (Section 64 of development plan) */
        char errmsg[256];
        int tape_len = (int)state->working_tape->length;
        switch (state->pending_type)
        {
        case OP_DELETE:
            snprintf(errmsg, sizeof(errmsg),
                     "INVALID POSITION\n"
                     "Position %d does not exist.\n"
                     "Valid range: 1-%d (current tape length: %d)",
                     state->pending_pos1, tape_len, tape_len);
            break;
        case OP_INSERT:
            if (state->pending_pos1 < 1 || state->pending_pos1 > tape_len + 1)
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID POSITION\n"
                         "Position %d does not exist.\n"
                         "Valid range: 1-%d (current tape length: %d)",
                         state->pending_pos1, tape_len + 1, tape_len);
            else
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID BASE\n"
                         "'%c' is not a valid DNA base.\n"
                         "Valid bases: A, T, C, G",
                         state->pending_base);
            break;
        case OP_REVERSE:
            if (state->pending_pos2 < 2)
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID LENGTH\n"
                         "REVERSE must affect at least 2 bases.\n"
                         "You entered length %d.",
                         state->pending_pos2);
            else if (state->pending_pos1 < 1 ||
                     state->pending_pos1 + state->pending_pos2 - 1 > tape_len)
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID SEGMENT\n"
                         "REV(%d,%d) goes beyond tape end.\n"
                         "Segment end: %d, Tape length: %d",
                         state->pending_pos1, state->pending_pos2,
                         state->pending_pos1 + state->pending_pos2 - 1, tape_len);
            else
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID REVERSE\n"
                         "Check positions and length.");
            break;
        case OP_SWAP:
            if (state->pending_pos1 == state->pending_pos2)
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID SWAP\n"
                         "Cannot swap a position with itself.");
            else if (state->pending_pos1 < 1 || state->pending_pos1 > tape_len ||
                     state->pending_pos2 < 1 || state->pending_pos2 > tape_len)
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID POSITION\n"
                         "Position does not exist.\n"
                         "Valid range: 1-%d (current tape length: %d)",
                         tape_len, tape_len);
            else
                snprintf(errmsg, sizeof(errmsg),
                         "INVALID SWAP\n"
                         "Check positions.");
            break;
        default:
            strcpy(errmsg, "Invalid instruction.");
            break;
        }
        strcpy(state->message, errmsg);
        state->message_timer = 60;
        state->input_mode = INPUT_MODE_NONE;
        return;
    }

    /* Check no-op */
    if (mutation_is_noop(&inst, state->working_tape))
    {
        /* Educational no-op message (Section 64) */
        char errmsg[256];
        switch (state->pending_type)
        {
        case OP_REVERSE:
            snprintf(errmsg, sizeof(errmsg),
                     "NO-OP\n"
                     "REV(%d,1) does not change the tape.\n"
                     "REVERSE must affect at least 2 bases.",
                     state->pending_pos1);
            break;
        case OP_SWAP:
            if (state->pending_pos1 == state->pending_pos2)
                snprintf(errmsg, sizeof(errmsg),
                         "NO-OP\n"
                         "SWP(%d,%d) swaps a position with itself.\n"
                         "The two positions must be different.",
                         state->pending_pos1, state->pending_pos2);
            else
                snprintf(errmsg, sizeof(errmsg),
                         "NO-OP\n"
                         "The bases at positions %d and %d are already the same.\n"
                         "SWAP only works when the two bases differ.",
                         state->pending_pos1, state->pending_pos2);
            break;
        default:
            strcpy(errmsg, "NO-OP: This instruction would not change the tape.");
            break;
        }
        strcpy(state->message, errmsg);
        state->message_timer = 60;
        state->input_mode = INPUT_MODE_NONE;
        return;
    }

    /* Check inventory */
    if (!inventory_has(&state->remaining_inventory, state->pending_type))
    {
        strcpy(state->message, "No more of this instruction type remaining!");
        state->message_timer = 40;
        state->input_mode = INPUT_MODE_NONE;
        return;
    }

    /* Apply */
    result = mutation_apply(state->pending_type, state->pending_pos1,
                            state->pending_pos2, state->pending_base,
                            state->working_tape);

    if (result == APPLY_SUCCESS)
    {
        inventory_consume(&state->remaining_inventory, state->pending_type);
        program_add(&state->program, &inst);

        /* Format the instruction for the message */
        {
            char inst_buf[64];
            mutation_format_instruction(&inst, inst_buf, sizeof(inst_buf));
            sprintf(state->message, "Applied: %s", inst_buf);
        }
        state->message_timer = 40;

        /* Clear execution state since tape changed */
        state->exec_mode = EXEC_MODE_NONE;
        memset(&state->exec_result, 0, sizeof(ExecutionResult));
    }
    else
    {
        sprintf(state->message, "Failed to apply instruction (error %d)", result);
        state->message_timer = 40;
    }

    state->input_mode = INPUT_MODE_NONE;
}

static void handle_playing_input(GameState *state, int key)
{

    if (state->input_mode == INPUT_MODE_NONE)
    {
        /* Wait for instruction type selection */
        switch (key)
        {
        case 'd':
        case 'D':
            if (inventory_has(&state->remaining_inventory, OP_DELETE))
                start_instruction(state, OP_DELETE);
            else
            {
                strcpy(state->message, "No DELETE instructions remaining!");
                state->message_timer = 40;
            }
            break;
        case 'i':
        case 'I':
            if (inventory_has(&state->remaining_inventory, OP_INSERT))
                start_instruction(state, OP_INSERT);
            else
            {
                strcpy(state->message, "No INSERT instructions remaining!");
                state->message_timer = 40;
            }
            break;
        case 'r':
        case 'R':
            if (inventory_has(&state->remaining_inventory, OP_REVERSE))
                start_instruction(state, OP_REVERSE);
            else
            {
                strcpy(state->message, "No REVERSE instructions remaining!");
                state->message_timer = 40;
            }
            break;
        case 's':
        case 'S':
            if (inventory_has(&state->remaining_inventory, OP_SWAP))
                start_instruction(state, OP_SWAP);
            else
            {
                strcpy(state->message, "No SWAP instructions remaining!");
                state->message_timer = 40;
            }
            break;
        case 'h':
        case 'H':
            use_hint(state);
            break;
        case 'l':
        case 'L':
            state->lab_ref_active = !state->lab_ref_active;
            break;
        case 'x':
        case 'X':
            /* Reset attempt state completely for the same challenge.
             * Do NOT regenerate the challenge. */
            if (state->current_challenge != NULL && state->working_tape != NULL)
            {
                sequence_destroy(state->working_tape);
                state->working_tape = sequence_copy(
                    state->current_challenge->initial_tape);
                state->remaining_inventory = state->current_challenge->inventory;
                program_clear(&state->program);
                state->input_mode = INPUT_MODE_NONE;
                state->exec_mode = EXEC_MODE_NONE;
                memset(&state->exec_result, 0, sizeof(ExecutionResult));
                state->current_codon = 0;
                state->hints_used = 0;
                state->hint_level = 0;
                state->level_score = 0;
                state->challenge_start = time(NULL);
                strcpy(state->message, "Attempt reset. Hint 1 available again.");
                state->message_timer = 40;
            }
            break;
        case ' ': /* Space = STEP */
            if (state->exec_mode == EXEC_MODE_STEP &&
                state->exec_result.trace_count > 0)
            {
                /* Advance to next codon in step mode */
                state->current_codon++;
                if (state->current_codon >= state->exec_result.codon_count)
                {
                    /* Reached the end */
                    /* STEP does NOT award score or mark level solved
                     * (spec section 17). It only shows output. */
                    if (state->exec_result.diagnostic == DIAG_SUCCESS)
                    {
                        sprintf(state->message, "STEP: SUCCESS - protein matches target");
                        game_beep_correct(state);
                    }
                    else
                    {
                        strncpy(state->message, state->exec_result.message, sizeof(state->message) - 1);
                        state->message[sizeof(state->message) - 1] = '\0';
                        game_beep(state);
                    }
                    state->exec_mode = EXEC_MODE_NONE;
                }
                else
                {
                    CodonTrace *ct = &state->exec_result.trace[state->current_codon];
                    sprintf(state->message,
                            "STEP %d/%d: %s -> %s -> %s%s",
                            state->current_codon + 1,
                            state->exec_result.codon_count,
                            ct->dna_triplet, ct->mrna_triplet,
                            ct->actual_output,
                            ct->is_match ? " (MATCH)" : "");
                }
                state->message_timer = 80;
            }
            else if (state->working_tape != NULL && state->current_challenge != NULL)
            {
                /* Start new step execution */
                ExecutionResult result;
                result = challenge_execute(state->current_challenge,
                                           state->working_tape);
                state->exec_result = result;
                state->exec_mode = EXEC_MODE_STEP;
                state->current_codon = 0;
                if (result.trace_count > 0)
                {
                    CodonTrace *ct = &result.trace[0];
                    sprintf(state->message,
                            "STEP 1/%d: %s -> %s -> %s%s",
                            result.codon_count,
                            ct->dna_triplet, ct->mrna_triplet,
                            ct->actual_output,
                            ct->is_match ? " (MATCH)" : "");
                }
                state->message_timer = 80;
            }
            break;
        case '\n':
        case '\r': /* Enter = RUN */
            if (state->working_tape != NULL && state->current_challenge != NULL)
            {
                ExecutionResult result;
                result = challenge_execute(state->current_challenge,
                                           state->working_tape);
                state->exec_result = result;
                state->exec_mode = EXEC_MODE_RUN;
                if (result.diagnostic == DIAG_SUCCESS)
                {
                    int elapsed = (int)(time(NULL) - state->challenge_start);
                    int instructions_used = state->program.count;
                    int score = scoring_calculate(state->current_level,
                                                  state->hints_used,
                                                  elapsed, 1);
                    /* Efficiency bonus: +100 if player used solution depth
                     * or fewer instructions (Section 71 of dev plan) */
                    if (state->current_challenge != NULL)
                    {
                        const SolverResult *sr = solver_get_last_result();
                        if (sr != NULL && sr->is_valid &&
                            instructions_used <= sr->min_depth)
                            score += SCORE_EFFICIENCY_BONUS;
                    }
                    state->level_score = score;
                    state->total_score += score;
                    sprintf(state->message, "SUCCESS! +%d points", score);
                    game_beep_correct(state);
                    state->screen = SCREEN_RESULT;
                }
                else
                {
                    sprintf(state->message, "%s", result.message);
                    game_beep(state);
                }
                state->message_timer = 80;
            }
            break;
        }
        return;
    }

    /* Handle numeric input for positions */
    if (state->input_mode == INPUT_MODE_POS1 ||
        state->input_mode == INPUT_MODE_POS2)
    {
        if (key >= '0' && key <= '9')
        {
            if (state->input_len < 4)
            {
                state->input_buf[state->input_len] = (char)key;
                state->input_len++;
                state->input_buf[state->input_len] = '\0';
            }
        }
        else if (key == 127 || key == GAME_KEY_BACKSPACE)
        {
            if (state->input_len > 0)
            {
                state->input_len--;
                state->input_buf[state->input_len] = '\0';
            }
        }
        else if (key == GAME_KEY_ENTER)
        {
            int val = atoi(state->input_buf);
            if (val <= 0)
            {
                strcpy(state->message, "Invalid position! Enter a positive number.");
                state->message_timer = 40;
                state->input_mode = INPUT_MODE_NONE;
                return;
            }

            if (state->input_mode == INPUT_MODE_POS1)
            {
                state->pending_pos1 = val;
                state->input_buf[0] = '\0';
                state->input_len = 0;

                if (state->pending_type == OP_REVERSE)
                {
                    state->input_mode = INPUT_MODE_POS2;
                    strcpy(state->message, "REV: Enter segment length");
                }
                else if (state->pending_type == OP_SWAP)
                {
                    state->input_mode = INPUT_MODE_POS2;
                    strcpy(state->message, "SWP: Enter second position");
                }
                else if (state->pending_type == OP_INSERT)
                {
                    state->input_mode = INPUT_MODE_BASE;
                    strcpy(state->message, "INS: Enter base (A/T/C/G)");
                }
                else /* DELETE */
                {
                    apply_instruction_from_pending(state);
                    return;
                }
                state->message_timer = 60;
            }
            else /* POS2 */
            {
                state->pending_pos2 = val;
                apply_instruction_from_pending(state);
                return;
            }
        }
        else if (key == GAME_KEY_ESC)
        {
            state->input_mode = INPUT_MODE_NONE;
            strcpy(state->message, "Instruction cancelled.");
            state->message_timer = 30;
        }
        return;
    }

    /* Handle base input for INSERT */
    if (state->input_mode == INPUT_MODE_BASE)
    {
        char base = '\0';
        if (key == 'a' || key == 'A')
            base = 'A';
        else if (key == 't' || key == 'T')
            base = 'T';
        else if (key == 'c' || key == 'C')
            base = 'C';
        else if (key == 'g' || key == 'G')
            base = 'G';
        else if (key == GAME_KEY_ESC)
        {
            state->input_mode = INPUT_MODE_NONE;
            strcpy(state->message, "Instruction cancelled.");
            state->message_timer = 30;
            return;
        }

        if (base != '\0')
        {
            state->pending_base = base;
            apply_instruction_from_pending(state);
            return;
        }
    }
}

static void handle_splash_key(GameState *state, int key)
{
    (void)key;
    state->screen = SCREEN_MAINMENU;
    state->selected_option = 0;
}

static void handle_mainmenu_key(GameState *state, int key)
{
    int last = (int)MAIN_MENU_COUNT - 1;

    if (state->screen != SCREEN_MAINMENU)
        return;

    if (key == GAME_KEY_DOWN || key == 106)
    {
        state->selected_option++;
        if (state->selected_option > last)
            state->selected_option = 0;
    }
    else if (key == GAME_KEY_UP || key == 107)
    {
        state->selected_option--;
        if (state->selected_option < 0)
            state->selected_option = last;
    }
    else if (key == GAME_KEY_ENTER)
    {
        switch (state->selected_option)
        {
        case MENU_CAMPAIGN:
            state->screen = SCREEN_LEVELS;
            state->selected_level = state->current_level;
            break;
        case MENU_SIMULATION:
            state->screen = SCREEN_SIMULATION;
            state->sim_mode = SIM_MENU;
            state->sim_menu_option = 0;
            break;
        case MENU_LEARN:
            state->screen = SCREEN_TUTORIAL;
            state->tutorial_page = 0;
            break;
        case MENU_HELP:
            state->screen = SCREEN_HELP;
            break;
        default:
            break;
        }
    }
    else if (key == 'q' || key == 'Q')
    {
        state->quit = 1;
    }
}

static void handle_tutorial_key(GameState *state, int key)
{
    if (state->screen != SCREEN_TUTORIAL)
        return;
    if (key == GAME_KEY_ESC || key == 'q' || key == 'Q')
    {
        state->screen = SCREEN_MAINMENU;
        state->selected_option = MENU_LEARN;
    }
    else if (key == GAME_KEY_UP || key == 107)
    {
        state->tutorial_page--;
        if (state->tutorial_page < 0)
            state->tutorial_page = state->tutorial_count - 1;
    }
    else if (key == GAME_KEY_DOWN || key == 106)
    {
        state->tutorial_page++;
        if (state->tutorial_page >= state->tutorial_count)
            state->tutorial_page = 0;
    }
}

static void handle_levels_key(GameState *state, int key)
{
    if (state->screen != SCREEN_LEVELS)
        return;
    if (key == GAME_KEY_ESC || key == 'q' || key == 'Q')
    {
        state->screen = SCREEN_MAINMENU;
        state->selected_option = MENU_CAMPAIGN;
    }
    else if (key == GAME_KEY_DOWN || key == 106 || key == GAME_KEY_RIGHT)
    {
        state->selected_level++;
        if (state->selected_level > state->max_level)
            state->selected_level = 1;
    }
    else if (key == GAME_KEY_UP || key == 107 || key == GAME_KEY_LEFT)
    {
        state->selected_level--;
        if (state->selected_level < 1)
            state->selected_level = state->max_level;
    }
    else if (key == GAME_KEY_ENTER || key == '\n' || key == '\r')
    {
        state->current_level = state->selected_level;
        game_new_challenge(state);
    }
}

static void handle_playing_key(GameState *state, int key)
{
    if (state->screen != SCREEN_PLAYING)
        return;
    if (state->current_challenge == NULL)
        return;

    /* If lab reference expanded, ESC collapses it, L toggles */
    if (state->lab_ref_active)
    {
        if (key == GAME_KEY_ESC)
        {
            state->lab_ref_active = 0;
            return;
        }
        if (key == 'l' || key == 'L')
        {
            state->lab_ref_active = 0;
            return;
        }
    }

    if (key == GAME_KEY_ESC)
    {
        state->screen = SCREEN_MAINMENU;
        state->selected_option = MENU_CAMPAIGN;
        return;
    }

    handle_playing_input(state, key);
}

static void handle_result_key(GameState *state, int key)
{
    if (state->screen != SCREEN_RESULT)
        return;
    if (key == GAME_KEY_ESC || key == 'q' || key == 'Q')
    {
        state->screen = SCREEN_MAINMENU;
        state->selected_option = MENU_CAMPAIGN;
    }
    else if (key == GAME_KEY_ENTER || key == 'n' || key == 'N')
    {
        if (state->current_level < state->max_level)
        {
            state->current_level++;
            game_new_challenge(state);
        }
        else
            state->screen = SCREEN_GAMEOVER;
    }
}

static void handle_gameover_key(GameState *state, int key)
{
    if (state->screen != SCREEN_GAMEOVER)
        return;
    if (key == GAME_KEY_ESC || key == 'q' || key == 'Q')
        state->quit = 1;
    else if (key == GAME_KEY_ENTER || key == 'r' || key == 'R')
    {
        state->screen = SCREEN_MAINMENU;
        state->current_level = 1;
        state->total_score = 0;
        state->selected_option = MENU_CAMPAIGN;
    }
}

static void handle_settings_key(GameState *state, int key)
{
    if (state->screen != SCREEN_SETTINGS)
        return;
    if (key == GAME_KEY_ESC || key == 'q' || key == 'Q')
    {
        state->screen = SCREEN_MAINMENU;
        state->selected_option = MENU_HELP;
    }
    else if (key == GAME_KEY_UP || key == 107)
    {
        state->settings_cursor--;
        if (state->settings_cursor < 0)
            state->settings_cursor = 2;
    }
    else if (key == GAME_KEY_DOWN || key == 106)
    {
        state->settings_cursor++;
        if (state->settings_cursor > 2)
            state->settings_cursor = 0;
    }
    else if (key == GAME_KEY_LEFT || key == GAME_KEY_RIGHT ||
             key == GAME_KEY_ENTER)
    {
        switch (state->settings_cursor)
        {
        case 0:
            state->color_mode = (state->color_mode + 1) % 3;
            break;
        case 1:
            state->animation_mode = (state->animation_mode + 1) % 3;
            break;
        case 2:
            state->sound_on = !state->sound_on;
            break;
        }
    }
}

static void handle_help_key(GameState *state, int key)
{
    if (state->screen != SCREEN_HELP)
        return;
    if (key == GAME_KEY_ESC || key == 'q' || key == 'Q' || key == GAME_KEY_ENTER)
    {
        state->screen = SCREEN_MAINMENU;
        state->selected_option = MENU_HELP;
        state->help_scroll = 0;
    }
    else if (key == GAME_KEY_UP || key == 107)
    {
        if (state->help_scroll > 0)
            state->help_scroll--;
    }
    else if (key == GAME_KEY_DOWN || key == 106)
    {
        state->help_scroll++;
    }
}

static void start_sim_playing(GameState *state)
{
    /* Create tape from sim_dna_buf */
    if (state->working_tape != NULL)
    {
        sequence_destroy(state->working_tape);
        state->working_tape = NULL;
    }
    state->working_tape = sequence_from_string(state->sim_dna_buf);
    if (state->working_tape == NULL || state->working_tape->length == 0)
    {
        strcpy(state->message, "Invalid DNA sequence!");
        state->message_timer = 60;
        state->sim_mode = SIM_MENU;
        return;
    }
    program_init(&state->program);
    state->input_mode = INPUT_MODE_NONE;
    state->exec_mode = EXEC_MODE_NONE;
    memset(&state->exec_result, 0, sizeof(ExecutionResult));
    state->current_codon = 0;
    /* Simulation mode has unlimited operations. */
    memset(&state->remaining_inventory, 0, sizeof(Inventory));
    state->remaining_inventory.sim_mode = 1;
    state->sim_mode = SIM_PLAYING;
    strcpy(state->message, "Free play: use D/I/R/S to mutate, Enter/Space to execute.");
    state->message_timer = 80;
}

static void handle_simulation_key(GameState *state, int key)
{
    if (state->screen != SCREEN_SIMULATION)
        return;

    /* --- SIM_MENU mode --- */
    if (state->sim_mode == SIM_MENU)
    {
        if (key == GAME_KEY_ESC || key == 'q' || key == 'Q')
        {
            state->screen = SCREEN_MAINMENU;
            state->selected_option = MENU_SIMULATION;
        }
        else if (key == GAME_KEY_DOWN || key == 106)
        {
            state->sim_menu_option = 1 - state->sim_menu_option;
        }
        else if (key == GAME_KEY_UP || key == 107)
        {
            state->sim_menu_option = 1 - state->sim_menu_option;
        }
        else if (key == GAME_KEY_ENTER)
        {
            if (state->sim_menu_option == 0)
            {
                /* Random DNA */
                int len = 9 + (int)((unsigned)time(NULL) % 13); /* 9-21 bases, always valid length */
                /* Ensure length is a multiple of 3 for clean display */
                len = (len / 3) * 3;
                if (len < 9)
                    len = 9;
                {
                    int k;
                    const char bases[] = "ACGT";
                    unsigned int rng = (unsigned int)time(NULL);
                    for (k = 0; k < len; k++)
                    {
                        rng = rng * 1103515245 + 12345;
                        state->sim_dna_buf[k] = bases[(rng >> 16) & 3];
                    }
                    state->sim_dna_buf[len] = '\0';
                    state->sim_dna_len = len;
                }
                start_sim_playing(state);
            }
            else
            {
                /* User input */
                state->sim_mode = SIM_INPUT;
                state->sim_dna_buf[0] = '\0';
                state->sim_dna_len = 0;
                strcpy(state->message, "Enter DNA sequence (A/T/C/G):");
                state->message_timer = 120;
            }
        }
        return;
    }

    /* --- SIM_INPUT mode --- */
    if (state->sim_mode == SIM_INPUT)
    {
        if (key == GAME_KEY_ESC)
        {
            state->sim_mode = SIM_MENU;
            return;
        }
        if (key == 127 || key == GAME_KEY_BACKSPACE)
        {
            if (state->sim_dna_len > 0)
            {
                state->sim_dna_len--;
                state->sim_dna_buf[state->sim_dna_len] = '\0';
            }
        }
        else if (key == GAME_KEY_ENTER && state->sim_dna_len > 0)
        {
            state->sim_dna_buf[state->sim_dna_len] = '\0';
            start_sim_playing(state);
        }
        else
        {
            char c = '\0';
            if (key == 'a' || key == 'A')
                c = 'A';
            else if (key == 't' || key == 'T')
                c = 'T';
            else if (key == 'c' || key == 'C')
                c = 'C';
            else if (key == 'g' || key == 'G')
                c = 'G';
            if (c && state->sim_dna_len < 255)
            {
                state->sim_dna_buf[state->sim_dna_len] = c;
                state->sim_dna_len++;
                state->sim_dna_buf[state->sim_dna_len] = '\0';
            }
        }
        return;
    }

    /* --- SIM_PLAYING mode --- */
    if (state->sim_mode == SIM_PLAYING)
    {
        /* Lab reference toggle */
        if (state->lab_ref_active)
        {
            if (key == GAME_KEY_ESC || key == 'l' || key == 'L')
            {
                state->lab_ref_active = 0;
                return;
            }
        }

        if (key == GAME_KEY_ESC)
        {
            state->sim_mode = SIM_MENU;
            if (state->working_tape != NULL)
            {
                sequence_destroy(state->working_tape);
                state->working_tape = NULL;
            }
            program_init(&state->program);
            state->exec_mode = EXEC_MODE_NONE;
            return;
        }

        /* X = reset tape to original */
        if (key == 'x' || key == 'X')
        {
            if (state->working_tape != NULL)
            {
                sequence_destroy(state->working_tape);
            }
            state->working_tape = sequence_from_string(state->sim_dna_buf);
            program_init(&state->program);
            state->input_mode = INPUT_MODE_NONE;
            state->exec_mode = EXEC_MODE_NONE;
            memset(&state->exec_result, 0, sizeof(ExecutionResult));
            strcpy(state->message, "Tape reset.");
            state->message_timer = 40;
            return;
        }

        /* L = toggle lab reference */
        if (key == 'l' || key == 'L')
        {
            state->lab_ref_active = !state->lab_ref_active;
            return;
        }

        /* H = hint */
        if (key == 'h' || key == 'H')
        {
            strcpy(state->message, "Try DEL/INS/REV/SWP to mutate, then RUN (Enter) or STEP (Space).");
            state->message_timer = 80;
            return;
        }

        /* Reuse the playing input handler for mutations and execution.
         * We temporarily set current_challenge to non-NULL so the
         * handle_playing_input guard passes, then restore it after. */
        {
            Challenge dummy;
            memset(&dummy, 0, sizeof(dummy));
            Challenge *saved = state->current_challenge;
            state->current_challenge = &dummy;

            /* For execution in simulation, we need to run without a target.
             * We build a target from the tape's own output so execution
             * works via the same path. But for free play, we just want
             * to see the output - so we handle Enter/Space directly. */

            /* Handle instruction input modes (POS1, POS2, BASE) */
            if (state->input_mode == INPUT_MODE_POS1 ||
                state->input_mode == INPUT_MODE_POS2)
            {
                state->current_challenge = saved;
                handle_playing_input(state, key);
                return;
            }
            if (state->input_mode == INPUT_MODE_BASE)
            {
                state->current_challenge = saved;
                handle_playing_input(state, key);
                return;
            }

            /* D/I/R/S = start instruction */
            if (key == 'd' || key == 'D' || key == 'i' || key == 'I' ||
                key == 'r' || key == 'R' || key == 's' || key == 'S')
            {
                state->current_challenge = saved;
                handle_playing_input(state, key);
                return;
            }

            state->current_challenge = saved;

            /* Space = STEP, Enter = RUN - handle directly for free play */
            if (state->working_tape != NULL)
            {
                if (key == ' ')
                {
                    /* STEP */
                    if (state->exec_mode == EXEC_MODE_STEP &&
                        state->exec_result.trace_count > 0)
                    {
                        state->current_codon++;
                        if (state->current_codon >= state->exec_result.codon_count)
                        {
                            state->exec_mode = EXEC_MODE_NONE;
                            strcpy(state->message, "Execution complete.");
                        }
                        else
                        {
                            CodonTrace *ct = &state->exec_result.trace[state->current_codon];
                            sprintf(state->message,
                                    "STEP %d/%d: %s -> %s -> %s",
                                    state->current_codon + 1,
                                    state->exec_result.codon_count,
                                    ct->dna_triplet, ct->mrna_triplet,
                                    ct->actual_output);
                        }
                        state->message_timer = 80;
                    }
                    else
                    {
                        /* execution_run with NULL target transcribes
                         * without comparison (used by Simulation mode) */
                        state->exec_result = execution_run(state->working_tape, NULL, 0);
                        state->exec_mode = EXEC_MODE_STEP;
                        state->current_codon = 0;
                        if (state->exec_result.trace_count > 0)
                        {
                            CodonTrace *ct = &state->exec_result.trace[0];
                            sprintf(state->message,
                                    "STEP 1/%d: %s -> %s -> %s",
                                    state->exec_result.codon_count,
                                    ct->dna_triplet, ct->mrna_triplet,
                                    ct->actual_output);
                        }
                        else
                        {
                            state->exec_mode = EXEC_MODE_NONE;
                            strcpy(state->message, state->exec_result.message);
                        }
                        state->message_timer = 80;
                    }
                }
                else if (key == GAME_KEY_ENTER || key == '\n' || key == '\r')
                {
                    /* RUN - execute all codons at once */
                    state->exec_result = execution_run(state->working_tape, NULL, 0);
                    state->exec_mode = EXEC_MODE_RUN;
                    if (state->exec_result.trace_count > 0)
                    {
                        int ci;
                        sprintf(state->message, "Output: ");
                        for (ci = 0; ci < state->exec_result.trace_count; ci++)
                        {
                            int pos = (int)strlen(state->message);
                            snprintf(state->message + pos,
                                     sizeof(state->message) - (size_t)pos,
                                     "%s%s",
                                     ci > 0 ? " -> " : "",
                                     state->exec_result.trace[ci].actual_output);
                        }
                    }
                    else
                    {
                        strcpy(state->message, state->exec_result.message);
                    }
                    state->message_timer = 80;
                }
            }
        }
        return;
    }
}

void game_handle_key(GameState *state, int key)
{
    if (state == NULL)
        return;

    switch (state->screen)
    {
    case SCREEN_SPLASH:
        handle_splash_key(state, key);
        break;
    case SCREEN_MAINMENU:
        handle_mainmenu_key(state, key);
        break;
    case SCREEN_TUTORIAL:
        handle_tutorial_key(state, key);
        break;
    case SCREEN_LEVELS:
        handle_levels_key(state, key);
        break;
    case SCREEN_PLAYING:
        handle_playing_key(state, key);
        break;
    case SCREEN_RESULT:
        handle_result_key(state, key);
        break;
    case SCREEN_GAMEOVER:
        handle_gameover_key(state, key);
        break;
    case SCREEN_SIMULATION:
        handle_simulation_key(state, key);
        break;
    case SCREEN_SETTINGS:
        handle_settings_key(state, key);
        break;
    case SCREEN_HELP:
        handle_help_key(state, key);
        break;
    default:
        break;    }
}