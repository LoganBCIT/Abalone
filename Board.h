#ifndef ABALONE_BOARD_H
#define ABALONE_BOARD_H

#include <array>
#include <string>
#include <unordered_map>

// Simple occupant type: empty, black, white
enum class Occupant
{
    EMPTY = 0,
    BLACK,
    WHITE
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

    Occupant nextToMove = Occupant::BLACK; // or you might store a bool, etc.

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
