#include "Board.h"
#include <stdexcept>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdexcept> 
#include <unordered_set>

// Directions in (dm, dy) form, matching your doc
const std::array<std::pair<int, int>, Board::NUM_DIRECTIONS> Board::DIRECTION_OFFSETS = { {
    {-1,  0}, // W
    {+1,  0}, // E
    { 0, +1}, // NW
    {+1, +1}, // NE
    {-1, -1}, // SW
    { 0, -1}  // SE
} };

static const std::array<int, Board::NUM_DIRECTIONS> OPPOSITES = { 1, 0, 5, 4, 3, 2 };

bool Board::tryMove(const std::vector<int>& group, int direction, Move& move) const {
    Board temp = *this;
    move.marbleIndices = group;
    move.direction = direction;
    if (group.size() == 1) {
        move.isInline = false;
    }
    else {
        int alignedDir;
        if (isGroupAligned(group, alignedDir)) {
            if (direction == alignedDir || direction == OPPOSITES[alignedDir])
                move.isInline = true;
            else
                move.isInline = false;
        }
        else {
            move.isInline = false;
        }
    }
    try {
        temp.applyMove(move);
    }
    catch (const std::runtime_error&) {
        return false;
    }
    return true;
}

std::set<std::vector<int>> Board::generateColumnGroups(Occupant side) const {
    std::set<std::vector<int>> groups;
    for (int i = 0; i < NUM_CELLS; i++) {
        if (occupant[i] != side)
            continue;
        for (int d = 0; d < NUM_DIRECTIONS; d++) {
            int opp = (d + 3) % NUM_DIRECTIONS;
            int behind = neighbors[i][opp];
            if (behind >= 0 && occupant[behind] == side)
                continue;
            std::vector<int> col;
            col.push_back(i);
            int current = i;
            while (col.size() < 3) {
                int next = neighbors[current][d];
                if (next >= 0 && occupant[next] == side) {
                    col.push_back(next);
                    current = next;
                }
                else {
                    break;
                }
            }
            for (size_t s = 1; s <= col.size(); s++) {
                std::vector<int> group(col.begin(), col.begin() + s);
                groups.insert(group);
            }
        }
    }
    return groups;
}

void Board::dfsGroup(int current, Occupant side, std::vector<int>& group, std::set<std::vector<int>>& result) const {
    result.insert(group);
    if (group.size() == 3)
        return;
    for (int d = 0; d < NUM_DIRECTIONS; d++) {
        int n = neighbors[current][d];
        if (n >= 0 && occupant[n] == side) {
            if (std::find(group.begin(), group.end(), n) == group.end() && n > group.back()) {
                group.push_back(n);
                dfsGroup(n, side, group, result);
                group.pop_back();
            }
        }
    }
}

