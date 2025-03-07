#include "Board.h"
#include <stdexcept>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdexcept> 

// Directions in (dm, dy) form, matching your doc
const std::array<std::pair<int, int>, Board::NUM_DIRECTIONS> Board::DIRECTION_OFFSETS = { {
    {-1,  0}, // W
    {+1,  0}, // E
    { 0, +1}, // NW
    {+1, +1}, // NE
    {-1, -1}, // SW
    { 0, -1}  // SE
} };

//========================== 0) Move Logic ==========================//

bool Board::tryMove(const std::vector<int>& group, int direction, Move& move) const {
    // Create a temporary copy of the board.
    Board temp = *this;

    // Fill the move structure.
    move.marbleIndices = group;
    move.direction = direction;

    // Decide move type:
    if (group.size() == 1) {
        // For single marble, force side-step.
        move.isInline = false;
    }
    else {
        int alignedDir;
        if (isGroupAligned(group, alignedDir)) {
            // If candidate direction equals the alignment or its opposite, mark as inline.
            if (direction == alignedDir || direction == ((alignedDir + 3) % NUM_DIRECTIONS))
                move.isInline = true;
            else
                move.isInline = false;
        }
        else {
            // Not aligned: default to side-step.
            move.isInline = false;
        }
    }

    // (Optionally, set move.pushCount here if needed.)

    // Attempt to apply the move on the temporary board.
    try {
        temp.applyMove(move);
    }
    catch (const std::runtime_error& e) {
        return false;
    }
    return true;
}

void Board::dfsGroup(int current, Occupant side, std::vector<int>& group, std::set<std::vector<int>>& result) const {
    // Add the current group (sorted to avoid duplicates)
    std::vector<int> sortedGroup = group;
    std::sort(sortedGroup.begin(), sortedGroup.end());
    result.insert(sortedGroup);

    if (group.size() == 3)
        return;

    // For each neighbor of the current cell:
    for (int d = 0; d < NUM_DIRECTIONS; d++) {
        int n = neighbors[current][d];
        if (n >= 0 && occupant[n] == side) {
            // If n is not already in group, then add and search further.
            if (std::find(group.begin(), group.end(), n) == group.end()) {
                group.push_back(n);
                dfsGroup(n, side, group, result);
                group.pop_back();
            }
        }
    }
}

bool Board::isGroupAligned(const std::vector<int>& group, int& alignedDirection) const {
    // We require at least 2 marbles to define a direction.
    if (group.size() < 2)
        return false;

    // Work with a sorted copy of the group.
    std::vector<int> sortedGroup = group;
    std::sort(sortedGroup.begin(), sortedGroup.end());

    // Get the coordinate of the first marble.
    auto coord0 = s_indexToCoord[sortedGroup[0]]; // (m, y)
    // Determine the candidate direction vector from the first to second marble.
    auto coord1 = s_indexToCoord[sortedGroup[1]];
    int dx = coord1.first - coord0.first;
    int dy = coord1.second - coord0.second;
    if (dx == 0 && dy == 0)
        return false; // should not happen

    // Try each allowed direction.
    for (int d = 0; d < NUM_DIRECTIONS; d++) {
        auto offset = DIRECTION_OFFSETS[d];
        // Check if (dx,dy) is a positive multiple of offset.
        // We require that there exists an integer factor k >= 1 such that:
        //   dx == k * offset.first and dy == k * offset.second.
        // To avoid division (and potential rounding issues), we check:
        if ((offset.first != 0 && dx % offset.first == 0 &&
            dx / offset.first >= 1 && dy == (dx / offset.first) * offset.second) ||
            (offset.first == 0 && offset.second != 0 && dy % offset.second == 0 &&
                dy / offset.second >= 1 && dx == 0)) {
            // Now verify that every other marble lies along the same ray from coord0.
            bool aligned = true;
            int factor = (offset.first != 0) ? dx / offset.first : dy / offset.second;
            for (size_t i = 2; i < sortedGroup.size(); i++) {
                auto coord = s_indexToCoord[sortedGroup[i]];
                int adx = coord.first - coord0.first;
                int ady = coord.second - coord0.second;
                // Check that (adx,ady) is a positive multiple of offset.
                if (offset.first != 0) {
                    if (adx % offset.first != 0) { aligned = false; break; }
                    int k = adx / offset.first;
                    if (k < 1 || ady != k * offset.second) { aligned = false; break; }
                }
                else { // offset.first==0, so offset.second != 0
                    if (ady % offset.second != 0) { aligned = false; break; }
                    int k = ady / offset.second;
                    if (k < 1 || adx != 0) { aligned = false; break; }
                }
            }
            if (aligned) {
                alignedDirection = d;
                return true;
            }
        }
    }
    return false;
}



