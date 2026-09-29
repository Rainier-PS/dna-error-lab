#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codon.h"
#include "challenge.h"
#include "levels.h"

/* Convert DNA template base to mRNA base (transcription) */
static char dna_to_mrna(char dna_base)
{
    switch (dna_base)
    {
        case 'A': return 'U';
        case 'T': return 'A';
        case 'C': return 'G';
        case 'G': return 'C';
        default:  return 'N';
    }
}

/* Validate a single lab reference entry */
static int validate_entry(const AAEntry *entry)
{
    int k;
    int errors = 0;

    if (entry->codon_count == 0)
    {
        printf("  FAIL: %s has no codons in reference\n", entry->name);
        return 1;
    }

    for (k = 0; k < entry->codon_count; k++)
    {
        const CodonMapping *cm = &entry->codons[k];
        char transcribed_mrna[4];
        const char *translated_aa;

        /* Check 1: DNA bases are valid */
        if (cm->dna[0] == 'N' || cm->dna[1] == 'N' || cm->dna[2] == 'N')
        {
            printf("  FAIL: %s codon %d has invalid DNA bases: %s\n",
                   entry->name, k, cm->dna);
            errors++;
            continue;
        }

        /* Check 2: Transcription produces correct mRNA */
        transcribed_mrna[0] = dna_to_mrna(cm->dna[0]);
        transcribed_mrna[1] = dna_to_mrna(cm->dna[1]);
        transcribed_mrna[2] = dna_to_mrna(cm->dna[2]);
        transcribed_mrna[3] = '\0';

        if (strcmp(transcribed_mrna, cm->mrna) != 0)
        {
            printf("  FAIL: %s codon %s -> %s (transcription mismatch, expected %s)\n",
                   entry->name, cm->dna, cm->mrna, transcribed_mrna);
            errors++;
            continue;
        }

        /* Check 3: mRNA translates to the correct amino acid */
        translated_aa = codon_translate(cm->mrna);

        /* GENETIC_CODE now returns UPPERCASE names */
        if (strcmp(translated_aa, entry->name) != 0)
        {
            printf("  FAIL: %s codon %s -> %s -> %s (expected %s)\n",
                   entry->name, cm->dna, cm->mrna, translated_aa, entry->name);
            errors++;
        }
    }

    return errors;
}

int main(void)
{
    int level;
    int total_pass = 0;
    int total_fail = 0;
    int seeds_to_try[] = {42, 100, 999, 12345, 54321};
    int num_seeds = 5;

    printf("Lab Reference Validation\n\n");

    for (level = 1; level <= 15; level++)
    {
        int s;
        for (s = 0; s < num_seeds; s++)
        {
            Challenge *ch;
            LabReference ref;
            int seed = seeds_to_try[s];
            int i, errors = 0;
            int total_codons = 0;
            int j;

            ch = challenge_generate(seed, level);
            if (ch == NULL)
            {
                printf("Level %2d [seed=%5d]: SKIPPED (generation failed)\n",
                       level, seed);
                continue;
            }

            /* Generate lab reference from the target */
            {
                const char *target_ptrs[MAX_TARGET_CODONS];
                for (i = 0; i < ch->target_count; i++)
                    target_ptrs[i] = ch->target[i];
                ref = lab_reference_generate(target_ptrs, ch->target_count);
            }

            /* Check 1: Every target amino acid has entries */
            for (i = 0; i < ch->target_count; i++)
            {
                int found = 0;
                for (j = 0; j < ref.entry_count; j++)
                {
                    if (strcmp(ref.entries[j].name, ch->target[i]) == 0)
                    {
                        found = 1;
                        break;
                    }
                }
                if (!found)
                {
                    printf("Level %2d [seed=%5d]: FAIL - target AA '%s' not in reference\n",
                           level, seed, ch->target[i]);
                    errors++;
                }
            }

            /* Check 2: Every displayed codon is biologically correct */
            for (i = 0; i < ref.entry_count; i++)
            {
                errors += validate_entry(&ref.entries[i]);
                total_codons += ref.entries[i].codon_count;
            }

            /* Check 3: No duplicate amino acid entries */
            for (j = 0; j < ref.entry_count; j++)
            {
                int k;
                for (k = j + 1; k < ref.entry_count; k++)
                {
                    if (strcmp(ref.entries[j].name, ref.entries[k].name) == 0)
                    {
                        printf("Level %2d [seed=%5d]: FAIL - duplicate AA '%s' in reference\n",
                               level, seed, ref.entries[j].name);
                        errors++;
                    }
                }
            }

            if (errors == 0)
            {
                printf("Level %2d [seed=%5d]: PASS (%d AAs, %d codons total)\n",
                       level, seed, ref.entry_count, total_codons);
                total_pass++;
            }
            else
            {
                printf("Level %2d [seed=%5d]: FAIL (%d errors)\n",
                       level, seed, errors);
                total_fail++;
            }

            challenge_destroy(ch);
        }
    }

    printf("\nTOTAL: %d/%d passed\n", total_pass, total_pass + total_fail);

    return (total_fail > 0) ? 1 : 0;
}
