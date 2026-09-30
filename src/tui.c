/*
AI USAGE DISCLOSURE

I used AI tools during the development of this TUI implementation to understand how termbox2 works, learn how to use its API, and help with coding and debugging terminal UI behaviour.

AI assistance is also used to help me consider whether a terminal UI library such as termbox2 was suitable for this project. I made the final decision to use termbox2 and designed the screens, layout, controls, and user flow myself.
*/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE
#define TB_IMPL
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "termbox2.h"
#include "tui.h"
#include "sequence.h"
#include "mutation.h"
#include "execution.h"
#include "scoring.h"
#include "codon.h"

/* Drawing helpers for boxes, titles, and centered text */

static void draw_hline(int x, int y, int width, uint32_t ch, uintattr_t color)
{
    int i;
    for (i = 0; i < width; i++)
        tb_set_cell(x + i, y, ch, color, TB_DEFAULT);
}

static void draw_box(int x, int y, int w, int h, uintattr_t color)
{
    int i;
    tb_set_cell(x, y, '+', color, TB_DEFAULT);
    tb_set_cell(x + w - 1, y, '+', color, TB_DEFAULT);
    tb_set_cell(x, y + h - 1, '+', color, TB_DEFAULT);
    tb_set_cell(x + w - 1, y + h - 1, '+', color, TB_DEFAULT);
    draw_hline(x + 1, y, w - 2, '-', color);
    draw_hline(x + 1, y + h - 1, w - 2, '-', color);
    for (i = 1; i < h - 1; i++)
    {
        tb_set_cell(x, y + i, '|', color, TB_DEFAULT);
        tb_set_cell(x + w - 1, y + i, '|', color, TB_DEFAULT);
    }
}

static void draw_title(int y, const char *title, int term_width)
{
    int len = (int)strlen(title);
    int x = (term_width - len) / 2;
    if (x < 1)
        x = 1;
    tb_print(x, y, TB_CYAN | TB_BOLD, TB_DEFAULT, title);
}

static void print_centered(int y, const char *text, uintattr_t color, int term_width)
{
    int len = (int)strlen(text);
    int x = (term_width - len) / 2;
    if (x < 1)
        x = 1;
    tb_print(x, y, color, TB_DEFAULT, text);
}

static void draw_splash(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int cx = w / 2;
    int cy = h / 2;
    int i;

    (void)state;
    draw_box(1, 1, w - 2, h - 2, TB_CYAN);

    for (i = 0; i < 6; i++)
    {
        tb_set_cell(cx - 8 + i * 3, cy - 7, 'A', TB_GREEN, TB_DEFAULT);
        tb_set_cell(cx - 7 + i * 3, cy - 7, '-', TB_WHITE, TB_DEFAULT);
        tb_set_cell(cx - 6 + i * 3, cy - 7, 'T', TB_RED, TB_DEFAULT);
    }

    draw_title(cy - 4, "DNA ERROR LAB", w);
    print_centered(cy - 2, "CODON CIRCUIT", TB_GREEN | TB_BOLD, w);
    print_centered(cy + 1, "Edit the sequence. Run the circuit. Reach the target.", TB_YELLOW, w);
    print_centered(cy + 3, "A Computational Biology Puzzle Game", TB_WHITE, w);

    for (i = 0; i < 6; i++)
    {
        tb_set_cell(cx - 8 + i * 3, cy + 6, 'G', TB_GREEN, TB_DEFAULT);
        tb_set_cell(cx - 7 + i * 3, cy + 6, '-', TB_WHITE, TB_DEFAULT);
        tb_set_cell(cx - 6 + i * 3, cy + 6, 'C', TB_RED, TB_DEFAULT);
    }

    print_centered(h - 6, "CS50x 2026 Final Project by Rainier Pearson Saputra", TB_WHITE, w);
    print_centered(h - 4, "Press any key to continue...", TB_GREEN | TB_BOLD, w);
}

static void draw_mainmenu(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int cx = w / 2;
    int i;
    int num_options = game_main_menu_count();
    int btn_w = 30;
    int btn_x = cx - btn_w / 2;
    int btn_start_y = 8;
    int btn_gap = 3;

    draw_box(1, 1, w - 2, h - 2, TB_CYAN);
    draw_title(2, "DNA ERROR LAB", w);
    print_centered(4, "CODON CIRCUIT", TB_GREEN | TB_BOLD, w);

    for (i = 0; i < num_options; i++)
    {
        int y = btn_start_y + i * btn_gap;
        const char *label = game_main_menu_label(i);
        uintattr_t border_color, text_color, bg;
        int j;

        if (i == state->selected_option)
        {
            border_color = TB_GREEN | TB_BOLD;
            text_color = TB_BLACK | TB_BOLD;
            bg = TB_GREEN;
        }
        else
        {
            border_color = TB_CYAN;
            text_color = TB_WHITE;
            bg = TB_DEFAULT;
        }

        for (j = 0; j < btn_w; j++)
            tb_set_cell(btn_x + j, y + 1, ' ', text_color, bg);

        tb_set_cell(btn_x, y, '+', border_color, bg);
        for (j = 1; j < btn_w - 1; j++)
            tb_set_cell(btn_x + j, y, '-', border_color, bg);
        tb_set_cell(btn_x + btn_w - 1, y, '+', border_color, bg);

        tb_set_cell(btn_x, y + 1, '|', border_color, bg);
        tb_set_cell(btn_x + btn_w - 1, y + 1, '|', border_color, bg);

        tb_set_cell(btn_x, y + 2, '+', border_color, bg);
        for (j = 1; j < btn_w - 1; j++)
            tb_set_cell(btn_x + j, y + 2, '-', border_color, bg);
        tb_set_cell(btn_x + btn_w - 1, y + 2, '+', border_color, bg);

        {
            int text_x = btn_x + (btn_w - (int)strlen(label)) / 2;
            tb_print(text_x, y + 1, text_color, bg, label);
        }
    }

    if (state->total_score > 0)
    {
        char buf[64];
        sprintf(buf, "Total Score: %d", state->total_score);
        print_centered(h - 6, buf, TB_YELLOW | TB_BOLD, w);
    }

    print_centered(h - 4, "[Up/Down] Navigate   [Enter] Select   [Q] Quit", TB_WHITE, w);
}

