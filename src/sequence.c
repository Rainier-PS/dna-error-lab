#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sequence.h"

// Sequnce creation and destruction

Sequence *sequence_create(size_t capacity)
{
    Sequence *seq = malloc(sizeof(Sequence));
    if (seq == NULL) return NULL;

    seq->data = malloc(capacity + 2);
    if (seq->data == NULL) {
        free(seq);
        return NULL;
    }

    seq->data[0] = '\0';
    seq->length = 0;
    seq->capacity = capacity + 1; // allow one insert beyond requested capacity
    return seq;
}

void sequence_destroy(Sequence *seq)
{
    if (seq == NULL) return;
    free(seq->data);
    free(seq);
}

// Sequence creation from data

Sequence *sequence_from_string(const char *str)
{
    if (str == NULL) return NULL;

    size_t len = strlen(str);
    Sequence *seq = sequence_create(len + 1);
    if (seq == NULL) return NULL;

    for (size_t i = 0; i < len; i++)
    {
        seq->data[i] = (char)toupper((unsigned char)str[i]);
    }

    seq->data[len] = '\0';
    seq->length = len;
    return seq;
}

Sequence *sequence_copy(const Sequence *src)
{
    if (src == NULL) return NULL;

    Sequence *copy = sequence_create(src->capacity);
    if (copy == NULL) return NULL;

    memcpy(copy->data, src->data, src->length);
    copy->data[src->length] = '\0';
    copy->length = src->length;
    return copy;
}

// Validation and comparison

int sequence_validate(const Sequence *seq)
{
    if (seq == NULL || seq->length == 0) return 0;

    for (size_t i = 0; i < seq->length; i++)
    {
        char c = seq->data[i];
        if (c != 'A' && c != 'C' && c != 'G' && c != 'T') return 0;
    }
    return 1;
}

int sequence_equal(const Sequence *a, const Sequence *b)
{
    if (a == NULL || b == NULL || a->length != b->length) return 0;
    return memcmp(a->data, b->data, a->length) == 0;
}

// Sequence mutation (insert and delete)

int sequence_insert(Sequence *seq, size_t pos, char base)
{
    if (seq == NULL || seq->length >= seq->capacity || pos > seq->length) return 0;

    char uc = (char)toupper((unsigned char)base);
    if (uc != 'A' && uc != 'C' && uc != 'G' && uc != 'T') return 0;

    // Shift downstream elements right
    memmove(&seq->data[pos + 1], &seq->data[pos], seq->length - pos);

    seq->data[pos] = uc;
    seq->length++;
    seq->data[seq->length] = '\0';
    return 1;
}

int sequence_delete(Sequence *seq, size_t pos)
{
    if (seq == NULL || pos >= seq->length) return 0;

    // Shift downstream elements left
    memmove(&seq->data[pos], &seq->data[pos + 1], seq->length - pos - 1);

    seq->length--;
    seq->data[seq->length] = '\0';
    return 1;
}

// String conversion
void sequence_to_string(const Sequence *seq, char *buf, size_t bufsize)
{
    if (buf == NULL || bufsize == 0) return;

    if (seq ==NULL)
    {
        buf[0] = '\0';
        return;
    }

    size_t copy_len = (seq->length < bufsize) ? seq->length : bufsize - 1;
    memcpy(buf, seq->data, copy_len);
    buf[copy_len] = '\0';
}

// Segment Operations for Codon Circuit instructions

int sequence_reverse_segment(Sequence *seq, size_t pos, size_t length)
{
    if (seq == NULL || length < 2 || pos + length > seq->length)
    {
        return 0;
    }

    size_t left = pos;
    size_t right = pos + length - 1;

    while (left < right)
    {
        char tmp = seq->data[left];
        seq->data[left] = seq->data[right];
        seq->data[right] = tmp;
        left++;
        right--;
    }
    return 1;
}

int sequence_swap_bases(Sequence *seq, size_t pos1, size_t pos2)
{
    if (seq == NULL || pos1 >= seq->length || pos2 >= seq->length || pos1 == pos2) return 0;

    char tmp = seq->data[pos1];
    seq->data[pos1] = seq->data[pos2];
    seq->data[pos2] = tmp;

    return 1;
}