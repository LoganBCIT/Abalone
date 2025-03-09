# Abalone
Abalone game created to develop an effective AI bot system with heuristics for optimized gameplay strategies.

IMPORTANT:

Before attempting to run, you must open your project after connecting to a WSL environment.


HOW TO RUN:

g++ -std=c++17 main.cpp Board.cpp -o abalone
./abalone Test1.input  // type out what input file you want to use


with DEBUG messages:

g++ -std=c++17 -DDEBUG main.cpp Board.cpp -o abalone
./abalone Test1.input


How to compare the output to the desired output:

g++ -std=c++17 compareBoards.cpp -o compareBoards
./compareBoards Test2.board boards.txt moves.txt



main.cpp:

Command Line Input:

The program starts by checking if an input filename was provided as a command-line argument. If not, it prints a usage message and exits. This allows the user to specify which file contains the board configuration instead of having it hardcoded.
Reading and Cleaning the Input File:

The program opens the specified input file and reads it line by line.
Each line is trimmed to remove leading and trailing whitespace.
Only non-empty lines (after trimming) are stored in a vector. This step removes any blank lines that might otherwise cause issues in parsing the board setup.
Creating a Temporary File:

The cleaned (non-empty) lines are then written to a temporary file called temp.input. This temporary file is used to ensure that the board-loading function only sees the valid, non-empty lines.
Loading the Board:

A Board object is created, and the board configuration is loaded from the temporary file using the loadFromInputFile method.
The first line of the input determines which side (BLACK or WHITE) is next to move, and the second line contains the positions of the marbles.
Generating Legal Moves:

The program calls generateMoves on the Board object, passing in the side that is next to move.
This function computes all legal moves based on the current board state and returns them as a vector of Move objects.
Writing Moves and Resulting Boards to Output Files:

For each legal move, the program does the following:
It converts the move into a human-readable notation (using moveToNotation) and writes it to a file name moves.txt.
It creates a copy of the board, applies the move to that copy, and then writes the resulting board configuration (as a string of occupied cell notations) to a file named boards.txt.
Program Termination:

After processing all moves, the program exits, having produced two output files: one listing the move notations and another showing the resulting board configurations for each move.





Board.cpp

Board.cpp implements all the functionality declared in Board.h. It handles the core logic for representing and manipulating the Abalone board. The file is organized into several logical sections:

Debugging Support
A debug macro (DEBUG_PRINT) is defined at the top. When the preprocessor symbol DEBUG is defined, calls to DEBUG_PRINT(...) output detailed diagnostic messages to the console. This lets you easily toggle verbose logging without changing the main logic.

Helper Functions

occupantToString: 
Converts an occupant (EMPTY, BLACK, WHITE) to a human‑readable string.
trim (static inline): Trims whitespace from a string; used when reading input.


Direction and Opposite Calculation

The file defines the six possible movement directions as pairs of offsets (e.g. West is (-1, 0), East is (+1, 0), etc.) in the order matching the game’s directions. It also defines an array of “opposite” directions so that, for any given direction, you can quickly look up its reverse.



Move Attempt and Generation

tryMove: 

This function tries to perform a move (given a group of marble indices and a direction) on a temporary copy of the board. It prints debugging output (if enabled) showing the candidate group, the alignment check (if the group is aligned, and what its aligned direction is), and then attempts to apply the move. If any errors occur (illegal move conditions), it catches them and returns false. This helps filter out illegal moves while logging details.
generateColumnGroups: This function scans the board to build “column groups” of marbles (groups of 1–3 marbles that are contiguous in a given direction). It prints debug messages for each candidate column group and then returns a set of unique groups.


dfsGroup: 
A recursive depth-first search is used to collect connected groups of marbles. As it explores neighboring cells (based on the board’s neighbor table), it logs each step and builds candidate groups of marbles.