static void draw_levels(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int cx = w / 2;
    char buf[128];
    int i;

    const char *level_names[] = {
        "First Repair",
        "Position Matters",
        "Cut the Error",
        "Find the Break",
        "Wrong Letter",
        "Premature STOP",
        "Repair Protocol",
        "Reverse Engineering",
        "Swap Test",
        "Two-Step Repair",
        "Limited Toolkit",
        "Frame Recovery",
        "Same Protein, Different DNA",
        "Full Repair",
        "Codon Circuit"};

    draw_box(1, 1, w - 2, h - 2, TB_GREEN);
    draw_title(2, "SELECT LEVEL", w);

    sprintf(buf, "Level %d of %d", state->selected_level, state->max_level);
    print_centered(4, buf, TB_WHITE, w);

    if (state->selected_level >= 1 && state->selected_level <= 15)
        print_centered(6, level_names[state->selected_level - 1], TB_YELLOW | TB_BOLD, w);

    {
        int col, row;
        int start_x = cx - 20;
        int start_y = 9;

        for (i = 0; i < state->max_level; i++)
        {
            col = i % 5;
            row = i / 5;
            int lx = start_x + col * 9;
            int ly = start_y + row * 3;
            uintattr_t color;

            if (i + 1 == state->selected_level)
                color = TB_GREEN | TB_BOLD;
            else if (i + 1 <= state->current_level)
                color = TB_WHITE;
            else
                color = TB_WHITE | TB_DIM;

            sprintf(buf, " [%2d] ", i + 1);
            tb_print(lx, ly, color, TB_DEFAULT, buf);
        }
    }

    {
        int diff = state->selected_level;
        int bar_width = 20;
        int filled = (diff * bar_width) / state->max_level;
        char diff_buf[64];
        int pos = 0;
        int j;

        memcpy(diff_buf, "Difficulty: [", 13);
        pos = 13;
        for (j = 0; j < bar_width && pos + 1 < (int)sizeof(diff_buf); j++)
            diff_buf[pos++] = (j < filled) ? '#' : '-';
        diff_buf[pos++] = ']';
        diff_buf[pos] = '\0';
        print_centered(h - 8, diff_buf, TB_WHITE, w);
    }

    if (state->message[0] != '\0' && state->message_timer > 0)
        print_centered(h - 6, state->message, TB_YELLOW, w);

    print_centered(h - 4, "[Up/Down/Left/Right] Navigate   [Enter] Start   [Esc] Back", TB_WHITE, w);
}

static void draw_tutorial(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    char buf[128];
    const char *title, *body;

    draw_box(1, 1, w - 2, h - 2, TB_GREEN);
    draw_title(2, "TUTORIAL", w);

    sprintf(buf, "Lesson %d of %d", state->tutorial_page + 1, state->tutorial_count);
    print_centered(3, buf, TB_WHITE, w);

    title = game_get_tutorial_title(state->tutorial_page);
    body = game_get_tutorial_body(state->tutorial_page);

    if (title != NULL)
        print_centered(5, title, TB_GREEN | TB_BOLD, w);

    if (body != NULL)
    {
        int y = 7, col = 3;
        const char *p = body;
        while (*p != '\0' && y < h - 6)
        {
            if (*p == '\n')
            {
                y++;
                col = 3;
                p++;
            }
            else
            {
                tb_set_cell(col, y, *p, TB_WHITE, TB_DEFAULT);
                col++;
                if (col >= w - 3)
                {
                    y++;
                    col = 3;
                }
                p++;
            }
        }
    }

    print_centered(h - 4, "[Up/Down] Navigate   [Esc] Back", TB_WHITE, w);
}

