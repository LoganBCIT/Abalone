#ifndef ABALONE_BOARD_H
#define ABALONE_BOARD_H

#include <array>
#include <string>
#include <unordered_map>
#include <vector>


// Simple occupant type: empty, black, white
enum class Occupant
{
    EMPTY = 0,
    BLACK,
    WHITE
};

struct Move {
    // The indices of the marbles being moved. Typically 1–3 contiguous marbles of the same color.
    // For example, {10} for a single marble, or {10,11,12} for three in a line.
    std::vector<int> marbleIndices;

    // Which direction (0..5) - matches Board::DIRECTION_OFFSETS
    int direction;

    // Is it an inline move or sidestep move? (true = inline, false = side-step)
    bool isInline;

    // For convenience, store occupant color if you like
    // Occupant who;
};

class Board
{
public:
    static const int NUM_CELLS = 61;     // Exactly 61 valid positions
    static const int NUM_DIRECTIONS = 6; // E, W, NW, NE, SW, SE

    // You can tweak the direction order as desired.
    // For your problem, directions from your doc:
    //  W = (-1,  0)
    //  E = (+1,  0)
    //  NW= (0,  +1)
    //  NE= (+1, +1)
    //  SW= (-1, -1)
    //  SE= (0,  -1)
    // We'll store them in that order: W=0, E=1, NW=2, NE=3, SW=4, SE=5
    // Each "dxdy" is applied to (m, y).
    static const std::array<std::pair<int, int>, NUM_DIRECTIONS> DIRECTION_OFFSETS;

    Occupant nextToMove = Occupant::BLACK;

    // Generate all legal moves for 'side'
    std::vector<Move> generateMoves(Occupant side) const;

    // Apply a move to *this* board (modifying occupant[]).
    // Alternatively, you can return a new Board if you prefer a copy-on-write style.
    void applyMove(const Move& m);

    // Make a notation string like "(b, 2m) i → NW" given a Move & occupant color
    static std::string moveToNotation(const Move& m, Occupant side);

    // Convert board occupant array to e.g. "C5b,D5b,E4b,..." sorted black first, then white
    std::string toBoardString() const;


    std::string indexToNotation(int idx) const;


    // Hardcode these:
    void initStandardLayout();
    void initBelgianDaisyLayout();
    void initGermanDaisyLayout();

    // Load from your 2-line input file
    bool loadFromInputFile(const std::string& filename);

    void setOccupant(const std::string& notation, Occupant who);

    // Board storage: occupant[i] says who is in cell index i
    std::array<Occupant, NUM_CELLS> occupant;

    // For each cell i, neighbors[i][d] = index of neighbor in direction d, or -1 if none
    std::array<std::array<int, NUM_DIRECTIONS>, NUM_CELLS> neighbors;

    // Constructor
    Board();

    // Converts "A1", "H5", "I9", etc. to a cell index in [0..60]
    // Returns -1 if invalid
    static int notationToIndex(const std::string& notation);

    // Helper to mark occupant of a cell at index
    void setOccupant(int index, Occupant who)
    {
        if (index >= 0 && index < NUM_CELLS)
        {
            occupant[index] = who;
        }
    }

    // Access occupant
    Occupant getOccupant(int index) const
    {
        if (index < 0 || index >= NUM_CELLS)
            return Occupant::EMPTY; // or handle error
        return occupant[index];
    }

private:
    // Once we have a (m,y) → index table, we can use it to build neighbors[][]
    static void initMapping();
    static bool s_mappingInitialized;

    // For every valid (m,y), we store its index in s_coordToIndex
    // e.g. s_coordToIndex[{1,1}] = 0, s_coordToIndex[{1,2}] = 1, ...
    // or you can store a single key like (m << 8) + y
    static std::unordered_map<long long, int> s_coordToIndex;

    // For reverse mapping: s_indexToCoord[i] = {m,y}
    static std::array<std::pair<int, int>, NUM_CELLS> s_indexToCoord;

    // Build the neighbor array
    void initNeighbors();
};

#endif // ABALONE_BOARD_H
