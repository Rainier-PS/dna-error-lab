# DNA Error Lab

#### Video Demo:

## Description

DNA Error Lab is a terminal puzzle game made using C. The player gets a corrupted DNA sequence, a target protein, and a small inventory of mutation operations. The player's job is to repair the DNA so that when the game runs it through simplified biology pipeline, the output protein matches the target.

A level gives three things. First, a DNA tape, for example `TACCCGCACT`. Second, a target protein such as `MET -> GLY -> STOP`. Third, an inventory, for example one DELETE and nothing else. The player types instructions like D then 6 (which means `DEL` or `DELETE` the base at position 6 or `DEL(6)`) to edit the tape. Pressing RUN executes the tape: the game transcribes  the DNA to mRNA, reads it in three-base codons, and translates each codon into an amino acid. If the produced protein matches the target, the level is solved. Because several codons can translate to the same amino acid, the game compares proteins rather than requiring one exact DNA string. For example, both `ACT` and `ATT` transcribe to stop codons, so either ending is accepted.

The mutations are INSERT, DELETE, REVERSE, and SWAP. They are ordinary string edits, but every position refers to the tape as it is right now. If you delete a base at position 8 first, a later REVERSE of positions 8 to 20 means something different, That is what makes later levels hard: they need several operations in one working order.

The game is built as a puzzle, not a simulation (though there is a dedicated freeworld mode). The actual biological concept is intentionally simplified.

## How to Build and Run

The game needs a C compiler and Make. It runs on Linux, macOS, or WSL 2 on Windows.

```bash
make        # build the game
make run    # build and start the game
make dev    # build and run the developer audit
make clean  # remove the build folder
```

No additional libraries is needed to be installed. The terminal interface uses termbox2, which is included in `third-party`. Running `make` produces the executable `build/dna-error-lab`.

## How to play

1. Start the game and press any key to pass the splash screen.
2. Choose levels from the main menu.
3. Pick a level. The play screen shows the target protein, the DNA tape, and the inventory.
4. Press D, I, R, S to start DELETE, INSERT, REVERSE, or SWAP then type the position numbers. (press Enter after each number). For insert, finish by typing the base letter.
5. Press Space to STEP through the execution one codon at a time.
6. Press Enter to RUN the whole tape and get the verdict.
7. Press X to RESET the level. The tape and inventory are restored, the same puzzle stays.
8. Press H for a hint if you are stuck. Press L to expand the lab reference panel, which shows which DNA triplets map to which mRNA codons.

INSERT places a base before the given position, DELETE removes the base at one, REVERSE flips a segment of a given length, and SWAP exchanges the bases at two positions. Each level gives the player a limited inventory of mutation operations, and the counters drain as they are used. The game rejects instructions that would change nothing, for example swapping two identical bases, and those do not consume inventory.

## Game Rules

The DNA tape is treated as a template strand. The machine transcribes it with the complement pairing: A becomes U, T becomes A, C becomes G, and G becomes C. The mRNA is read leftto right in codones of three bases, and each codon translates into an amino acid using the standard genetic code. The output is compared with the target protein.

The machine reports specific diagnostics. A FRAME ERROR means the tape length is not divisible by three. A PREMATURE STOP means a stop codon appeared before the end. A CODON MISS means one codon produced the wrong amino acid. A RUN-ON means the final codon is not a stop. A successfull run produces exactly the target protein, starting with MET and ending with STOP.

This is an intentionally simplified model for a programming/logic puzzle, not a real genetics or medical tool.

## Game Levels

There are 15 deterministic levels, so every level is the same puzzle every time you play it. The progression teaches one idea at a time. Levels 1 to 2 cover INSERT. Levels 3 and 4 instroduce DELETE and more careful sequence editing. Levels 5 to 7 combine DELETE and INSERT. Level 8 introduces REVERSE and level 9 introduces SWAP. Levels 10 to 14 need two or three step plans that mix operation types. Level 15 requires all four operations in a working order.

## Project Files

All source lives in `src/` with matching headers in `include/`. Each header declares one module's interface, and the `.c` file implements it.

- `main.c` is the entry point. It parses the `--dev` flag, then initializes the game state and the terminal, runs the main loop, and cleans up in reverse order.
- `sequence.c` is the DNA tape itself: a dynmically allocated character array with create, copy, insert, delete, reverse, swap, and comparison functions.
- `mutation.c` defines the four instruction types, validates them, detects no-ops, and applies them to a tape. It also owns the inventory counters and the player's program list.
- `codon.c` holds the genetic code table and translates an mRNA codon into an amino acid. It also builds the lab reference panel data.
- `execution.c` is the evaluation engine. It transcribes the tape, translates each codon, compares the result with the target, and reports one of the five diagnostics with a codon-by-codon trace.
- `challenge.c` builds a playable challenge from level data. It verifies that the target DNA produces the target protein, that the starting tape is actually broken, and that the canonical solution replays correctly. Random generation exists as a fallback.
- `levels.c` contains the 15 level definitions: target and initial DNA budgets, hints, and the canonical solution steps.
- `scoring.c` calculates points from difficulty, hints, and time.
- `solver.c` is a development tool. It runs a breadth-first search over possible instruction sequences to validate that generated puzzles are solvable. It does not run during normal play.
- `game.c` is the state machine. It tracks the current screen, handles all keyboard input, runs the hint system, and connects input to the mutation and execution modules.
- `tui.c` draws everything with termbox2 and runs the event loop. It contains no game rules. It reads the game state and forwards key presses to `game.c`.

The Makefile comiles each `.c` file into `build/` as an object file and links them into `build/dna-error-lab`. The only external dependency, termbox2, is vendored in `third-party/` and is not my code.

The `tests/` folder containes standalone test programs: `test_canonical.c` verifies all 15 canonical levels, `test_full_audit.c` checks all 64 codons and every diagnostic, and `stress_dev.c` checks 15000 generated puzzles across all levels.

## Design Choices

**C and a terminal UI.** C fits the low-level sequence manipulation used by the game, and the whole game is text-based, so a character grid renders it naturally. termsbox2 is a single-header library vendored in the repository, so no external library installation is needed.

**Protein match instead of exact DNA.** Real genetic code is degenerate: different codons produce the same amino acid. A game about codons should accept any DNA that produces the right protein instead od requiring one exact DNA sequence.

**Limited inventory.** The counters force planning. Without them, most levels would have many trivial solutions.

**Module seperation.** Sequence handling, mutations, translation, evaluation, challenge building, scoring, and the UI are seperate modules with headers as contracts. The game logic never depends on the terminal library, which is why the tests can run the whole game without opening a terminal.