static void draw_playing(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int y = 3;
    int i;
    char buf[256];
    Challenge *ch;
    Sequence *tape;

    ch = state->current_challenge;
    tape = state->working_tape;
    if (ch == NULL || tape == NULL)
        return;

    draw_box(1, 1, w - 2, h - 2, TB_GREEN);

    sprintf(buf, "DNA ERROR LAB // CODON CIRCUIT    LEVEL %d", state->current_level);
    draw_title(2, buf, w);

    /* Target protein output */
    tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "TARGET:");
    y++;
    {
        int tx = 5;
        for (i = 0; i < ch->target_count; i++)
        {
            uintattr_t color = TB_YELLOW;
            if (strcmp(ch->target[i], "STOP") == 0)
                color = TB_RED | TB_BOLD;
            else if (strcmp(ch->target[i], "MET") == 0)
                color = TB_GREEN | TB_BOLD;

            tb_print(tx, y, color, TB_DEFAULT, ch->target[i]);
            tx += (int)strlen(ch->target[i]) + 3;
            if (i < ch->target_count - 1)
                tb_print(tx - 3, y, TB_WHITE, TB_DEFAULT, "->");
        }
    }
    y += 2;

    /* DNA tape with position indices */
    tb_print(3, y, TB_GREEN | TB_BOLD, TB_DEFAULT, "DNA TAPE:");
    y++;

    {
        int px = 5;
        for (i = 0; i < (int)tape->length && i < 60; i++)
        {
            char num_buf[8];
            sprintf(num_buf, "%02d", i + 1);
            tb_print(px, y, TB_WHITE, TB_DEFAULT, num_buf);
            px += 3;
        }
    }
    y++;

    /* Bases colored by codon group for visual clarity */
    {
        int px = 5;
        for (i = 0; i < (int)tape->length && i < 60; i++)
        {
            uintattr_t color;
            if ((i / 3) % 2 == 0)
                color = TB_GREEN | TB_BOLD;
            else
                color = TB_CYAN;
            tb_set_cell(px, y, tape->data[i], color, TB_DEFAULT);
            if ((i + 1) % 3 == 0 && i < (int)tape->length - 1)
                tb_set_cell(px + 1, y, '|', TB_WHITE | TB_DIM, TB_DEFAULT);
            px += 3;
        }
    }
    y += 2;

    /* Inventory display */
    tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "INVENTORY:");
    y++;
    {
        int ix = 5;
        sprintf(buf, "DELETE x%d", state->remaining_inventory.delete_count);
        tb_print(ix, y, state->remaining_inventory.delete_count > 0 ? TB_WHITE : TB_RED | TB_DIM, TB_DEFAULT, buf);
        ix += 14;
        sprintf(buf, "INSERT x%d", state->remaining_inventory.insert_count);
        tb_print(ix, y, state->remaining_inventory.insert_count > 0 ? TB_WHITE : TB_RED | TB_DIM, TB_DEFAULT, buf);
        ix += 14;
        sprintf(buf, "REVERSE x%d", state->remaining_inventory.reverse_count);
        tb_print(ix, y, state->remaining_inventory.reverse_count > 0 ? TB_WHITE : TB_RED | TB_DIM, TB_DEFAULT, buf);
        ix += 14;
        sprintf(buf, "SWAP x%d", state->remaining_inventory.swap_count);
        tb_print(ix, y, state->remaining_inventory.swap_count > 0 ? TB_WHITE : TB_RED | TB_DIM, TB_DEFAULT, buf);
    }
    y += 2;

    /* Instructions applied so far */
    if (state->program.count > 0)
    {
        tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "PROGRAM:");
        y++;
        {
            int px = 5;
            for (i = 0; i < state->program.count; i++)
            {
                char inst_buf[32];
                mutation_format_instruction(&state->program.instructions[i], inst_buf, sizeof(inst_buf));
                tb_print(px, y, TB_YELLOW, TB_DEFAULT, inst_buf);
                px += (int)strlen(inst_buf) + 2;
                if (px > w - 10)
                {
                    y++;
                    px = 5;
                }
            }
        }
        y++;
    }

    /* Instruction input prompt */
    if (state->input_mode != INPUT_MODE_NONE)
    {
        tb_print(3, y, TB_GREEN | TB_BOLD, TB_DEFAULT, "INPUT:");
        y++;
        sprintf(buf, "> %s", state->input_buf);
        tb_print(5, y, TB_GREEN, TB_DEFAULT, buf);
        tb_set_cell(5 + 2 + state->input_len, y, '_', TB_GREEN | TB_BOLD, TB_DEFAULT);
        y++;
    }

    /* Full execution trace */
    if (state->exec_mode == EXEC_MODE_RUN && state->exec_result.trace_count > 0)
    {
        y++;
        tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "EXECUTION:");
        y++;
        {
            int start = state->exec_result.trace_count - 4;
            if (start < 0)
                start = 0;
            for (i = start; i < state->exec_result.trace_count; i++)
            {
                CodonTrace *ct = &state->exec_result.trace[i];
                uintattr_t match_color = ct->is_match ? TB_GREEN : TB_RED;
                sprintf(buf, "CODON %d: %s -> %s -> %s%s",
                        ct->codon_index, ct->dna_triplet, ct->mrna_triplet,
                        ct->actual_output, ct->is_match ? " OK" : " MISS");
                tb_print(5, y, match_color, TB_DEFAULT, buf);
                y++;
            }
        }
    }

    /* Step-by-step execution display */
    if (state->exec_mode == EXEC_MODE_STEP && state->exec_result.trace_count > 0)
    {
        y++;
        tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "STEP EXECUTION:");
        y++;
        {
            CodonTrace *ct = &state->exec_result.trace[state->current_codon];
            uintattr_t match_color = ct->is_match ? TB_GREEN : TB_RED;
            sprintf(buf, "STEP %d/%d: %s -> %s -> %s",
                    state->current_codon + 1, state->exec_result.codon_count,
                    ct->dna_triplet, ct->mrna_triplet, ct->actual_output);
            tb_print(5, y, match_color | TB_BOLD, TB_DEFAULT, buf);
            y++;
            sprintf(buf, "TARGET:   %s", ct->expected_output);
            tb_print(5, y, TB_YELLOW, TB_DEFAULT, buf);
            y++;
            sprintf(buf, "STATUS:   %s", ct->is_match ? "MATCH" : "MISMATCH");
            tb_print(5, y, ct->is_match ? TB_GREEN : TB_RED, TB_DEFAULT, buf);
        }
    }

    if (state->message[0] != '\0' && state->message_timer > 0)
    {
        y++;
        if (y < h - 4)
            print_centered(y, state->message, TB_YELLOW, w);
    }

    /* Lab reference panel - compact or expanded based on toggle */
    {
        int rx = 3;
        int j;
        LabReference ref;
        const char *target_ptrs[MAX_TARGET_CODONS];
        int compact = !state->lab_ref_active;
        int ref_start_y, ref_lines;

        for (i = 0; i < ch->target_count && i < MAX_TARGET_CODONS; i++)
            target_ptrs[i] = ch->target[i];

        ref = lab_reference_generate(target_ptrs, ch->target_count);

        if (compact)
        {
            ref_lines = 2 + ref.entry_count + 1;
            if (ref_lines < 4)
                ref_lines = 4;
            ref_start_y = h - 2 - ref_lines;

            draw_box(1, ref_start_y - 1, w - 2, ref_lines + 2, TB_YELLOW);
            if (state->lab_ref_active)
                tb_print(2, ref_start_y - 1, TB_YELLOW | TB_BOLD, TB_DEFAULT,
                         "+ LAB REFERENCE (expanded) +");
            else
                tb_print(2, ref_start_y - 1, TB_YELLOW | TB_BOLD, TB_DEFAULT,
                         "+ LAB REFERENCE [L] expand +");

            tb_print(rx, ref_start_y, TB_GREEN | TB_BOLD, TB_DEFAULT,
                     "DNA -> mRNA:  A -> U   T -> A   C -> G   G -> C");
            ref_start_y++;

            for (j = 0; j < ref.entry_count && ref_start_y < h - 3; j++)
            {
                const AAEntry *aa = &ref.entries[j];
                char line_buf[256];
                int pos = 0;
                int k;
                uintattr_t aa_color = TB_WHITE;

                if (strcmp(aa->name, "STOP") == 0)
                    aa_color = TB_RED | TB_BOLD;
                else if (strcmp(aa->name, "MET") == 0)
                    aa_color = TB_GREEN | TB_BOLD;

                pos += snprintf(line_buf + pos, sizeof(line_buf) - (size_t)pos,
                                "%s: ", aa->name);
                for (k = 0; k < aa->codon_count; k++)
                {
                    pos += snprintf(line_buf + pos, sizeof(line_buf) - (size_t)pos,
                                    "%s -> %s ",
                                    aa->codons[k].dna, aa->codons[k].mrna);
                }
                tb_print(rx, ref_start_y, aa_color, TB_DEFAULT, line_buf);
                ref_start_y++;
            }
        }
        else
        {
            int aa_heights[LAB_REF_MAX_AA];
            int total_aa_lines = 0;
            int left_count;
            int left_lines = 0, right_lines = 0;
            int mid_x = w / 2;
            int col1_x = rx;
            int col2_x = mid_x + 2;
            int col_y;
            int max_col_lines;

            for (j = 0; j < ref.entry_count; j++)
            {
                aa_heights[j] = 1 + ref.entries[j].codon_count;
                total_aa_lines += aa_heights[j];
            }

            left_count = (ref.entry_count + 1) / 2;
            for (j = 0; j < left_count; j++)
                left_lines += aa_heights[j];
            for (j = left_count; j < ref.entry_count; j++)
                right_lines += aa_heights[j];

            max_col_lines = left_lines > right_lines ? left_lines : right_lines;
            ref_lines = 3 + max_col_lines;

            ref_start_y = h - 2 - ref_lines;
            if (ref_start_y < 8)
                ref_start_y = 8;

            draw_box(1, ref_start_y - 1, w - 2, ref_lines + 2, TB_YELLOW);
            tb_print(2, ref_start_y - 1, TB_YELLOW | TB_BOLD, TB_DEFAULT,
                     "+ LAB REFERENCE [L] collapse +");

            tb_print(rx, ref_start_y, TB_GREEN | TB_BOLD, TB_DEFAULT, "DNA -> mRNA");
            ref_start_y++;
            tb_print(rx, ref_start_y, TB_WHITE, TB_DEFAULT, "A -> U     T -> A");
            ref_start_y++;
            tb_print(rx, ref_start_y, TB_WHITE, TB_DEFAULT, "C -> G     G -> C");
            ref_start_y++;

            col_y = ref_start_y;
            for (j = 0; j < left_count; j++)
            {
                const AAEntry *aa = &ref.entries[j];
                uintattr_t aa_color = TB_WHITE;
                int k;

                if (strcmp(aa->name, "STOP") == 0)
                    aa_color = TB_RED | TB_BOLD;
                else if (strcmp(aa->name, "MET") == 0)
                    aa_color = TB_GREEN | TB_BOLD;

                {
                    char aa_header[32];
                    sprintf(aa_header, "%s:", aa->name);
                    tb_print(col1_x, col_y, aa_color | TB_BOLD, TB_DEFAULT, aa_header);
                }
                col_y++;
                for (k = 0; k < aa->codon_count; k++)
                {
                    char codon_line[32];
                    sprintf(codon_line, "%s -> %s",
                            aa->codons[k].dna, aa->codons[k].mrna);
                    tb_print(col1_x + 2, col_y, TB_WHITE, TB_DEFAULT, codon_line);
                    col_y++;
                }
            }

            col_y = ref_start_y;
            for (j = left_count; j < ref.entry_count; j++)
            {
                const AAEntry *aa = &ref.entries[j];
                uintattr_t aa_color = TB_WHITE;
                int k;

                if (strcmp(aa->name, "STOP") == 0)
                    aa_color = TB_RED | TB_BOLD;
                else if (strcmp(aa->name, "MET") == 0)
                    aa_color = TB_GREEN | TB_BOLD;

                {
                    char aa_header[32];
                    sprintf(aa_header, "%s:", aa->name);
                    tb_print(col2_x, col_y, aa_color | TB_BOLD, TB_DEFAULT, aa_header);
                }
                col_y++;
                for (k = 0; k < aa->codon_count; k++)
                {
                    char codon_line[32];
                    sprintf(codon_line, "%s -> %s",
                            aa->codons[k].dna, aa->codons[k].mrna);
                    tb_print(col2_x + 2, col_y, TB_WHITE, TB_DEFAULT, codon_line);
                    col_y++;
                }
            }
        }
    }

    if (y < h - 2)
    {
        print_centered(h - 3, "[D] Delete [I] Insert [R] Reverse [S] Swap [Space] Step [Enter] Run [X] Reset [H] Hint [L] Lab Ref [Esc] Back", TB_WHITE | TB_DIM, w);
    }
}

