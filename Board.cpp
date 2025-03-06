#include "Board.h"
#include <stdexcept>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Directions in (dm, dy) form, matching your doc
const std::array<std::pair<int, int>, Board::NUM_DIRECTIONS> Board::DIRECTION_OFFSETS = { {
    {-1,  0}, // W
    {+1,  0}, // E
    { 0, +1}, // NW
    {+1, +1}, // NE
    {-1, -1}, // SW
    { 0, -1}  // SE
} };

//========================== 1) HARDCODED LAYOUTS ==========================//

void Board::initStandardLayout() {
    occupant.fill(Occupant::EMPTY);

    // Example squares for the "standard" arrangement.
    // This is just a sample list. Replace with the actual squares for a real standard setup.
    // Usually black is at top rows, white at bottom, etc.

    // Hardcode black squares:
    std::vector<std::string> blackPositions = {
        // For example:
        "A4", "A5",
        "B4", "B5", "B6",
        "C4", "C5", "C6", "C7",
        "D5", "D6", "D7",
        "E5", "E6"
    };
    for (auto& cell : blackPositions) {
        setOccupant(cell, Occupant::BLACK);
    }

    // Hardcode white squares:
    std::vector<std::string> whitePositions = {
        // For example:
        "E4", "F4", "F5", "F6", "F7",
        "G3", "G4", "G5", "G6", "G7",
        "H4", "H5", "H6",
        "I5"
    };
    for (auto& cell : whitePositions) {
        setOccupant(cell, Occupant::WHITE);
    }
}

void Board::initBelgianDaisyLayout() {
    occupant.fill(Occupant::EMPTY);

    // Example squares for Belgian Daisy arrangement.
    // Replace with the official squares from your reference.

    // Black
    std::vector<std::string> blackPositions = {
        "C5","C6","D4","D7","E4","E7","F4","F7","G5","G6"
        // Add the rest to total 14 black marbles
    };
    for (auto& cell : blackPositions) {
        setOccupant(cell, Occupant::BLACK);
    }

    // White
    std::vector<std::string> whitePositions = {
        "C4","D3","E3","F3","G4","G7","D8","E8","F8","G8"
        // Add the rest to total 14 white marbles
    };
    for (auto& cell : whitePositions) {
        setOccupant(cell, Occupant::WHITE);
    }
}

void Board::initGermanDaisyLayout() {
    occupant.fill(Occupant::EMPTY);

    // Example squares for German Daisy arrangement.
    // Replace with official squares.
    // Typically forms a daisy shape offset differently from Belgian.

    // Black
    std::vector<std::string> blackPositions = {
        "B4","C4","D5","E5","F5","G5","H6"
        // ... etc. Fill up to 14
    };
    for (auto& cell : blackPositions) {
        setOccupant(cell, Occupant::BLACK);
    }

    // White
    std::vector<std::string> whitePositions = {
        "B5","C5","D4","E4","F4","G4","H5"
        // ... etc. Fill up to 14
    };
    for (auto& cell : whitePositions) {
        setOccupant(cell, Occupant::WHITE);
    }
}

//========================== 2) LOADING FROM INPUT FILE ==========================//
//
// Format you described:
//   Line 1: single character 'b' or 'w'
//   Line 2: comma-separated "A5b,D5b,E4b,E5b,..."
//
bool Board::loadFromInputFile(const std::string& filename) {
    occupant.fill(Occupant::EMPTY);

    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: could not open file: " << filename << "\n";
        return false;
    }

    // 1) Read the color of the marbles to move next
    std::string line;
    if (!std::getline(fin, line)) {
        std::cerr << "Error: file is missing the first line.\n";
        return false;
    }
    if (line.size() < 1) {
        std::cerr << "Error: first line is empty.\n";
        return false;
    }

    char nextColorChar = line[0];
    if (nextColorChar == 'b' || nextColorChar == 'B') {
        nextToMove = Occupant::BLACK;
    }
    else if (nextColorChar == 'w' || nextColorChar == 'W') {
        nextToMove = Occupant::WHITE;
    }
    else {
        std::cerr << "Error: first line must be 'b' or 'w'. Found: " << line << "\n";
        return false;
    }

    // 2) Read the second line with positions
    if (!std::getline(fin, line)) {
        std::cerr << "Error: file is missing the second line.\n";
        return false;
    }
    // line might be e.g. "C5b,D5b,E4b,E5b,E6b..."

    // We'll split by commas
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // token e.g. "C5b" or "D5b"
        // trim whitespace if any
        if (token.empty()) continue;

        // The last character is color: 'b' or 'w'
        char c = token.back();
        Occupant who;
        if (c == 'b' || c == 'B') who = Occupant::BLACK;
        else if (c == 'w' || c == 'W') who = Occupant::WHITE;
        else {
            std::cerr << "Warning: token '" << token << "' does not end in b/w. Skipping.\n";
            continue;
        }

        // remove that last character from the token so only e.g. "C5" remains
        token.pop_back();

        // token should now be "C5", "D5", etc.
        // set occupant
        setOccupant(token, who);
    }

    fin.close();
    return true;
}

