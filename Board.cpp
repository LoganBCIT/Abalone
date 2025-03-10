#include "Board.h"
#include <stdexcept>
#include <cctype>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iostream>

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


std::vector<Move> Board::generateMoves(Occupant side) const {
    std::vector<Move> moves;
    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };

    // Loop through all board cells.
    for (int i = 0; i < NUM_CELLS; i++) {
        // Process only cells with our marble.
        if (occupant[i] != side)
            continue;

        // -------- 1. Single Marble Moves --------
        std::cout << "Checking single marble at " << indexToNotation(i) << "\n";
        for (int d = 0; d < NUM_DIRECTIONS; d++) {
            int nIdx = neighbors[i][d];
            if (nIdx >= 0 && occupant[nIdx] == Occupant::EMPTY) {
                Move mv;
                mv.marbleIndices.push_back(i);
                mv.direction = d;
                mv.isInline = true; // single marble—no pushing.
                std::cout << "  Single move: " << indexToNotation(i) << " -> "
                    << indexToNotation(nIdx) << " (dir " << d << " [" << DIRS[d] << "])\n";
                moves.push_back(mv);
            }
        }

        // -------- 2. Group Formation: Two- and Three-Marble Groups --------
        // For each direction, see if a friendly marble is adjacent.
        for (int d = 0; d < NUM_DIRECTIONS; d++) {
            int j = neighbors[i][d];
            if (j < 0 || occupant[j] != side)
                continue;
            // We have at least a 2-marble group.
            std::vector<int> group = { i, j };

            // Debug: Print 2-marble group attempt.
            std::cout << "Attempting 2-marble group from " << indexToNotation(i)
                << " in direction " << d << " [" << DIRS[d] << "]\n";

            // Check for duplicate group: only proceed if i is the minimum.
            if (i != *std::min_element(group.begin(), group.end()))
                continue;

            // Generate moves for the 2-marble group.
            generateGroupMoves(group, d, moves);

            // Now try to extend the group to 3 marbles (in the same direction).
            int k = neighbors[j][d];
            if (k >= 0 && occupant[k] == side) {
                group.push_back(k);
                std::cout << "Extended group to 3 marbles: ";
                for (int cell : group)
                    std::cout << indexToNotation(cell) << " ";
                std::cout << "(direction " << d << " [" << DIRS[d] << "])\n";

                if (i != *std::min_element(group.begin(), group.end()))
                    continue;

                // Generate moves for the 3-marble group.
                generateGroupMoves(group, d, moves);
            }
        }
    }
    return moves;
}

