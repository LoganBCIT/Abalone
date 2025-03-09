# Abalone
Abalone game created to develop an effective AI bot system with heuristics for optimized gameplay strategies.

main.cpp:

Input File Processing

The program opens the input file ("Test2.input") and reads it line by line.
Each line is “trimmed” to remove any leading or trailing whitespace.
Only non‑empty lines are collected into a vector. This ensures that any blank lines (such as an accidental blank line at the end of the file) are ignored.
These non‑empty lines are then written into a temporary file ("temp.input"). This temporary file becomes the cleaned input for the board setup.
Board Initialization

A Board object is created.
The board is loaded using the cleaned input from "temp.input". This file contains the initial board configuration as well as an indicator for which team (black or white) is to move next.
Move Generation

The program generates all legal moves for the team indicated by the board’s nextToMove variable. This move generation takes into account both inline moves (which can include push moves) and sidestep moves.
Output Moves and Resulting Boards

For every legal move that has been generated:
The move is converted into a human‑readable notation (for example, using team symbol, the group of marbles involved, and the move direction). This notation is written to "1‑moves.txt".
A copy of the current board is made, and the move is applied to that copy.
The new board state (converted into a standardized string that indicates the occupancy of each cell) is written to "1‑boards.txt".
End of Process

The program finishes execution after processing all moves.