static void draw_simulation(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int y = 3;
    int i;
    char buf[256];

    draw_box(1, 1, w - 2, h - 2, TB_CYAN);
    draw_title(2, "SIMULATION // FREE PLAY", w);

    if (state->sim_mode == SIM_MENU)
    {
        int opts_y = 7;
        print_centered(opts_y, "Choose a DNA source:", TB_WHITE, w);
        opts_y += 2;

        for (i = 0; i < 2; i++)
        {
            const char *labels[] = {"Random DNA", "Enter DNA"};
            uintattr_t color = (i == state->sim_menu_option)
                                   ? (TB_GREEN | TB_BOLD)
                                   : TB_WHITE;
            char opt_buf[64];
            sprintf(opt_buf, "%s %s", i == state->sim_menu_option ? ">" : " ", labels[i]);
            print_centered(opts_y + i * 2, opt_buf, color, w);
        }

        print_centered(h - 4, "[Up/Down] Navigate   [Enter] Select   [Esc] Back", TB_WHITE, w);
        return;
    }

    if (state->sim_mode == SIM_INPUT)
    {
        print_centered(6, "Enter DNA sequence (A/T/C/G):", TB_WHITE, w);
        {
            int inp_x = 5;
            int inp_y = 8;
            draw_box(inp_x - 1, inp_y - 1, w - 8, 3, TB_GREEN);
            tb_print(inp_x, inp_y, TB_GREEN | TB_BOLD, TB_DEFAULT, state->sim_dna_buf);
            tb_set_cell(inp_x + state->sim_dna_len, inp_y, '_', TB_GREEN | TB_BOLD, TB_DEFAULT);
        }
        print_centered(12, "Type A/T/C/G. Enter to confirm. Esc to go back.", TB_WHITE | TB_DIM, w);

        if (state->message[0] != '\0' && state->message_timer > 0)
            print_centered(14, state->message, TB_YELLOW, w);
        return;
    }

    if (state->sim_mode == SIM_PLAYING && state->working_tape != NULL)
    {
        Sequence *tape = state->working_tape;

        if (tape->length > 0)
        {
            tb_print(3, y, TB_GREEN | TB_BOLD, TB_DEFAULT, "DNA TAPE:");
            y++;
            {
                int px = 5;
                for (i = 0; i < (int)tape->length && i < 60; i++)
                {
                    char num_buf[8];
                    sprintf(num_buf, "%02d", i + 1);
                    tb_print(px, y, TB_WHITE, TB_DEFAULT, num_buf);
                    px += 3;
                }
            }
            y++;
            {
                int px = 5;
                for (i = 0; i < (int)tape->length && i < 60; i++)
                {
                    uintattr_t color = ((i / 3) % 2 == 0) ? (TB_GREEN | TB_BOLD) : TB_CYAN;
                    tb_set_cell(px, y, tape->data[i], color, TB_DEFAULT);
                    if ((i + 1) % 3 == 0 && i < (int)tape->length - 1)
                        tb_set_cell(px + 1, y, '|', TB_WHITE | TB_DIM, TB_DEFAULT);
                    px += 3;
                }
            }
        }
        else
        {
            tb_print(3, y, TB_GREEN | TB_BOLD, TB_DEFAULT, "DNA TAPE:");
            y++;
            tb_print(5, y, TB_WHITE | TB_DIM, TB_DEFAULT, "(empty)");
        }
        y += 2;

        /* Unlimited operations in simulation mode */
        {
            int ix = 5;
            tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "OPS:");
            ix += 3;
            tb_print(ix, y, TB_GREEN, TB_DEFAULT, "DEL:INF");
            ix += 10;
            tb_print(ix, y, TB_GREEN, TB_DEFAULT, "INS:INF");
            ix += 10;
            tb_print(ix, y, TB_GREEN, TB_DEFAULT, "REV:INF");
            ix += 10;
            tb_print(ix, y, TB_GREEN, TB_DEFAULT, "SWP:INF");
        }
        y += 2;

        if (state->program.count > 0)
        {
            tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "PROGRAM:");
            y++;
            {
                int px = 5;
                for (i = 0; i < state->program.count; i++)
                {
                    char inst_buf[32];
                    mutation_format_instruction(&state->program.instructions[i], inst_buf, sizeof(inst_buf));
                    tb_print(px, y, TB_YELLOW, TB_DEFAULT, inst_buf);
                    px += (int)strlen(inst_buf) + 2;
                    if (px > w - 10)
                    {
                        y++;
                        px = 5;
                    }
                }
            }
            y++;
        }

        if (state->input_mode != INPUT_MODE_NONE)
        {
            tb_print(3, y, TB_GREEN | TB_BOLD, TB_DEFAULT, "INPUT:");
            y++;
            sprintf(buf, "> %s", state->input_buf);
            tb_print(5, y, TB_GREEN, TB_DEFAULT, buf);
            tb_set_cell(5 + 2 + state->input_len, y, '_', TB_GREEN | TB_BOLD, TB_DEFAULT);
            y++;
        }

        if (state->exec_mode == EXEC_MODE_RUN && state->exec_result.trace_count > 0)
        {
            y++;
            tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "OUTPUT:");
            y++;
            {
                int start = state->exec_result.trace_count - 4;
                if (start < 0)
                    start = 0;
                for (i = start; i < state->exec_result.trace_count; i++)
                {
                    CodonTrace *ct = &state->exec_result.trace[i];
                    sprintf(buf, "CODON %d: %s -> %s -> %s",
                            ct->codon_index, ct->dna_triplet, ct->mrna_triplet,
                            ct->actual_output);
                    tb_print(5, y, TB_WHITE, TB_DEFAULT, buf);
                    y++;
                }
            }
        }

        if (state->exec_mode == EXEC_MODE_STEP && state->exec_result.trace_count > 0)
        {
            y++;
            tb_print(3, y, TB_CYAN | TB_BOLD, TB_DEFAULT, "STEP EXECUTION:");
            y++;
            {
                CodonTrace *ct = &state->exec_result.trace[state->current_codon];
                sprintf(buf, "STEP %d/%d: %s -> %s -> %s",
                        state->current_codon + 1, state->exec_result.codon_count,
                        ct->dna_triplet, ct->mrna_triplet, ct->actual_output);
                tb_print(5, y, TB_WHITE | TB_BOLD, TB_DEFAULT, buf);
            }
        }

        if (state->message[0] != '\0' && state->message_timer > 0)
        {
            y++;
            if (y < h - 4)
                print_centered(y, state->message, TB_YELLOW, w);
        }

        if (y < h - 2)
        {
            print_centered(h - 3, "[D] Delete [I] Insert [R] Reverse [S] Swap [Space] Step [Enter] Run [X] Reset [Esc] Back", TB_WHITE | TB_DIM, w);
        }
    }
}

