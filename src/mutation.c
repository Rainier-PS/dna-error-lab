#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mutation.h"

const char *mutation_type_name(InstructionType type)
{
    switch (type)
    {
        case OP_DELETE:  return "DELETE";
        case OP_INSERT:  return "INSERT";
        case OP_REVERSE: return "REVERSE";
        case OP_SWAP:    return "SWAP";
        default:         return "UNKNOWN";
    }
}

int mutation_format_instruction(const Instruction *inst, char *buf, size_t bufsize)
{
    if (inst == NULL || buf == NULL || bufsize == 0) return 0;

    switch (inst->type)
    {
        case OP_DELETE:
            return snprintf(buf, bufsize, "DEL(%d)", inst->pos1);
        case OP_INSERT:
            return snprintf(buf, bufsize, "INS(%d,%c)", inst->pos1, inst->base);
        case OP_REVERSE:
            return snprintf(buf, bufsize, "REV(%d,%d)", inst->pos1, inst->pos2);
        case OP_SWAP:
            return snprintf(buf, bufsize, "SWP(%d,%d)", inst->pos1, inst->pos2);
        default:
            return snprintf(buf, bufsize, "???");
    }
}

int mutation_is_noop(const Instruction *inst, const Sequence *tape)
{
    if (inst == NULL || tape == NULL) return 1;

    switch (inst->type)
    {
        case OP_DELETE:
            /* DEL is never a no-op if validated (position exists) */
            return 0;

        case OP_INSERT:
            /* INS is never a no-op if validated */
            return 0;

        case OP_REVERSE:
            /* REV(p,1) is a no-op (reversing a single character) */
            if (inst->pos2 < 2) return 1;
            /* REV(p,l) where l=0 or segment is all same chars? 
             * We only check length < 2 for simplicity */
            return 0;

        case OP_SWAP:
        {
            /* SWP(p,p) is a no-op (same position) */
            if (inst->pos1 == inst->pos2) return 1;
            /* SWP(p1,p2) where S[p1]==S[p2] is a no-op */
            {
                int p1 = inst->pos1 - 1; /* convert to 0-based */
                int p2 = inst->pos2 - 1;
                if (p1 >= 0 && p1 < (int)tape->length &&
                    p2 >= 0 && p2 < (int)tape->length &&
                    tape->data[p1] == tape->data[p2])
                {
                    return 1;
                }
            }
            return 0;
        }

        default:
            return 1;
    }
}

int mutation_validate(const Instruction *inst, const Sequence *tape)
{
    if (inst == NULL || tape == NULL) return 0;

    switch (inst->type)
    {
        case OP_DELETE:
        {
            /* Valid range: 1 <= p <= length */
            int p = inst->pos1;
            return (p >= 1 && p <= (int)tape->length);
        }

        case OP_INSERT:
        {
            /* Valid range: 1 <= p <= length + 1
             * Valid bases: A, T, C, G */
            int p = inst->pos1;
            char b = inst->base;
            if (p < 1 || p > (int)tape->length + 1) return 0;
            if (b != 'A' && b != 'T' && b != 'C' && b != 'G') return 0;
            return 1;
        }

        case OP_REVERSE:
        {
            /* Requirements: l >= 2, 1 <= p, p + l - 1 <= length */
            int p = inst->pos1;
            int l = inst->pos2;
            if (l < 2) return 0;
            if (p < 1) return 0;
            if (p + l - 1 > (int)tape->length) return 0;
            return 1;
        }

        case OP_SWAP:
        {
            /* Requirements: 1 <= p1,p2 <= length, p1 != p2 */
            int p1 = inst->pos1;
            int p2 = inst->pos2;
            if (p1 < 1 || p1 > (int)tape->length) return 0;
            if (p2 < 1 || p2 > (int)tape->length) return 0;
            if (p1 == p2) return 0;
            return 1;
        }

        default:
            return 0;
    }
}