std::vector<Move> Board::generateMoves(Occupant side) const {
    std::vector<Move> moves;
    std::set<std::vector<int>> candidateGroups; // unique groups (sorted)

    // For each cell of the given side, run DFS to collect all connected groups (size 1-3).
    for (int i = 0; i < NUM_CELLS; i++) {
        if (occupant[i] != side)
            continue;
        std::vector<int> group = { i };
        dfsGroup(i, side, group, candidateGroups);
    }

    // Now, for each candidate group, try each direction.
    for (const auto& group : candidateGroups) {
        for (int d = 0; d < NUM_DIRECTIONS; d++) {
            Move candidateMove;
            if (tryMove(group, d, candidateMove))
                moves.push_back(candidateMove);
        }
    }
    return moves;
}




void Board::applyMove(const Move& m) {
    if (m.marbleIndices.empty()) {
        throw std::runtime_error("No marbles in move.");
    }

    int d = m.direction;
    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };

    std::cout << "Applying move: ";
    std::cout << (occupant[m.marbleIndices[0]] == Occupant::BLACK ? "b" : "w")
        << ", group (size " << m.marbleIndices.size() << "): ";
    for (int idx : m.marbleIndices)
        std::cout << indexToNotation(idx) << " ";
    std::cout << ", direction: " << d << " (" << DIRS[d] << ")"
        << (m.isInline ? " [inline]" : " [side-step]") << "\n";

    if (m.isInline) {
        std::vector<int> sortedGroup = m.marbleIndices;
        std::sort(sortedGroup.begin(), sortedGroup.end());
        int front = sortedGroup.back();
        int dest = neighbors[front][d];
        std::cout << "  Front cell: " << indexToNotation(front)
            << ", destination: " << (dest >= 0 ? indexToNotation(dest) : "off-board") << "\n";

        // Check for opponent marble in destination.
        if (dest >= 0 && occupant[dest] != Occupant::EMPTY && occupant[dest] != occupant[front]) {
            // Walk the chain of opponent marbles.
            int oppCount = 0;
            int cell = dest;
            while (cell >= 0 && occupant[cell] != Occupant::EMPTY &&
                occupant[cell] != occupant[front]) {
                oppCount++;
                cell = neighbors[cell][d];
            }
            // Rule: moving group must outnumber opponent chain.
            if (oppCount >= sortedGroup.size()) {
                throw std::runtime_error("Illegal move: cannot push, opponent group too large.");
            }
            // If there is a cell after the opponent chain, it must be empty.
            if (cell >= 0 && occupant[cell] != Occupant::EMPTY) {
                throw std::runtime_error("Illegal move: push blocked, destination not empty.");
            }
            std::cout << "  Push detected: pushing " << oppCount
                << " opponent marble" << (oppCount > 1 ? "s" : "") << ".\n";

            // Collect indices of opponent chain.
            std::vector<int> chain;
            cell = dest;
            for (int i = 0; i < oppCount; i++) {
                chain.push_back(cell);
                cell = neighbors[cell][d];
            }
            // Push opponent marbles in reverse order.
            for (int i = chain.size() - 1; i >= 0; i--) {
                int from = chain[i];
                int to = (i == chain.size() - 1) ? cell : chain[i + 1];
                if (to < 0) {
                    occupant[from] = Occupant::EMPTY;
                    std::cout << "    Marble at " << indexToNotation(from)
                        << " pushed off-board.\n";
                }
                else {
                    if (occupant[to] != Occupant::EMPTY)
                        throw std::runtime_error("Illegal move: push blocked while moving opponent marbles.");
                    occupant[to] = occupant[from];
                    occupant[from] = Occupant::EMPTY;
                    std::cout << "    Marble at " << indexToNotation(from)
                        << " moved to " << indexToNotation(to) << ".\n";
                }
            }
        }

        // Now move your own marbles.
        for (auto it = sortedGroup.rbegin(); it != sortedGroup.rend(); ++it) {
            int idx = *it;
            int target = neighbors[idx][d];
            std::cout << "  Moving " << indexToNotation(idx) << " to "
                << (target >= 0 ? indexToNotation(target) : "off-board") << "\n";
            if (target < 0) {
                throw std::runtime_error("Illegal move: marble would move off-board.");
            }
            if (occupant[target] != Occupant::EMPTY) {
                throw std::runtime_error("Illegal move: destination cell is not empty for inline move.");
            }
            occupant[target] = occupant[idx];
            occupant[idx] = Occupant::EMPTY;
        }
    }

    else {
        // Side-step moves: move each marble individually.
        for (int idx : m.marbleIndices) {
            int target = neighbors[idx][d];
            std::cout << "  Side-stepping " << indexToNotation(idx) << " to "
                << (target >= 0 ? indexToNotation(target) : "off-board") << "\n";
            if (target < 0) {
                throw std::runtime_error("Illegal move: side-step moves off-board.");
            }
            if (occupant[target] != Occupant::EMPTY) {
                throw std::runtime_error("Illegal move: destination cell is not empty for side-step.");
            }
            occupant[target] = occupant[idx];
            occupant[idx] = Occupant::EMPTY;
        }
    }
}

