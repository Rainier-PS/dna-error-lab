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

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
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
    fwrite("data", 1, 4, f);
    fwrite(&data_size, 4, 1, f);

    for (i = 0; i < num_samples; i++)
    {
        double t = (double)i / (double)sample_rate;
        double env = 1.0;
        short sample;
        if (i < 80)
            env = (double)i / 80.0;
        else if (i > num_samples - 80)
            env = (double)(num_samples - i) / 80.0;
        sample = (short)(amp * env * sin(2.0 * M_PI * freq_hz * t));
        fwrite(&sample, sizeof(short), 1, f);
    }

    fclose(f);

    snprintf(cmd, sizeof(cmd), "(aplay -q '%s' 2>/dev/null || paplay '%s' 2>/dev/null) &", tmpfile, tmpfile);
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
        game_play_tone(784, 80);
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
     "  A (adenine)\n  T (thymine)\n  C (cytosine)\n  G (guanine)\n\n"
     "The bases pair up: A with T, C with G.\n"
     "This is called complementary pairing."},
    {"Codons",
     "Bases are read in groups of three called codons.\n"
     "Each codon translates to an amino acid.\n\n"
     "  ATG = MET (Start)\n  CCA = Pro\n  TTG = Leu\n\n"
     "There are 64 possible codons and 20 amino acids."},
    {"Transcription",
     "DNA template strand is transcribed into mRNA.\n"
     "The pairing rules are:\n"
     "  A (adenine) pairs with U (uracil)\n"
     "  T (thymine) pairs with A (adenine)\n"
     "  C (cytosine) pairs with G (guanine)\n"
     "  G (guanine) pairs with C (cytosine)\n"
     "Example:\n"
     "  DNA:  TAC GGC CAG ACT\n"
     "  mRNA: AUG CCG GUC UGA"},
    {"Translation",
     "mRNA codons are translated into amino acids.\n"
     "  AUG -> MET (Start)\n"
     "  CCG -> PRO\n"
     "  GUC -> VAL\n"
     "  UGA -> STOP\n"
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
     "INS(p, b) inserts base b before position p.\n\n"
     "Example:\n"
     "  ATGCA\n"
     "  INS(4, G)\n"
     "  ATGGCA\n\n"
     "Valid bases: A, T, C, G\n"
     "This shifts all bases from position p onward to the right."},
    {"REVERSE",
     "REV(p, l) reverses a segment of length l starting at p.\n\n"
     "Example:\n"
     "  ATCGA\n"
     "  REV(2, 3)\n"
     "  AGCTA\n\n"
     "This is a string reversal, not a reverse complement.\n"
     "It does not change the tape length."},
     {"SWAP",
     "SWP(p1, p2) exchanges the bases at two positions.\n\n"
     "Example:\n"
     "  ATCG\n"
     "  SWP(2, 4)\n"
     "  AGCT\n\n"
     "The two positions must be different.\n"
     "It does not change the tape length."},
     {"How to Play",
     "LEVEL MODE - Edit DNA to reach the target!\n"
     "  1. Read the TARGET output.\n"
     "  2. Read the DNA template.\n"
     "  3. Check your INVENTORY.\n"
     "  4. Enter instructions to edit the tape.\n"
     "  5. Press RUN or STEP to execute.\n\n"
     "KEYBOARD SHORTCUTS:\n"
     "  [D] Delete   [I] Insert   [R] Reverse   [S] Swap\n"
     "  [ENTER] RUN  [Space] STEP  [X] RESET\n"
     "  [H] Hint     [ESC] Back    [Q] Quit"},
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
     "No-Ops do not consume inventory."}}

#define TUTORIAL_COUNT (sizeof(TUTORIAL_LESSONS) / sizeof(TUTORIAL_LESSONS[0]))

const char *game_get_tutorial_title(int page)
{
    if (page <0 || page >= (int)TUTORIAL_COUNT)
        return NULL;
    return TUTORIAL_LESSONS[page].title;
}

const char *game_get_tutorial_body(int page)
{
    if (page <0 || page >= (int)TUTORIAL_COUNT)
        return NULL;
    return TUTORIAL_LESSONS[page].body;
}

static const char *MAIN_MENU_LABELS[] = {
    "Levels",
    "Simulation",
    "Learn",
    "Help",
};

#define MAIN_MENU_COUNT (sizeof(MAIN_MENU_LABELS) / sizeof(MAIN_MENU_LABELS[0]))

int game_main_menu_count(void) { return (int)MAIN_MENU_COUNT; }