ApplyResult mutation_apply(InstructionType type, int pos1, int pos2, char base, Sequence *tape)
{
    Instruction inst;
    int p1_0, p2_0;

    if (tape == NULL) return APPLY_INVALID_POSITION;

    inst.type = type;
    inst.pos1 = pos1;
    inst.pos2 = pos2;
    inst.base = base;

    /* Validate */
    if (!mutation_validate(&inst, tape))
        return APPLY_INVALID_POSITION;

    /* Check no-op */
    if (mutation_is_noop(&inst, tape))
        return APPLY_NO_OP;

    switch (type)
    {
        case OP_DELETE:
        {
            /* Convert 1-based to 0-based */
            p1_0 = pos1 - 1;
            if (!sequence_delete(tape, (size_t)p1_0))
                return APPLY_INVALID_POSITION;
            return APPLY_SUCCESS;
        }

        case OP_INSERT:
        {
            /* Convert 1-based to 0-based (insert before position p) */
            p1_0 = pos1 - 1;
            if (!sequence_insert(tape, (size_t)p1_0, base))
                return APPLY_INVALID_POSITION;
            return APPLY_SUCCESS;
        }

        case OP_REVERSE:
        {
            /* Convert 1-based to 0-based */
            p1_0 = pos1 - 1;
            if (!sequence_reverse_segment(tape, (size_t)p1_0, (size_t)pos2))
                return APPLY_INVALID_POSITION;
            return APPLY_SUCCESS;
        }

        case OP_SWAP:
        {
            /* Convert 1-based to 0-based */
            p1_0 = pos1 - 1;
            p2_0 = pos2 - 1;
            if (!sequence_swap_bases(tape, (size_t)p1_0, (size_t)p2_0))
                return APPLY_INVALID_POSITION;
            return APPLY_SUCCESS;
        }

        default:
            return APPLY_INVALID_POSITION;
    }
}

int inventory_has(const Inventory *inv, InstructionType type)
{
    if (inv == NULL) return 0;
    /* Simulation mode has unlimited operations. */
    if (inv->sim_mode) return 1;
    switch (type)
    {
        case OP_DELETE:  return inv->delete_count > 0;
        case OP_INSERT:  return inv->insert_count > 0;
        case OP_REVERSE: return inv->reverse_count > 0;
        case OP_SWAP:    return inv->swap_count > 0;
        default:         return 0;
    }
}

int inventory_consume(Inventory *inv, InstructionType type)
{
    if (inv == NULL) return 0;
    /* Simulation mode never depletes inventory. */
    if (inv->sim_mode) return 1;
    switch (type)
    {
        case OP_DELETE:
            if (inv->delete_count > 0) { inv->delete_count--; return 1; }
            return 0;
        case OP_INSERT:
            if (inv->insert_count > 0) { inv->insert_count--; return 1; }
            return 0;
        case OP_REVERSE:
            if (inv->reverse_count > 0) { inv->reverse_count--; return 1; }
            return 0;
        case OP_SWAP:
            if (inv->swap_count > 0) { inv->swap_count--; return 1; }
            return 0;
        default:
            return 0;
    }
}

void inventory_init(Inventory *inv, int del, int ins, int rev, int swp)
{
    if (inv == NULL) return;
    inv->delete_count = del;
    inv->insert_count = ins;
    inv->reverse_count = rev;
    inv->swap_count = swp;
    inv->sim_mode = 0;
}

void program_init(PlayerProgram *prog)
{
    if (prog == NULL) return;
    prog->count = 0;
    memset(prog->instructions, 0, sizeof(prog->instructions));
}

int program_add(PlayerProgram *prog, const Instruction *inst)
{
    if (prog == NULL || inst == NULL) return 0;
    if (prog->count >= MAX_PROGRAM_LENGTH) return 0;
    prog->instructions[prog->count] = *inst;
    prog->count++;
    return 1;
}

void program_clear(PlayerProgram *prog)
{
    if (prog == NULL) return;
    prog->count = 0;
}