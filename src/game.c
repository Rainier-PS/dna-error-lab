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

    sprintf(cmd, sizeof(cmd), "(aplay -q '%s' 2>/dev/null || paplay '%s' 2>/dev/null) &", tmpfile, tmpfile);
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