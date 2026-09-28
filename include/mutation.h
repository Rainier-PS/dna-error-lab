#ifndef MUTATION_H
#define MUTATION_H

#include "sequence.h"

typedef enum {
    OP_DELETE  = 0,
    OP_INSERT  = 1,
    OP_REVERSE = 2,
    OP_SWAP    = 3
} InstructionType;

/* A single instruction in the player's program */
typedef struct {
    InstructionType type;
    int pos1;
    int pos2;
    char base;
} Instruction;

/* Inventory of available instructions */
typedef struct {
    int delete_count;
    int insert_count;
    int reverse_count;
    int swap_count;
    int sim_mode;   // 1 = unlimited operations (Simulation mode)
} Inventory;

/* Player's ordered instruction sequence (program) */
#define MAX_PROGRAM_LENGTH 32

typedef struct {
    Instruction instructions[MAX_PROGRAM_LENGTH];
    int count;
} PlayerProgram;

/* Result of applying a single instruction */
typedef enum {
    APPLY_SUCCESS = 0,
    APPLY_INVALID_POSITION,
    APPLY_INVALID_BASE,
    APPLY_NO_OP,
    APPLY_NO_INVENTORY,
    APPLY_BUFFER_FULL
} ApplyResult;

/* Returns 1 if the instruction is valid for the given tape, 0 otherwise. */
int mutation_validate(const Instruction *inst, const Sequence *tape);

/* Returns 1 if the instruction is a no-op (leaves tape unchanged). */
int mutation_is_noop(const Instruction *inst, const Sequence *tape);

ApplyResult mutation_apply(InstructionType type, int pos1, int pos2, char base, Sequence *tape);

int inventory_has(const Inventory *inv, InstructionType type);

int inventory_consume(Inventory *inv, InstructionType type);

void inventory_init(Inventory *inv, int del, int ins, int rev, int swp);

void program_init(PlayerProgram *prog);

int program_add(PlayerProgram *prog, const Instruction *inst);

void program_clear(PlayerProgram *prog);

const char *mutation_type_name(InstructionType type);

int mutation_format_instruction(const Instruction *inst, char *buf, size_t bufsize);

#endif