std::string Board::moveToNotation(const Move& m, Occupant side) {
    std::string notation;
    char teamChar = (side == Occupant::BLACK ? 'b' : 'w');

    // Create a vector of cell notations for each index in the move.
    std::vector<std::string> cellNotations;
    for (int idx : m.marbleIndices) {
        cellNotations.push_back(indexToNotation(idx));
    }

    // Sort in descending order (so that, e.g., F3, E3, D3 appear)
    std::sort(cellNotations.begin(), cellNotations.end(), std::greater<std::string>());

    // Build the notation string.
    notation = "(";
    notation.push_back(teamChar);
    notation += ", ";

    // Append each cell coordinate separated by commas.
    for (size_t i = 0; i < cellNotations.size(); i++) {
        if (i > 0)
            notation += ", ";
        notation += cellNotations[i];
    }
    notation += ") ";
    // 'i' for inline moves; 's' for side-step moves.
    notation += (m.isInline ? "i" : "s");
    notation += " → ";

    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };
    notation += DIRS[m.direction];

    return notation;
}



std::string Board::toBoardString() const {
    // Gather occupant positions
    // We'll store them in two vectors: blackCells, whiteCells
    std::vector<std::string> blackCells;
    std::vector<std::string> whiteCells;

    for (int i = 0; i < NUM_CELLS; i++) {
        if (occupant[i] == Occupant::BLACK) {
            blackCells.push_back(indexToNotation(i) + "b");
        }
        else if (occupant[i] == Occupant::WHITE) {
            whiteCells.push_back(indexToNotation(i) + "w");
        }
    }

    // Sort each vector. 
    // The notation "A1", "A2", ... "B1" etc. is alphabetical by letter then numeric by col
    // If your indexToNotation already returns "A1", "B2" in the correct style, 
    // you can just do a lexicographical sort.
    auto cmp = [](const std::string& a, const std::string& b) {
        return a < b; // simple lex order: A2b < A10b, careful with single vs double digit
        };
    std::sort(blackCells.begin(), blackCells.end(), cmp);
    std::sort(whiteCells.begin(), whiteCells.end(), cmp);

    // Combine them (black first, then white)
    std::vector<std::string> all;
    all.reserve(blackCells.size() + whiteCells.size());
    for (auto& bc : blackCells) all.push_back(bc);
    for (auto& wc : whiteCells) all.push_back(wc);

    // Join them with commas
    std::string result;
    for (size_t i = 0; i < all.size(); i++) {
        if (i > 0) result += ",";
        result += all[i];
    }

    return result;
}

// Helper: index -> e.g. "C5"
std::string Board::indexToNotation(int idx) {
    // s_indexToCoord[idx] => (m, y)
    auto [m, y] = s_indexToCoord[idx];
    // y => 'A' + (y-1)
    char rowLetter = char('A' + (y - 1));
    // col = m

    // Build string, e.g. "C5"
    std::string notation;
    notation.push_back(rowLetter);
    notation += std::to_string(m);
    return notation;
}



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
