#ifndef CODON_H
#define CODON_H

#include "sequence.h"

/* Maximum amino acid abbreviation length */
#define MAX_AA_LEN 4

typedef enum {
    AA_MET,
    AA_PHE,
    AA_LEU,
    AA_SER,
    AA_TYR,
    AA_CYS,
    AA_TRP,
    AA_THR,
    AA_PRO,
    AA_HIS,
    AA_GLN,
    AA_ARG,
    AA_ILE,
    AA_ASN,
    AA_LYS,
    AA_VAL,
    AA_ALA,
    AA_ASP,
    AA_GLU,
    AA_GLY,
    AA_STOP,
    AA_UNKNOWN
} AminoAcid;

/* Convert AminoAcid enum to uppercase 3-letter name */
const char *amino_acid_name(AminoAcid aa);

/* Convert uppercase name to AminoAcid enum */
AminoAcid amino_acid_from_name(const char *name);

/* Translate a single RNA codon (3 bases) to its amino acid.
 * Returns the amino acid abbreviation string (UPPERCASE). */
const char *codon_translate(const char *rna_bases);

AminoAcid codon_translate_amino(const char *rna_bases);

int codon_is_stop(const char *rna_bases);

int codon_is_start(const char *rna_bases);

#define LAB_REF_MAX_AA 16

#define LAB_REF_MAX_CODONS 8

/* A codon mapping: DNA template triplet -> mRNA codon */
typedef struct {
    char dna[4];        /* DNA template triplet */
    char mrna[4];       /* mRNA codon */
} CodonMapping;

/* Amino acid entry in the lab reference */
typedef struct {
    char name[8];       /* Amino acid abbreviation (e.g., "MET", "GLY") */
    CodonMapping codons[LAB_REF_MAX_CODONS];
    int codon_count;    /* Number of codon mappings for this AA */
} AAEntry;

/* Lab reference for a target protein */
typedef struct {
    AAEntry entries[LAB_REF_MAX_AA];
    int entry_count;    /* Number of unique AA entries (including STOP) */
} LabReference;

/* Generate a lab reference from a target amino acid sequence. */
LabReference lab_reference_generate(const char *const *target, int target_count);

#endif
