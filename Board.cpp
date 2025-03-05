#include "Board.h"
#include <stdexcept>
#include <cctype>
#include <iostream>

// Directions in (dm, dy) form, matching your problem doc
// W: (m-1, y), E: (m+1, y), NW: (m, y+1), NE: (m+1, y+1), SW: (m-1, y-1), SE: (m, y-1)
const std::array<std::pair<int, int>, Board::NUM_DIRECTIONS> Board::DIRECTION_OFFSETS = {{
    {-1, 0},  // W
    {+1, 0},  // E
    {0, +1},  // NW
    {+1, +1}, // NE
    {-1, -1}, // SW
    {0, -1}   // SE
}};

// Static containers for mapping
bool Board::s_mappingInitialized = false;
std::unordered_map<long long, int> Board::s_coordToIndex;
std::array<std::pair<int, int>, Board::NUM_CELLS> Board::s_indexToCoord;

static long long packCoord(int m, int y)
{
    // Combine (m,y) into a single 64-bit. For example:
    // upper 32 bits = m, lower 32 bits = y
    // or shift by 16 if you prefer. Just be consistent
    return (static_cast<long long>(m) << 32) ^ (static_cast<long long>(y) & 0xffffffff);
}

// This method sets up the 61 valid cells, assigns each a unique index in [0..60],
// and fills s_coordToIndex and s_indexToCoord accordingly.
void Board::initMapping()
{
    if (s_mappingInitialized)
        return;

    s_mappingInitialized = true;
    s_coordToIndex.clear();

    // ---------------------------------------------------------
    //  1) Fill out the 61 valid (m, y) coords from your doc.
    //     The user example: A1 => (1,1), B1 => ???, ...
    //     We'll do a quick rough demonstration. YOU must fill
    //     in the actual 61 valid positions in your chosen order.
    // ---------------------------------------------------------
    // Suppose we store them row by row (y from 1..9, m from something..).
    // The exact pattern depends on your problem doc. Just be sure to do
    // exactly 61. Here is an EXAMPLE snippet (not the final official layout):

    // We'll keep a local vector of (m,y).
    // **IMPORTANT**: The problem doc shows E1 => (1,5), etc. You will need
    // to systematically list all valid positions that appear on the board.

    // For illustration, we'll just pretend we do 61 in some top-down order:
    int idx = 0;

    // Example approach: y goes from 1 to 9, m from 1..(some range).
    // We skip invalid combos. This is just an example:
    for (int y = 1; y <= 9; ++y)
    {
        for (int m = 1; m <= 9; ++m)
        {
            // We'll skip combos that aren't physically on the Abalone board
            // (the corners). The user doc says certain squares do not exist for early rows.
            // For demonstration, let's just do a quick bounding trick:
            // e.g. the row y=1 only has columns m in [1..5], row y=2 => [1..6], etc.
            // But you’d fill these if they appear in your official problem diagram.

            // Example bounding: (y + m) <= 10 or something. YOU must do what matches your doc.
            // This is just to show how the loop might skip invalid spots.

            // The user doc might say row "A" is y=1, but only m=1..5 => "A1..A5"
            // row "B" is y=2, m=1..6 => "B1..B6", etc.
            bool validCell = false;
            // We'll do a simpler condition for demonstration:
            switch (y)
            {
            case 1:
                validCell = (m >= 1 && m <= 5);
                break; // A1..A5
            case 2:
                validCell = (m >= 1 && m <= 6);
                break; // B1..B6
            case 3:
                validCell = (m >= 1 && m <= 7);
                break; // C1..C7
            case 4:
                validCell = (m >= 1 && m <= 8);
                break; // D1..D8
            case 5:
                validCell = (m >= 1 && m <= 9);
                break; // E1..E9
            case 6:
                validCell = (m >= 2 && m <= 9);
                break; // F2..F9
            case 7:
                validCell = (m >= 3 && m <= 9);
                break; // G3..G9
            case 8:
                validCell = (m >= 4 && m <= 9);
                break; // H4..H9
            case 9:
                validCell = (m >= 5 && m <= 9);
                break; // I5..I9
            default:
                validCell = false;
            }

            if (validCell)
            {
                long long key = packCoord(m, y);
                s_coordToIndex[key] = idx;
                s_indexToCoord[idx] = {m, y};
                ++idx;
            }
        }
    }

    // Make sure we ended with exactly 61
    if (idx != NUM_CELLS)
    {
        throw std::runtime_error("Did not fill exactly 61 cells. Check your coordinate loops!");
    }
}

Board::Board()
{
    // Ensure we have a valid coordinate mapping
    initMapping();

    // Initialize occupant array
    occupant.fill(Occupant::EMPTY);

    // Build neighbor array
    initNeighbors();
}

void Board::initNeighbors()
{
    // For each index i in [0..60], find its (m, y).
    // Then for each of the 6 directions, compute neighbor's (m+dm, y+dy).
    // If that neighbor is valid => neighbors[i][d] = neighborIndex, else -1.
    for (int i = 0; i < NUM_CELLS; ++i)
    {
        auto [m, y] = s_indexToCoord[i];
        for (int d = 0; d < NUM_DIRECTIONS; ++d)
        {
            int dm = DIRECTION_OFFSETS[d].first;
            int dy = DIRECTION_OFFSETS[d].second;
            int nm = m + dm;
            int ny = y + dy;

            long long nkey = packCoord(nm, ny);
            auto it = s_coordToIndex.find(nkey);
            if (it == s_coordToIndex.end())
            {
                // No valid neighbor in this direction
                neighbors[i][d] = -1;
            }
            else
            {
                neighbors[i][d] = it->second;
            }
        }
    }
}

// Convert "A1" => index. If invalid, returns -1
// The doc might say row letter => y, number => m. So "A" => y=1, "B" => y=2, ...
//   "1" => m=1, "2" => m=2, etc. Adjust to match your doc precisely.
int Board::notationToIndex(const std::string &notation)
{
    if (notation.size() < 2 || notation.size() > 3)
    {
        return -1; // e.g. "A10" might or might not be valid
    }

    // Extract letter portion and digit portion
    // "A1" => letter='A', digit='1'
    // "I9" => letter='I', digit='9'
    // Usually row letter from A..I => y=1..9
    char letter = std::toupper(notation[0]);
    int y = (letter - 'A') + 1; // 'A' => 1, 'B' =>2, ...
    if (y < 1 || y > 9)
    {
        return -1;
    }

    // Remainder is the number for m
    // e.g. "1", "2", "9", etc.
    int m = 0;
    try
    {
        m = std::stoi(notation.substr(1));
    }
    catch (...)
    {
        return -1;
    }
    if (m < 1 || m > 9)
    {
        return -1;
    }

    // Now we have (m, y). Check if it’s a valid cell
    long long key = packCoord(m, y);
    auto it = s_coordToIndex.find(key);
    if (it == s_coordToIndex.end())
    {
        return -1; // invalid cell
    }
    return it->second;
}