static void draw_result(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    char buf[128];

    draw_box(1, 1, w - 2, h - 2, TB_GREEN);
    draw_title(2, "CHALLENGE RESULT", w);

    if (state->exec_result.diagnostic == DIAG_SUCCESS)
    {
        print_centered(6, "SUCCESS!", TB_GREEN | TB_BOLD, w);
        sprintf(buf, "Level %d completed!", state->current_level);
        print_centered(8, buf, TB_WHITE, w);
        sprintf(buf, "Score: +%d", state->level_score);
        print_centered(10, buf, TB_YELLOW | TB_BOLD, w);
        print_centered(12, "Instructions used:", TB_WHITE, w);
        sprintf(buf, "%d", state->program.count);
        print_centered(13, buf, TB_GREEN, w);
    }
    else
    {
        print_centered(6, "FAILED", TB_RED | TB_BOLD, w);
        print_centered(8, state->exec_result.message, TB_YELLOW, w);
    }

    print_centered(h - 4, "[Enter/N] Next Level   [Esc/Q] Back to Menu", TB_WHITE, w);
}

static void draw_gameover(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    char buf[64];

    draw_box(1, 1, w - 2, h - 2, TB_CYAN);
    draw_title(2, "CONGRATULATIONS!", w);

    print_centered(6, "You completed all 15 levels!", TB_GREEN | TB_BOLD, w);
    sprintf(buf, "Total Score: %d", state->total_score);
    print_centered(8, buf, TB_YELLOW | TB_BOLD, w);

    print_centered(h - 4, "[Enter/R] Restart   [Esc/Q] Quit", TB_WHITE, w);
}

