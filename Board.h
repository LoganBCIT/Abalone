#ifndef ABALONE_BOARD_H
#define ABALONE_BOARD_H

#include <array>
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <algorithm>

//------------------------------------------------------------------------------
// Occupant and Move Structures
//------------------------------------------------------------------------------

// Simple occupant type: empty, black, or white.
enum class Occupant {
    EMPTY = 0,
    BLACK,
    WHITE
};

// Structure to represent a move.
// 'marbleIndices' holds the indices of the marbles to be moved.
// 'direction' is an integer from 0 to 5 corresponding to a movement direction.
// 'isInline' indicates whether the move is inline (true) or a sidestep (false).
// 'pushCount' can store how many opponent marbles are pushed.
struct Move {
    std::vector<int> marbleIndices;
    int direction;
    bool isInline;
    int pushCount;
};

//------------------------------------------------------------------------------
// Board Class Declaration
//------------------------------------------------------------------------------

class Board {
public:
    static const int NUM_CELLS = 61;     // Exactly 61 valid positions.
    static const int NUM_DIRECTIONS = 6;   // Directions: W, E, NW, NE, SW, SE.

    // Direction offsets (dx, dy) in board coordinates.
    // Order: W=(-1,0), E=(+1,0), NW=(0,+1), NE=(+1,+1), SW=(-1,-1), SE=(0,-1)
    static const std::array<std::pair<int, int>, NUM_DIRECTIONS> DIRECTION_OFFSETS;

    // Next-to-move color.
    Occupant nextToMove = Occupant::BLACK;

    // Board representation: occupant[i] tells who occupies cell index i.
    std::array<Occupant, NUM_CELLS> occupant;
    // Neighbors table: for each cell i, neighbors[i][d] gives the neighbor's index in
    // direction d (or -1 if none exists).
    std::array<std::array<int, NUM_DIRECTIONS>, NUM_CELLS> neighbors;

    //--------------------------------------------------------------------------
    // Public Methods and Constructors
    //--------------------------------------------------------------------------

    // Constructor: initializes coordinate mapping and neighbor table.
    Board();

    // Static function: Converts a board cell's string notation (e.g., "A1", "H5")
    // to its corresponding cell index (0..60). Returns -1 if the notation is invalid.
    static int notationToIndex(const std::string& notation);

    // Converts a cell index to its board notation (e.g., 0 -> "A1").
    static std::string indexToNotation(int idx);

    // Attempts to apply a move on a temporary copy of the board.
    // Returns true if the move is legal (applied without error), false otherwise.
    bool tryMove(const std::vector<int>& group, int direction, Move& move) const;

    // Generate candidate column groups for the given side.
    std::set<std::vector<int>> generateColumnGroups(Occupant side) const;

    // Generate all legal moves for a given side.
    std::vector<Move> generateMoves(Occupant side) const;

    // Apply a move to *this* board (modifies occupant[]).
    void applyMove(const Move& m);

    // Returns the index of the marble in 'group' that is furthest in the given direction.
    // Uses the dot product with the direction offset to decide which marble is the "front."
    int getFrontCell(const std::vector<int>& group, int direction) const;

    // Converts a move into document notation (e.g., "(b, C5, D5) i → NW").
    static std::string moveToNotation(const Move& m, Occupant side);

    // Returns a string representing the board state (e.g., "C5b,D5b,E4b,...").
    std::string toBoardString() const;

    // Hardcoded starting positions.
    void initStandardLayout();
    void initBelgianDaisyLayout();
    void initGermanDaisyLayout();

    // Loads board configuration from an input file.
    // The file should have two lines: the first indicating the next-to-move ('b' or 'w'),
    // the second listing occupied positions with their occupants.
    bool loadFromInputFile(const std::string& filename);

    // Sets the occupant of a cell given its board notation.
    void setOccupant(const std::string& notation, Occupant who);

    // Helper: Sets the occupant of a cell by its index.
    void setOccupant(int index, Occupant who) {
        if (index >= 0 && index < NUM_CELLS) {
            occupant[index] = who;
        }
    }

    // Returns the occupant at a given cell index.
    Occupant getOccupant(int index) const {
        return (index >= 0 && index < NUM_CELLS) ? occupant[index] : Occupant::EMPTY;
    }

private:
    //--------------------------------------------------------------------------
    // Static Mapping and Neighbor Calculation
    //--------------------------------------------------------------------------

    // Flag to indicate if the coordinate mapping has been initialized.
    static bool s_mappingInitialized;
    // Mapping from a packed coordinate (m,y) to a cell index.
    static std::unordered_map<long long, int> s_coordToIndex;
    // Reverse mapping: cell index to coordinate (m,y).
    static std::array<std::pair<int, int>, NUM_CELLS> s_indexToCoord;

    // Initializes the coordinate mapping.
    static void initMapping();

    // Builds the neighbor table using the coordinate mapping.
    void initNeighbors();

    //--------------------------------------------------------------------------
    // Group Generation and Alignment Helpers
    //--------------------------------------------------------------------------

    // Converts a group of marble indices into a canonical (sorted) order.
    static std::vector<int> canonicalizeGroup(const std::vector<int>& group);

    // Recursively collects all connected groups (up to size 3) of marbles of a given side,
    // starting from cell 'current'. The current group is stored in 'group', and valid groups
    // are inserted into 'result'.
    void dfsGroup(int current, Occupant side, std::vector<int>& group,
        std::set<std::vector<int>>& result) const;

    // Checks if all marbles in 'group' are collinear in one of the allowed directions.
    // If so, sets 'alignedDirection' (0..5) to that direction and returns true;
    // otherwise, returns false.
    bool isGroupAligned(const std::vector<int>& group, int& alignedDirection) const;

    // (Alternate helper) Returns the alignment direction for a contiguous group.
    // If the group is not aligned, returns -1.
    int getGroupAlignmentDirection(const std::vector<int>& group) const;

    //--------------------------------------------------------------------------
    // Coordinate Conversion Helpers (Private)
    //--------------------------------------------------------------------------

    // Converts a coordinate pair (m,y) to board notation (e.g., "A1").
    std::string indexToNotation(const std::pair<int, int>& coord) const;

    // Given a coordinate pair, returns the corresponding cell index.
    // Assumes exactly one cell has that coordinate.
    int s_indexToCoordInverse(const std::pair<int, int>& coord) const;
};

#endif // ABALONE_BOARD_H