bool Board::isGroupAligned(const std::vector<int>& group, int& alignedDirection) const {
    if (group.size() < 2)
        return false;
    std::vector<int> sortedGroup = group;
    std::sort(sortedGroup.begin(), sortedGroup.end(), [this](int a, int b) {
        auto ca = s_indexToCoord[a];
        auto cb = s_indexToCoord[b];
        return (ca.second < cb.second) || (ca.second == cb.second && ca.first < cb.first);
        });
    auto coord0 = s_indexToCoord[sortedGroup[0]];
    auto coord1 = s_indexToCoord[sortedGroup[1]];
    int dx = coord1.first - coord0.first;
    int dy = coord1.second - coord0.second;
    if (dx == 0 && dy == 0)
        return false;
    for (int d = 0; d < NUM_DIRECTIONS; d++) {
        auto offset = DIRECTION_OFFSETS[d];
        if ((offset.first != 0 && dx % offset.first == 0 &&
            dx / offset.first >= 1 && dy == (dx / offset.first) * offset.second) ||
            (offset.first == 0 && offset.second != 0 && dy % offset.second == 0 &&
                dy / offset.second >= 1 && dx == 0)) {
            bool aligned = true;
            for (size_t i = 2; i < sortedGroup.size(); i++) {
                auto coord = s_indexToCoord[sortedGroup[i]];
                int adx = coord.first - coord0.first;
                int ady = coord.second - coord0.second;
                if (offset.first != 0) {
                    if (adx % offset.first != 0) { aligned = false; break; }
                    int k = adx / offset.first;
                    if (k < 1 || ady != k * offset.second) { aligned = false; break; }
                }
                else {
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

std::vector<int> Board::canonicalizeGroup(const std::vector<int>& group) {
    std::vector<int> canon = group;
    std::sort(canon.begin(), canon.end(), [](int a, int b) {
        auto ca = s_indexToCoord[a];
        auto cb = s_indexToCoord[b];
        return (ca.second < cb.second) || (ca.second == cb.second && ca.first < cb.first);
        });
    return canon;
}


// New version of generateMoves that uses canonical groups to eliminate duplicates
std::vector<Move> Board::generateMoves(Occupant side) const {
    std::vector<Move> moves;

    // Use a set keyed on the canonical string representation of a group.
    // (Alternatively, you could write a custom comparator for vector<int>.)
    std::set<std::string> uniqueGroupKeys;
    std::vector<std::vector<int>> candidateGroups;

    // --- Generate candidate groups via DFS ---
    {
        std::set<std::vector<int>> dfsGroups;
        for (int i = 0; i < NUM_CELLS; i++) {
            if (occupant[i] != side)
                continue;
            std::vector<int> group = { i };
            dfsGroup(i, side, group, dfsGroups);
        }
        for (const auto& g : dfsGroups) {
            int dummy;
            // Only consider single marbles or aligned groups
            if (g.size() == 1 || isGroupAligned(g, dummy)) {
                auto canon = canonicalizeGroup(g);
                // Create a key string from the canonical group
                std::string key;
                for (int idx : canon) {
                    key += std::to_string(idx) + ",";
                }
                // Insert if unique
                if (uniqueGroupKeys.insert(key).second) {
                    candidateGroups.push_back(canon);
                }
            }
        }
    }

    // --- Generate candidate groups via column grouping ---
    {
        auto colGroups = generateColumnGroups(side);
        for (const auto& g : colGroups) {
            auto canon = canonicalizeGroup(g);
            std::string key;
            for (int idx : canon) {
                key += std::to_string(idx) + ",";
            }
            if (uniqueGroupKeys.insert(key).second) {
                candidateGroups.push_back(canon);
            }
        }
    }

    // Now, for each unique candidate group, try every direction.
    for (const auto& group : candidateGroups) {
        for (int d = 0; d < NUM_DIRECTIONS; d++) {
            Move candidateMove;
            if (tryMove(group, d, candidateMove)) {
                moves.push_back(candidateMove);
            }
        }
    }

    return moves;
}

static inline std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        end--;
    return s.substr(start, end - start);
}

void Board::applyMove(const Move& m) {
    if (m.marbleIndices.empty()) {
        throw std::runtime_error("No marbles in move.");
    }
    int d = m.direction;
    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };
    std::cout << "Applying move: "
        << (occupant[m.marbleIndices[0]] == Occupant::BLACK ? "b" : "w")
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
        if (dest >= 0 && occupant[dest] != Occupant::EMPTY && occupant[dest] != occupant[front]) {
            int oppCount = 0;
            int cell = dest;
            while (cell >= 0 && occupant[cell] != Occupant::EMPTY &&
                occupant[cell] != occupant[front]) {
                oppCount++;
                cell = neighbors[cell][d];
            }
            if (oppCount >= sortedGroup.size()) {
                throw std::runtime_error("Illegal move: cannot push, opponent group too large.");
            }
            if (cell >= 0 && occupant[cell] != Occupant::EMPTY) {
                throw std::runtime_error("Illegal move: push blocked, destination not empty.");
            }
            std::cout << "  Push detected: pushing " << oppCount
                << " opponent marble" << (oppCount > 1 ? "s" : "") << ".\n";
            std::vector<int> chain;
            cell = dest;
            for (int i = 0; i < oppCount; i++) {
                chain.push_back(cell);
                cell = neighbors[cell][d];
            }
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
    std::vector<std::string> cellNotations;
    for (int idx : m.marbleIndices) {
        cellNotations.push_back(indexToNotation(idx));
    }
    std::sort(cellNotations.begin(), cellNotations.end(), std::greater<std::string>());
    notation = "(";
    notation.push_back(teamChar);
    notation += ", ";
    for (size_t i = 0; i < cellNotations.size(); i++) {
        if (i > 0)
            notation += ", ";
        notation += cellNotations[i];
    }
    notation += ") ";
    notation += (m.isInline ? "i" : "s");
    notation += " → ";
    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };
    notation += DIRS[m.direction];
    return notation;
}

std::string Board::toBoardString() const {
    std::string result;
    bool first = true;
    for (int i = 0; i < NUM_CELLS; i++) {
        if (occupant[i] == Occupant::BLACK || occupant[i] == Occupant::WHITE) {
            if (!first)
                result += ",";
            result += indexToNotation(i);
            result += (occupant[i] == Occupant::BLACK ? "b" : "w");
            first = false;
        }
    }
    return result;
}


std::string Board::indexToNotation(int idx) {
    auto [m, y] = s_indexToCoord[idx];
    char rowLetter = char('A' + (y - 1));
    std::string notation;
    notation.push_back(rowLetter);
    notation += std::to_string(m);
    return notation;
}

//========================== 1) HARDCODED LAYOUTS ==========================//

void Board::initStandardLayout() {
    occupant.fill(Occupant::EMPTY);
    std::vector<std::string> blackPositions = {
        "A4", "A5",
        "B4", "B5", "B6",
        "C4", "C5", "C6", "C7",
        "D5", "D6", "D7",
        "E5", "E6"
    };
    for (auto& cell : blackPositions) {
        setOccupant(cell, Occupant::BLACK);
    }
    std::vector<std::string> whitePositions = {
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
    std::vector<std::string> blackPositions = {
        "C5","C6","D4","D7","E4","E7","F4","F7","G5","G6"
    };
    for (auto& cell : blackPositions) {
        setOccupant(cell, Occupant::BLACK);
    }
    std::vector<std::string> whitePositions = {
        "C4","D3","E3","F3","G4","G7","D8","E8","F8","G8"
    };
    for (auto& cell : whitePositions) {
        setOccupant(cell, Occupant::WHITE);
    }
}

void Board::initGermanDaisyLayout() {
    occupant.fill(Occupant::EMPTY);
    std::vector<std::string> blackPositions = {
        "B4","C4","D5","E5","F5","G5","H6"
    };
    for (auto& cell : blackPositions) {
        setOccupant(cell, Occupant::BLACK);
    }
    std::vector<std::string> whitePositions = {
        "B5","C5","D4","E4","F4","G4","H5"
    };
    for (auto& cell : whitePositions) {
        setOccupant(cell, Occupant::WHITE);
    }
}

//========================== 2) LOADING FROM INPUT FILE ==========================//

bool Board::loadFromInputFile(const std::string& filename) {
    occupant.fill(Occupant::EMPTY);
    std::ifstream fin(filename);
    if (!fin.is_open()) {
        std::cerr << "Error: could not open file: " << filename << "\n";
        return false;
    }
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
    if (!std::getline(fin, line)) {
        std::cerr << "Error: file is missing the second line.\n";
        return false;
    }
    std::stringstream ss(line);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.empty()) continue;
        char c = token.back();
        Occupant who;
        if (c == 'b' || c == 'B') who = Occupant::BLACK;
        else if (c == 'w' || c == 'W') who = Occupant::WHITE;
        else {
            std::cerr << "Warning: token '" << token << "' does not end in b/w. Skipping.\n";
            continue;
        }
        token.pop_back();
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

static long long packCoord(int m, int y) {
    return (static_cast<long long>(m) << 32) ^ (static_cast<long long>(y) & 0xffffffff);
}

void Board::initMapping() {
    if (s_mappingInitialized) return;
    s_mappingInitialized = true;
    s_coordToIndex.clear();
    int idx = 0;
    for (int y = 1; y <= 9; ++y) {
        for (int m = 1; m <= 9; ++m) {
            bool validCell = false;
            switch (y) {
            case 1: validCell = (m >= 1 && m <= 5); break;
            case 2: validCell = (m >= 1 && m <= 6); break;
            case 3: validCell = (m >= 1 && m <= 7); break;
            case 4: validCell = (m >= 1 && m <= 8); break;
            case 5: validCell = (m >= 1 && m <= 9); break;
            case 6: validCell = (m >= 2 && m <= 9); break;
            case 7: validCell = (m >= 3 && m <= 9); break;
            case 8: validCell = (m >= 4 && m <= 9); break;
            case 9: validCell = (m >= 5 && m <= 9); break;
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

Board::Board() {
    initMapping();
    occupant.fill(Occupant::EMPTY);
    initNeighbors();
}

void Board::initNeighbors() {
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

int Board::notationToIndex(const std::string& notation) {
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