static void draw_settings(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int y = 6;
    const char *color_modes[] = {"Color", "Monochrome", "Auto"};
    const char *anim_modes[] = {"Full", "Reduced", "Off"};

    draw_box(1, 1, w - 2, h - 2, TB_CYAN);
    draw_title(2, "SETTINGS", w);

    {
        uintattr_t c;
        char buf[64];

        c = (state->settings_cursor == 0) ? TB_GREEN | TB_BOLD : TB_WHITE;
        sprintf(buf, "%s Appearance: %s", state->settings_cursor == 0 ? ">" : " ",
                color_modes[state->color_mode]);
        tb_print(5, y, c, TB_DEFAULT, buf);
        y += 2;

        c = (state->settings_cursor == 1) ? TB_GREEN | TB_BOLD : TB_WHITE;
        sprintf(buf, "%s Animation:  %s", state->settings_cursor == 1 ? ">" : " ",
                anim_modes[state->animation_mode]);
        tb_print(5, y, c, TB_DEFAULT, buf);
        y += 2;

        c = (state->settings_cursor == 2) ? TB_GREEN | TB_BOLD : TB_WHITE;
        sprintf(buf, "%s Sound:      %s", state->settings_cursor == 2 ? ">" : " ",
                state->sound_on ? "ON" : "OFF");
        tb_print(5, y, c, TB_DEFAULT, buf);
        y += 2;
    }

    print_centered(h - 4, "[Up/Down] Navigate   [Left/Right] Change   [Esc] Back", TB_WHITE, w);
}

typedef struct
{
    const char *line;
    uintattr_t color;
} HelpLine;

