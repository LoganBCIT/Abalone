#include "Board.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        end--;
    return s.substr(start, end - start);
}

int main() {
    // Open the original input file
    std::ifstream fin("Test2.input");
    if (!fin.is_open()) {
        std::cerr << "Error: could not open file: Test2.input\n";
        return 1;
    }

    // Read all non-empty lines (after trimming) into a vector.
    std::vector<std::string> nonEmptyLines;
    std::string line;
    while (std::getline(fin, line)) {
        std::string trimmed = trim(line);
        if (!trimmed.empty()) {
            nonEmptyLines.push_back(trimmed);
        }
    }
    fin.close();

    // Write these non-empty lines to a temporary file.
    std::ofstream tempOut("temp.input");
    if (!tempOut.is_open()) {
        std::cerr << "Error: could not create temporary file.\n";
        return 1;
    }
    for (const auto& l : nonEmptyLines) {
        tempOut << l << "\n";
    }
    tempOut.close();

    // Now load from the temporary file.
    Board board;
    if (!board.loadFromInputFile("temp.input")) {
        std::cerr << "Error loading board from temp.input\n";
        return 1;
    }

    // Generate moves using board.nextToMove from the file.
    auto moves = board.generateMoves(board.nextToMove);

    std::ofstream movesFile("1-moves.txt");
    std::ofstream boardsFile("1-boards.txt");

    // For each move:
    for (auto& m : moves) {
        // Write the move in document notation.
        std::string moveNotation = Board::moveToNotation(m, board.nextToMove);
        movesFile << moveNotation << "\n";

        // Create a copy and apply the move.
        Board copy = board;
        copy.applyMove(m);

        // Write the board state.
        std::string occupantStr = copy.toBoardString();
        boardsFile << occupantStr << "\n";
    }

    return 0;
}