//========================== 3) setOccupant & utility ==========================//

void Board::setOccupant(const std::string& notation, Occupant who) {
    int idx = notationToIndex(notation);
    if (idx >= 0) {
        occupant[idx] = who;
    }
    else {
        std::cerr << "Warning: invalid cell notation '" << notation << "'\n";
    }
}

//========================== 4) THE REST (mapping, neighbors, etc.) ==========================//

bool Board::s_mappingInitialized = false;
std::unordered_map<long long, int> Board::s_coordToIndex;
std::array<std::pair<int, int>, Board::NUM_CELLS> Board::s_indexToCoord;

static long long packCoord(int m, int y)
{
    // Combine (m,y) into a single 64-bit
    return (static_cast<long long>(m) << 32) ^ (static_cast<long long>(y) & 0xffffffff);
}

void Board::initMapping()
{
    if (s_mappingInitialized) return;
    s_mappingInitialized = true;
    s_coordToIndex.clear();

    int idx = 0;
    for (int y = 1; y <= 9; ++y) {
        for (int m = 1; m <= 9; ++m) {
            bool validCell = false;
            switch (y)
            {
            case 1: validCell = (m >= 1 && m <= 5); break; // A1..A5
            case 2: validCell = (m >= 1 && m <= 6); break; // B1..B6
            case 3: validCell = (m >= 1 && m <= 7); break; // C1..C7
            case 4: validCell = (m >= 1 && m <= 8); break; // D1..D8
            case 5: validCell = (m >= 1 && m <= 9); break; // E1..E9
            case 6: validCell = (m >= 2 && m <= 9); break; // F2..F9
            case 7: validCell = (m >= 3 && m <= 9); break; // G3..G9
            case 8: validCell = (m >= 4 && m <= 9); break; // H4..H9
            case 9: validCell = (m >= 5 && m <= 9); break; // I5..I9
            }

            if (validCell) {
                long long key = packCoord(m, y);
                s_coordToIndex[key] = idx;
                s_indexToCoord[idx] = { m, y };
                ++idx;
            }
        }
    }

    if (idx != NUM_CELLS) {
        throw std::runtime_error("Did not fill exactly 61 cells! Check your loops!");
    }
}

Board::Board()
{
    initMapping();
    occupant.fill(Occupant::EMPTY);
    initNeighbors();
}

void Board::initNeighbors()
{
    for (int i = 0; i < NUM_CELLS; ++i) {
        int m = s_indexToCoord[i].first;
        int y = s_indexToCoord[i].second;

        for (int d = 0; d < NUM_DIRECTIONS; ++d) {
            int dm = DIRECTION_OFFSETS[d].first;
            int dy = DIRECTION_OFFSETS[d].second;
            int nm = m + dm;
            int ny = y + dy;

            long long nkey = packCoord(nm, ny);
            auto it = s_coordToIndex.find(nkey);
            if (it == s_coordToIndex.end()) {
                neighbors[i][d] = -1;
            }
            else {
                neighbors[i][d] = it->second;
            }
        }
    }
}

int Board::notationToIndex(const std::string& notation)
{
    if (notation.size() < 2 || notation.size() > 3) {
        return -1;
    }
    char letter = std::toupper(notation[0]);
    int y = (letter - 'A') + 1;
    if (y < 1 || y > 9) {
        return -1;
    }

    int m = 0;
    try {
        m = std::stoi(notation.substr(1));
    }
    catch (...) {
        return -1;
    }
    if (m < 1 || m > 9) {
        return -1;
    }

    long long key = packCoord(m, y);
    auto it = s_coordToIndex.find(key);
    if (it == s_coordToIndex.end()) {
        return -1;
    }
    return it->second;
}