static const HelpLine HELP_PAGES[] = {
    {"DNA ERROR LAB - HOW IT WORKS", TB_CYAN | TB_BOLD},
    {"", TB_DEFAULT},
    {"1. You receive a DNA template tape and a TARGET output.", TB_WHITE},
    {"2. The machine reads DNA in groups of 3 (codons).", TB_WHITE},
    {"3. Each DNA codon is transcribed to mRNA, then translated.", TB_WHITE},
    {"4. Your goal: edit the DNA so it produces the target output.", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- DNA -> mRNA -> Amino Acid ---", TB_GREEN | TB_BOLD},
    {"", TB_DEFAULT},
    {"Transcription rules (DNA template -> mRNA):", TB_YELLOW},
    {"  DNA A -> mRNA U      DNA T -> mRNA A", TB_WHITE},
    {"  DNA C -> mRNA G      DNA G -> mRNA C", TB_WHITE},
    {"", TB_DEFAULT},
    {"Translation: mRNA codons -> amino acids via genetic code.", TB_WHITE},
    {"  Example: DNA ATG -> mRNA AUG -> MET (Start)", TB_GREEN},
    {"  Example: DNA TGG -> mRNA ACC -> THR", TB_GREEN},
    {"  Example: DNA TAA -> mRNA AUU -> ILE", TB_GREEN},
    {"  STOP codons: UAA, UAG, UGA (terminate output)", TB_RED},
    {"", TB_DEFAULT},
    {"--- Reading the Target ---", TB_CYAN | TB_BOLD},
    {"", TB_DEFAULT},
    {"Target shows: MET -> AA1 -> AA2 -> ... -> STOP", TB_WHITE},
    {"Each entry is one codon's output. Count = number of codons.", TB_WHITE},
    {"Tape length must equal 3 * number of codons.", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- Codons ---", TB_GREEN | TB_BOLD},
    {"", TB_DEFAULT},
    {"Each codon is 3 RNA bases encoding one amino acid.", TB_WHITE},
    {"64 codons map to 20 amino acids + 3 STOP signals.", TB_WHITE},
    {"Multiple codons can encode the same amino acid (degeneracy).", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- DELETE ---", TB_RED | TB_BOLD},
    {"Syntax:  DEL(p)", TB_YELLOW},
    {"Effect:  Remove the base at position p.", TB_WHITE},
    {"Length:  Tape becomes 1 base shorter.", TB_WHITE},
    {"Shift:   All bases after p shift LEFT by 1.", TB_WHITE},
    {"Frame:   Downstream reading frame shifts.", TB_WHITE},
    {"Example: ATCGGA -> DEL(3) -> AT_GGA (T removed)", TB_GREEN},
    {"Useful:  Fix an extra base disrupting the frame.", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- INSERT ---", TB_BLUE | TB_BOLD},
    {"Syntax:  INS(p,b)  where b = A, T, C, or G", TB_YELLOW},
    {"Effect:  Insert base b BEFORE position p.", TB_WHITE},
    {"Length:  Tape becomes 1 base longer.", TB_WHITE},
    {"Shift:   All bases from p onward shift RIGHT by 1.", TB_WHITE},
    {"Frame:   Downstream reading frame shifts.", TB_WHITE},
    {"Example: ATCGGA -> INS(4,T) -> ATCTGGA (T inserted)", TB_GREEN},
    {"Useful:  Restore a missing base.", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- REVERSE ---", TB_MAGENTA | TB_BOLD},
    {"Syntax:  REV(p,l)  where l >= 2", TB_YELLOW},
    {"Effect:  Reverse the order of l bases starting at p.", TB_WHITE},
    {"Length:  Tape length does NOT change.", TB_WHITE},
    {"Shift:   No positional shift - just reverses the segment.", TB_WHITE},
    {"Frame:   Only codons within the segment are affected.", TB_WHITE},
    {"Example: ATCGGA -> REV(2,4) -> AGCGTA (TCGG reversed)", TB_GREEN},
    {"Useful:  Rearrange bases within a segment.", TB_WHITE},
    {"Note:    String reversal, NOT reverse-complement.", TB_RED},
    {"", TB_DEFAULT},
    {"--- SWAP ---", TB_YELLOW | TB_BOLD},
    {"Syntax:  SWP(p1,p2)  where p1 != p2", TB_YELLOW},
    {"Effect:  Exchange the bases at positions p1 and p2.", TB_WHITE},
    {"Length:  Tape length does NOT change.", TB_WHITE},
    {"Shift:   No shift - only two bases are exchanged.", TB_WHITE},
    {"Frame:   Only the two codons containing p1 and p2.", TB_WHITE},
    {"Example: ATCGGA -> SWP(1,4) -> GTCAGA (A and G swapped)", TB_GREEN},
    {"Useful:  Fix two specific misplaced bases.", TB_WHITE},
    {"No-op:   If the two bases are already the same.", TB_RED},
    {"", TB_DEFAULT},
    {"--- Reading Diagnostics ---", TB_CYAN | TB_BOLD},
    {"", TB_DEFAULT},
    {"SUCCESS              All codons match the target.", TB_GREEN},
    {"FRAME ERROR          Sequence length not divisible by 3.", TB_RED},
    {"PREMATURE STOP       A STOP codon appears too early.", TB_RED},
    {"CODON MISS           A codon produces the wrong AA.", TB_RED},
    {"RUN-ON               Last codon is not STOP.", TB_RED},
    {"", TB_DEFAULT},
    {"--- How to Approach a Puzzle ---", TB_CYAN | TB_BOLD},
    {"", TB_DEFAULT},
    {"1. Read the TARGET and count codons (including STOP).", TB_WHITE},
    {"2. Run STEP or RUN to see current output.", TB_WHITE},
    {"3. Compare actual vs expected codon by codon.", TB_WHITE},
    {"4. Determine which operation type fixes the mismatch.", TB_WHITE},
    {"5. Consider length: too long -> DEL, too short -> INS.", TB_WHITE},
    {"6. Correct length but wrong order? Try REV or SWP.", TB_WHITE},
    {"7. Edit, then RUN again to verify.", TB_WHITE},
    {"8. Use [X] RESET if you get stuck.", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- Complete Pipeline Example ---", TB_CYAN | TB_BOLD},
    {"", TB_DEFAULT},
    {"DNA:     TAC CCA GGT ATT", TB_GREEN},
    {"           |   |   |   |", TB_WHITE},
    {"mRNA:    AUG GGU CCA UAA", TB_YELLOW},
    {"           |   |   |   |", TB_WHITE},
    {"Output:  MET GLY PRO STOP", TB_GREEN | TB_BOLD},
    {"", TB_DEFAULT},
    {"If the target is MET-GLY-PRO-STOP, this is SUCCESS.", TB_WHITE},
    {"If any codon is wrong, the machine reports the error.", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- Common Mistakes ---", TB_RED | TB_BOLD},
    {"", TB_DEFAULT},
    {"- Tape length not a multiple of 3 -> FRAME ERROR", TB_WHITE},
    {"- Fixed first codon but later ones wrong -> check shifts", TB_WHITE},
    {"- Delete at wrong position -> frame shifts differently", TB_WHITE},
    {"- Insert wrong base -> produces wrong amino acid", TB_WHITE},
    {"- Reversing length 1 -> no-op, not allowed", TB_WHITE},
    {"- Swapping same base -> no-op, not allowed", TB_WHITE},
    {"", TB_DEFAULT},
    {"--- Keyboard Shortcuts ---", TB_GREEN | TB_BOLD},
    {"", TB_DEFAULT},
    {"[D] Delete  [I] Insert  [R] Reverse  [S] Swap", TB_WHITE},
    {"[Space] Step  [Enter] Run  [X] Reset  [H] Hint", TB_WHITE},
    {"[Esc] Back", TB_WHITE},
};

#define HELP_LINE_COUNT (sizeof(HELP_PAGES) / sizeof(HELP_PAGES[0]))

static void draw_help(GameState *state)
{
    int w = tb_width();
    int h = tb_height();
    int y = 3;
    int scroll = state->help_scroll;
    int visible = h - 6;
    int i;

    draw_box(1, 1, w - 2, h - 2, TB_CYAN);
    draw_title(2, "HELP / CLUELESS", w);

    if (scroll < 0)
        scroll = 0;
    if (scroll > (int)HELP_LINE_COUNT - visible)
        scroll = (int)HELP_LINE_COUNT - visible;
    if (scroll < 0)
        scroll = 0;

    for (i = 0; i < visible && (scroll + i) < (int)HELP_LINE_COUNT; i++)
    {
        int idx = scroll + i;
        tb_print(3, y + i, HELP_PAGES[idx].color, TB_DEFAULT, HELP_PAGES[idx].line);
    }

    if (scroll > 0)
        print_centered(h - 5, "[Up] scroll up", TB_GREEN | TB_BOLD, w);
    if (scroll + visible < (int)HELP_LINE_COUNT)
        print_centered(h - 5, "[Down] scroll down", TB_GREEN | TB_BOLD, w);

    print_centered(h - 4, "[Up/Down] Scroll   [Esc] Back", TB_WHITE, w);
}

static void draw_stub(GameState *state, const char *mode_name)
{
    (void)state;
    int w = tb_width();
    int h = tb_height();

    draw_box(1, 1, w - 2, h - 2, TB_CYAN);
    draw_title(2, mode_name, w);
    print_centered(6, "Coming soon...", TB_YELLOW, w);
    print_centered(h - 4, "[Esc] Back", TB_WHITE, w);
}

void tui_draw(GameState *state)
{
    if (state == NULL)
        return;

    tb_clear();

    switch (state->screen)
    {
    case SCREEN_SPLASH:
        draw_splash(state);
        break;
    case SCREEN_MAINMENU:
        draw_mainmenu(state);
        break;
    case SCREEN_TUTORIAL:
        draw_tutorial(state);
        break;
    case SCREEN_LEVELS:
        draw_levels(state);
        break;
    case SCREEN_PLAYING:
        draw_playing(state);
        break;
    case SCREEN_RESULT:
        draw_result(state);
        break;
    case SCREEN_GAMEOVER:
        draw_gameover(state);
        break;
    case SCREEN_SETTINGS:
        draw_settings(state);
        break;
    case SCREEN_HELP:
        draw_help(state);
        break;
    case SCREEN_SIMULATION:
        draw_simulation(state);
        break;
    default:
        draw_stub(state, "UNKNOWN");
        break;
    }

    tb_present();
}

void tui_init(void)
{
    tb_init();
    tb_set_input_mode(TB_INPUT_ESC | TB_INPUT_MOUSE);
    tb_set_clear_attrs(TB_DEFAULT, TB_DEFAULT);
}

void tui_shutdown(void)
{
    tb_shutdown();
}

void tui_run(GameState *state)
{
    struct tb_event ev;

    while (!state->quit)
    {
        tui_draw(state);

        if (state->message_timer > 0)
            state->message_timer--;

        {
            int poll_rv = tb_poll_event(&ev);
            if (poll_rv == TB_OK)
            {
                if (ev.type == TB_EVENT_KEY)
                {
                    if (ev.key == TB_KEY_CTRL_C || ev.key == TB_KEY_CTRL_D)
                    {
                        state->quit = 1;
                        break;
                    }

                    game_handle_key(state, ev.ch ? ev.ch : ev.key);
                }
            }
        }
    }
}