isGroupAligned: 
This checks if a candidate group is “aligned” (i.e. its marbles are collinear in one of the allowed directions). It uses the board’s coordinate mapping and direction offsets. If a group is aligned, it also determines the corresponding direction. Debug messages help indicate which groups pass or fail this check.


canonicalizeGroup: 
This helper sorts a group into a canonical order based on board coordinates. This is used to eliminate duplicate candidate groups when generating moves.

generateMoves: 
This function brings together candidate groups from both the DFS search and column grouping. It deduplicates the candidate groups (using their canonical string representations) and then iterates over every candidate group and every possible direction. For each combination it calls tryMove and collects the moves that succeed. Debug messages log accepted groups (and specially flag candidate groups that might lead to pushes into the B‑row, if applicable) as well as the total count of legal moves generated.
Move Application (applyMove) and Support Functions

applyMove: 
This is one of the most critical functions. It takes a Move (which contains a list of marble indices, a direction, and a flag indicating whether it’s an inline move or a side-step) and updates the board state accordingly.
For inline moves (which may involve pushing opponent marbles), it:
Determines the “front” marble (the one furthest in the move’s direction, using getFrontCell).
Checks the destination cell of that front marble.
If the destination is occupied by an opponent, it enters a push loop to count how many opponent marbles are in line and determine if the push is legal. Detailed debug output prints the count of opponent marbles and the final cell status.
It then moves the opponent marbles (or pushes them off the board if necessary) and finally moves the moving group’s marbles in the correct order.
For side-step moves, it simply moves each marble to its adjacent neighbor in the given direction after verifying that the target cells are empty.

getFrontCell: 
Given a group of marbles and a direction, this function calculates which marble is the “front” by computing a dot product of each marble’s coordinate with the move’s direction offset. The marble with the highest score (i.e. furthest in that direction) is returned.

moveToNotation: 
Converts a Move into a human‑readable string notation (for example, “(b, C5, D5) i → NW”).
toBoardString & indexToNotation: These functions convert the board state into a compact string representation (listing cell notations with occupant codes) and convert a cell index to its board notation, respectively.
Hardcoded Layouts and Input Loading

The file implements functions to initialize the board in different hardcoded layouts (e.g., Standard, Belgian Daisy, German Daisy) by placing marbles at specific cell notations.
loadFromInputFile: Reads an input file (with two lines: the first line indicating which color moves next, and the second listing occupied cells with their occupant letters) and updates the board’s state accordingly.
setOccupant: Sets the occupant for a given cell by its notation.


Mapping and Neighbors

The private section includes functions and static variables that set up a coordinate mapping between board coordinates (e.g., (m, y)) and cell indices.
initMapping: Builds a mapping from coordinates to indices and vice versa.


initNeighbors: 
Using the mapping, this function precomputes, for each cell, its neighbor in every one of the six allowed directions.
Summary

Overall, Board.cpp provides all the internal logic to:

Represent the board state using an array of occupant values.
Translate between human-readable cell notations and internal indices.
Precompute neighbor relationships for efficient move lookups.
Generate candidate moves using DFS and column grouping, and to filter and deduplicate those moves.
Check move legality—including push moves—and update the board state when a move is applied.
Optionally produce detailed debugging output (when compiled with DEBUG defined) to help trace the move generation and application processes.





CompareBoards.cpp

Trimming and Normalization Helpers:

The file defines a helper function trim() that removes leading and trailing whitespace from a string.
The function normalizeBoardLine() splits a board configuration line (a comma‐separated list of cell states), trims each cell token, sorts the tokens in a canonical (alphabetical) order, and then rejoins them with commas. This normalization is used so that two equivalent board configurations (possibly listed in different orders) become identical strings.
File Reading and Writing:

The helper function readLines() reads all lines from a given file (trimming each one) and returns them in a vector.
The function writeNormalizedFile() reads an input file using readLines(), normalizes each line with normalizeBoardLine(), and writes the normalized lines to an output file.
Comparing Board Configurations:

The core function compareBoardsAndMoves() takes three filenames as arguments:
A file containing the desired board configurations.
A file containing the actual board configurations generated by your move generator.
A file containing the move notations (though the moves file is not processed further in this version).
It first normalizes the desired board file and the actual board file (writing them to temporary normalized files).
Both normalized files are then read into vectors.
The normalized board strings are stored in sets to enable set operations.
The program computes:
Legal configurations: the intersection of desired and actual sets.
Missing configurations: those desired configurations that are not present in the actual set.
Illegal (or extra) configurations: those actual configurations that are not in the desired set.
It also counts how many lines in the actual normalized file appear in the desired normalized file.
Output of Comparison:

The program prints to the console:
A list of the unique, normalized board configurations that are legal (i.e. appear in both files).
The total count of legal configurations.
A count of how many lines in the actual board file match the desired board file.
A list of board configurations that are missing (i.e. present in the desired file but not generated in the actual file).
A list of board configurations that are illegal (i.e. present in the actual file but not in the desired file).
This detailed output helps in diagnosing discrepancies between the board states produced by your program and the expected board states.
Main Function:

The main() function checks that exactly three command-line arguments (desired file, actual board file, and moves file) are provided.
It then calls compareBoardsAndMoves() with these filenames.
The program exits after printing all the comparison results.

Example output of compareBoards.cpp:

=== Legal Board Configurations (unique, normalized) ===
B2w,B3w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B2w,C2w,C4w,C5b,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B2w,C3w,C4w,C5b,D3w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B2w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B3w,B4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B3w,C3w,C4w,C5b,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B3w,C3w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B4b,C3w,C4w,C5w,D3w,D4w,D5b,D6w,E4b,E5b,E6b,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B4w,C3w,C4w,C5b,D3w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
B4w,C3w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C2w,C3w,C4w,C5b,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C2w,C3w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C2w,C4w,C5b,D2w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C2w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,C6w,D3w,D4w,D5b,D7w,E4b,E5b,E6b,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,C6w,D3w,D4w,D5b,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D2w,D3w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D2w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,D7w,E4b,E5b,E6b,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E3w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E3w,E4b,E5b,E6b,E7w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,E8b,F4w,F5b,F6b,F7b,F8w,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F3w,F5b,F6b,F7b,F8b,G4w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F3w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,F9w,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,F9w,G5w,G6b,G7w,G8w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G4w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H5b,H6w,H7w,H8w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H5b,H6w,H7w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H9w,I8w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H9w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,I8w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H8w,H9w,I7w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H8w,H9w,I8w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H9w,I7w,I8w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H9w,I8w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,I7w,I8w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,H6b,H7w,H8w,H9w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G9w,H6b,H7w,H8w,H9w,I8w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G8w,G9w,H6b,H7w,H8w,H9w,I7w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G8w,G9w,H6b,H7w,H8w,H9w,I9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G6b,G7w,G8w,G9w,H5w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F5b,F6b,F7b,F8b,G4w,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F5b,F6b,F7b,F8b,G4w,G6b,G7w,G8w,G9w,H5w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6w,H7w,H8w,H9w,I7b
C3w,C4w,C5b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E8w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D7w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D3w,D4w,D5b,D7w,E4b,E5b,E6b,E8w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C3w,C4w,C5b,D4w,D5b,D6w,E3w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C4w,C5b,D3w,D4w,D5b,D6w,E3w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
C4w,C5w,C6b,D3w,D4w,D5b,D6w,E4b,E5b,E6b,E7w,F4w,F5b,F6b,F7b,F8b,G5w,G6b,G7w,G8w,G9w,H6b,H7w,H8w,H9w
Count of legal (unique normalized) configurations: 53

=== Count of lines in 1-boardsNormalized.txt that appear in test1Normalized.board ===
53 out of 53

=== Missing Board Configurations (in desired but not in actual) ===
None
Missing count (as defined): 0

=== Illegal Board Configurations (in actual but not in desired) ===
None