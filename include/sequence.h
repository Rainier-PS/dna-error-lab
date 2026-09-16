#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <stddef.h>

// Maximum length for any sequence in the game.
#define MAX_SEQ_LEN 256

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} Sequence;

// Positions are 0-based inside the sequence module.
Sequence *sequence_create(size_t capacity);

void sequence_destroy(Sequence *seq);

// Creates a sequence from a string and converts bases to uppercase.
Sequence *sequence_from_string(const char *str);

// Creates an independent copy with its own allocated data.
Sequence *sequence_copy(const Sequence *src);

int sequence_validate(const Sequence *seq);

int sequence_equal(const Sequence *a, const Sequence *b);

int sequence_insert(Sequence *seq, size_t pos, char base);

int sequence_delete(Sequence *seq, size_t pos);

void sequence_to_string(const Sequence *seq, char *buf, size_t bufsize);

// Reverses length bases starting at pos
int sequence_reverse_segment(Sequence *seq, size_t pos, size_t length);

// Swap the bases at two 0-based positions.
int sequence_swap_bases(Sequence *seq, size_t pos1, size_t pos2);

#endif