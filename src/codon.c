#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "codon.h"

/* Strict C11 ASCII case-insensitive comparison helper */
static int c11_strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && (tolower((unsigned char)*s1) == tolower((unsigned char)*s2))) {
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

static const char *const GENETIC_CODE[64] = {
    [0]  = "PHE", [1]  = "PHE", [2]  = "LEU", [3]  = "LEU",
    [4]  = "SER", [5]  = "SER", [6]  = "SER", [7]  = "SER",
    [8]  = "TYR", [9]  = "TYR", [10] = "STOP",[11] = "STOP",
    [12] = "CYS", [13] = "CYS", [14] = "STOP",[15] = "TRP",
    [16] = "LEU", [17] = "LEU", [18] = "LEU", [19] = "LEU",
    [20] = "PRO", [21] = "PRO", [22] = "PRO", [23] = "PRO",
    [24] = "HIS", [25] = "HIS", [26] = "GLN", [27] = "GLN",
    [28] = "ARG", [29] = "ARG", [30] = "ARG", [31] = "ARG",
    [32] = "ILE", [33] = "ILE", [34] = "ILE", [35] = "MET",
    [36] = "THR", [37] = "THR", [38] = "THR", [39] = "THR",
    [40] = "ASN", [41] = "ASN", [42] = "LYS", [43] = "LYS",
    [44] = "SER", [45] = "SER", [46] = "ARG", [47] = "ARG",
    [48] = "VAL", [49] = "VAL", [50] = "VAL", [51] = "VAL",
    [52] = "ALA", [53] = "ALA", [54] = "ALA", [55] = "ALA",
    [56] = "ASP", [57] = "ASP", [58] = "GLU", [59] = "GLU",
    [60] = "GLY", [61] = "GLY", [62] = "GLY", [63] = "GLY"
};

/* AminoAcid enum <-> name mapping */
typedef struct { AminoAcid aa; const char *name; } AANameEntry;

static const AANameEntry AA_NAMES[] = {
    { AA_MET, "MET" }, { AA_PHE, "PHE" }, { AA_LEU, "LEU" },
    { AA_SER, "SER" }, { AA_TYR, "TYR" }, { AA_CYS, "CYS" },
    { AA_TRP, "TRP" }, { AA_THR, "THR" }, { AA_PRO, "PRO" },
    { AA_HIS, "HIS" }, { AA_GLN, "GLN" }, { AA_ARG, "ARG" },
    { AA_ILE, "ILE" }, { AA_ASN, "ASN" }, { AA_LYS, "LYS" },
    { AA_VAL, "VAL" }, { AA_ALA, "ALA" }, { AA_ASP, "ASP" },
    { AA_GLU, "GLU" }, { AA_GLY, "GLY" }, { AA_STOP, "STOP" },
    { AA_UNKNOWN, "UNKNOWN" }
};
#define AA_NAME_COUNT (sizeof(AA_NAMES) / sizeof(AA_NAMES[0]))

const char *amino_acid_name(AminoAcid aa)
{
    for (size_t i = 0; i < AA_NAME_COUNT; i++)
        if (AA_NAMES[i].aa == aa) return AA_NAMES[i].name;
    return "UNKNOWN";
}

AminoAcid amino_acid_from_name(const char *name)
{
    if (!name) return AA_UNKNOWN;
    for (size_t i = 0; i < AA_NAME_COUNT; i++)
        if (c11_strcasecmp(AA_NAMES[i].name, name) == 0)
            return AA_NAMES[i].aa;
    return AA_UNKNOWN;
}

static const char RNA_LOOKUP[4] = {'U', 'C', 'A', 'G'};
static const char DNA_LOOKUP[4] = {'A', 'G', 'T', 'C'};

int base_to_index(char base)
{
    switch (toupper((unsigned char)base)) {
        case 'U': case 'T': return 0;
        case 'C':           return 1;
        case 'A':           return 2;
        case 'G':           return 3;
        default:            return -1;
    }
}

const char *codon_translate(const char *rna_bases)
{
    if (!rna_bases) return "???";
    int v1 = base_to_index(rna_bases[0]);
    int v2 = base_to_index(rna_bases[1]);
    int v3 = base_to_index(rna_bases[2]);

    if (v1 < 0 || v2 < 0 || v3 < 0) return "???";
    return GENETIC_CODE[(v1 << 4) | (v2 << 2) | v3];
}

/* Check for STOP codon */
int codon_is_stop(const char *rna_bases)
{
    return c11_strcasecmp(codon_translate(rna_bases), "STOP") == 0;
}

/* Check for START codon */
int codon_is_start(const char *rna_bases)
{
    if (!rna_bases) return 0;
    return (toupper((unsigned char)rna_bases[0]) == 'A' &&
            toupper((unsigned char)rna_bases[1]) == 'U' &&
            toupper((unsigned char)rna_bases[2]) == 'G');
}

AminoAcid codon_translate_amino(const char *rna_bases)
{
    if (!rna_bases) return AA_UNKNOWN;
    const char *name = codon_translate(rna_bases);
    if (!name) return AA_UNKNOWN;
    return amino_acid_from_name(name);
}

LabReference lab_reference_generate(const char *const *target, int target_count)
{
    LabReference ref = {0};
    if (!target || target_count <= 0) return ref;

    for (int i = 0; i < target_count && ref.entry_count < LAB_REF_MAX_AA; i++) {
        /* Check if already processed */
        int exists = 0;
        for (int k = 0; k < ref.entry_count; k++) {
            if (c11_strcasecmp(ref.entries[k].name, target[i]) == 0) {
                exists = 1;
                break;
            }
        }
        if (exists) continue;

        AAEntry *entry = &ref.entries[ref.entry_count];
        snprintf(entry->name, sizeof(entry->name), "%s", target[i]);
        entry->codon_count = 0;

        for (int idx = 0; idx < 64 && entry->codon_count < LAB_REF_MAX_CODONS; idx++) {
            if (c11_strcasecmp(GENETIC_CODE[idx], target[i]) == 0) {
                int r1 = (idx >> 4) & 3;
                int r2 = (idx >> 2) & 3;
                int r3 = idx & 3;

                CodonMapping *cp = &entry->codons[entry->codon_count++];
                cp->mrna[0] = RNA_LOOKUP[r1];
                cp->mrna[1] = RNA_LOOKUP[r2];
                cp->mrna[2] = RNA_LOOKUP[r3];
                cp->mrna[3] = '\0';
                cp->dna[0]  = DNA_LOOKUP[r1];
                cp->dna[1]  = DNA_LOOKUP[r2];
                cp->dna[2]  = DNA_LOOKUP[r3];
                cp->dna[3]  = '\0';
            }
        }
        ref.entry_count++;
    }
    return ref;
}