// Helper function to generate moves (inline and side-step) for a given group.
// The parameter 'd' is the direction in which the group is aligned.
void Board::generateGroupMoves(const std::vector<int>& group, int d, std::vector<Move>& moves) const {
    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };
    // Build a string representation for debugging.
    std::string groupStr;
    for (int idx : group)
        groupStr += indexToNotation(idx) + " ";
    std::cout << "  Generating moves for group (" << groupStr << "), aligned in direction "
        << d << " [" << DIRS[d] << "], group size: " << group.size() << "\n";

    // ---- Inline Moves ----
    // Forward inline: destination for the front marble.
    int front = group.back();
    int frontDest = neighbors[front][d];
    if (frontDest >= 0) {
        if (occupant[frontDest] == Occupant::EMPTY) {
            Move mv;
            mv.marbleIndices = group;
            mv.direction = d;
            mv.isInline = true;
            // No push required.
            std::cout << "    Inline forward move: group (" << groupStr << ") can move forward into "
                << indexToNotation(frontDest) << " (dir " << d << " [" << DIRS[d] << "])\n";
            moves.push_back(mv);
        }
        // Check push possibility forward:
        else if (occupant[frontDest] != occupant[group[0]]) {  // opponent present
            // For groups of 2, only one marble can be pushed.
            if (group.size() == 2) {
                bool canPush = true;
                int current = frontDest;
                std::cout << "    Checking forward push for 2-marble group (" << groupStr
                    << ") in direction " << d << " [" << DIRS[d] << "], starting at "
                    << indexToNotation(frontDest) << "\n";
                int nextChain = neighbors[current][d];
                std::cout << "      Push chain: " << indexToNotation(current)
                    << " -> " << (nextChain >= 0 ? indexToNotation(nextChain) : "off-board") << "\n";
                if (nextChain < 0 || (nextChain >= 0 && occupant[nextChain] != Occupant::EMPTY)) {
                    canPush = false;
                    std::cout << "      Cannot push: destination "
                        << (nextChain >= 0 ? indexToNotation(nextChain) : "off-board")
                        << " is not empty.\n";
                }
                if (canPush) {
                    Move mv;
                    mv.marbleIndices = group;
                    mv.direction = d;
                    mv.isInline = true;
                    mv.pushCount = 1; // record that we push one marble.
                    std::cout << "    Forward push move generated for 2-marble group (" << groupStr << ")\n";
                    moves.push_back(mv);
                }
            }
            // For groups of 3, allow pushing either one or two opponent marbles.
            else if (group.size() == 3) {
                // --- Case 1: Push one opponent marble ---
                int next1 = neighbors[frontDest][d];
                bool canPushOne = (next1 < 0 || (next1 >= 0 && occupant[next1] == Occupant::EMPTY));
                if (canPushOne) {
                    Move mv;
                    mv.marbleIndices = group;
                    mv.direction = d;
                    mv.isInline = true;
                    mv.pushCount = 1;
                    std::cout << "    Forward push move generated for 3-marble group (" << groupStr
                        << ") pushing one marble\n";
                    moves.push_back(mv);
                }
                // --- Case 2: Push two opponent marbles ---
                // Ensure the first opponent cell is occupied.
                if (frontDest >= 0 && occupant[frontDest] != Occupant::EMPTY && occupant[frontDest] != occupant[group[0]]) {
                    int secondOpp = neighbors[frontDest][d];
                    if (secondOpp >= 0 && occupant[secondOpp] != Occupant::EMPTY && occupant[secondOpp] != occupant[group[0]]) {
                        int third = neighbors[secondOpp][d];
                        bool canPushTwo = (third < 0 || (third >= 0 && occupant[third] == Occupant::EMPTY));
                        if (canPushTwo) {
                            Move mv;
                            mv.marbleIndices = group;
                            mv.direction = d;
                            mv.isInline = true;
                            mv.pushCount = 2;
                            std::cout << "    Forward push move generated for 3-marble group (" << groupStr
                                << ") pushing two marbles\n";
                            moves.push_back(mv);
                        }
                    }
                }
            }
        }
    }

    // ---- Backward inline moves (similar logic with opposite direction) ----
    int opp = (d + 3) % 6;
    int back = group.front();
    int backDest = neighbors[back][opp];
    if (backDest >= 0) {
        if (occupant[backDest] == Occupant::EMPTY) {
            Move mv;
            mv.marbleIndices = group;
            mv.direction = opp;
            mv.isInline = true;
            std::cout << "    Inline backward move: group (" << groupStr << ") can move backward into "
                << indexToNotation(backDest) << " (dir " << opp << " [" << DIRS[opp] << "])\n";
            moves.push_back(mv);
        }
        else if (occupant[backDest] != occupant[group[0]]) { // opponent present
            if (group.size() == 2) {
                bool canPush = true;
                int current = backDest;
                std::cout << "    Checking backward push for 2-marble group (" << groupStr
                    << ") in direction " << opp << " [" << DIRS[opp] << "], starting at "
                    << indexToNotation(backDest) << "\n";
                int nextChain = neighbors[current][opp];
                std::cout << "      Push chain backward: " << indexToNotation(current)
                    << " -> " << (nextChain >= 0 ? indexToNotation(nextChain) : "off-board") << "\n";
                if (nextChain < 0 || (nextChain >= 0 && occupant[nextChain] != Occupant::EMPTY)) {
                    canPush = false;
                    std::cout << "      Cannot push backward: destination "
                        << (nextChain >= 0 ? indexToNotation(nextChain) : "off-board")
                        << " is not empty.\n";
                }
                if (canPush) {
                    Move mv;
                    mv.marbleIndices = group;
                    mv.direction = opp;
                    mv.isInline = true;
                    mv.pushCount = 1;
                    std::cout << "    Backward push move generated for 2-marble group (" << groupStr << ")\n";
                    moves.push_back(mv);
                }
            }
            else if (group.size() == 3) {
                // --- Case 1: Push one opponent marble backward ---
                int next1 = neighbors[backDest][opp];
                bool canPushOne = (next1 < 0 || (next1 >= 0 && occupant[next1] == Occupant::EMPTY));
                if (canPushOne) {
                    Move mv;
                    mv.marbleIndices = group;
                    mv.direction = opp;
                    mv.isInline = true;
                    mv.pushCount = 1;
                    std::cout << "    Backward push move generated for 3-marble group (" << groupStr
                        << ") pushing one marble\n";
                    moves.push_back(mv);
                }
                // --- Case 2: Push two opponent marbles backward ---
                if (backDest >= 0 && occupant[backDest] != Occupant::EMPTY && occupant[backDest] != occupant[group[0]]) {
                    int secondOpp = neighbors[backDest][opp];
                    if (secondOpp >= 0 && occupant[secondOpp] != Occupant::EMPTY && occupant[secondOpp] != occupant[group[0]]) {
                        int third = neighbors[secondOpp][opp];
                        bool canPushTwo = (third < 0 || (third >= 0 && occupant[third] == Occupant::EMPTY));
                        if (canPushTwo) {
                            Move mv;
                            mv.marbleIndices = group;
                            mv.direction = opp;
                            mv.isInline = true;
                            mv.pushCount = 2;
                            std::cout << "    Backward push move generated for 3-marble group (" << groupStr
                                << ") pushing two marbles\n";
                            moves.push_back(mv);
                        }
                    }
                }
            }
        }
    }

    // ---- Side-Step Moves ----
    // For side-step moves, check every direction that is not inline.
    for (int sd = 0; sd < NUM_DIRECTIONS; sd++) {
        if (sd == d || sd == opp)
            continue;
        bool canSideStep = true;
        std::cout << "    Checking side-step (dir " << sd << " [" << DIRS[sd]
            << "]) for group (" << groupStr << ")\n";
            for (int cell : group) {
                int dest = neighbors[cell][sd];
                std::cout << "      " << indexToNotation(cell) << " -> "
                    << (dest >= 0 ? indexToNotation(dest) : "off-board") << "\n";
                if (dest < 0 || occupant[dest] != Occupant::EMPTY) {
                    canSideStep = false;
                    std::cout << "      Cannot side-step: destination not empty or off-board.\n";
                    break;
                }
            }
            if (canSideStep) {
                Move mv;
                mv.marbleIndices = group;
                mv.direction = sd;
                mv.isInline = false;
                std::cout << "    Side-step move generated for group (" << groupStr
                    << ") in direction " << sd << " [" << DIRS[sd] << "]\n";
                moves.push_back(mv);
            }
    }
}


void Board::applyMove(const Move& m) {
    if (m.marbleIndices.empty()) return;

    int d = m.direction;
    static const char* DIRS[] = { "W", "E", "NW", "NE", "SW", "SE" };

    std::cout << "Applying move: ";
    std::cout << (occupant[m.marbleIndices[0]] == Occupant::BLACK ? "b" : "w")
        << ", group (size " << m.marbleIndices.size() << "): ";
    for (int idx : m.marbleIndices)
        std::cout << indexToNotation(idx) << " ";
    std::cout << ", direction: " << d << " (" << DIRS[d] << ")";
    std::cout << (m.isInline ? " [inline]" : " [side-step]") << "\n";

    if (m.isInline) {
        std::vector<int> sortedGroup = m.marbleIndices;
        std::sort(sortedGroup.begin(), sortedGroup.end());
        int front = sortedGroup.back();
        int dest = neighbors[front][d];
        std::cout << "  Front cell: " << indexToNotation(front)
            << ", destination: " << (dest >= 0 ? indexToNotation(dest) : "off-board") << "\n";

        // Handle pushing if necessary.
        if (dest >= 0 && occupant[dest] != Occupant::EMPTY && occupant[dest] != occupant[front]) {
            // Use m.pushCount if set (otherwise default as before).
            int pushLimit = (m.pushCount > 0 ? m.pushCount : (sortedGroup.size() == 2 ? 1 : 2));
            int current = dest;
            std::cout << "  Push detected starting at " << indexToNotation(dest) << "\n";
            for (int j = 0; j < pushLimit; j++) {
                int nextChain = neighbors[current][d];
                std::cout << "    Pushing: " << indexToNotation(current)
                    << " -> " << (nextChain >= 0 ? indexToNotation(nextChain) : "off-board") << "\n";
                if (nextChain >= 0 && occupant[nextChain] == Occupant::EMPTY) {
                    // Move the opponent marble one cell forward.
                    occupant[nextChain] = occupant[current];
                    occupant[current] = Occupant::EMPTY;
                    current = nextChain;
                }
                else if (nextChain < 0) {
                    // Marble is pushed off the board.
                    occupant[current] = Occupant::EMPTY;
                    std::cout << "    Marble pushed off the board from " << indexToNotation(current)
                        << ". This marble is now removed from play.\n";
                    // (If you are tracking counts explicitly, decrement the opponent’s count here.)
                    break;
                }
                else {
                    std::cout << "    Push failed; move aborted.\n";
                    return;
                }
            }
        }
        // Move own marbles from front to back.
        for (auto it = sortedGroup.rbegin(); it != sortedGroup.rend(); ++it) {
            int idx = *it;
            int target = neighbors[idx][d];
            std::cout << "  Moving " << indexToNotation(idx) << " to "
                << (target >= 0 ? indexToNotation(target) : "off-board") << "\n";
            if (target >= 0 && occupant[target] == Occupant::EMPTY) {
                occupant[target] = occupant[idx];
                occupant[idx] = Occupant::EMPTY;
            }
        }
    }
    else {
        // Side-step moves: move each marble individually.
        for (int idx : m.marbleIndices) {
            int target = neighbors[idx][d];
            std::cout << "  Side-stepping " << indexToNotation(idx) << " to "
                << (target >= 0 ? indexToNotation(target) : "off-board") << "\n";
            if (target >= 0 && occupant[target] == Occupant::EMPTY) {
                occupant[target] = occupant[idx];
                occupant[idx] = Occupant::EMPTY;
            }
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
