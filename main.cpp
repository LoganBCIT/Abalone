#include "Board.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>

// Helper: Trim whitespace from the beginning and end of a string.
std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        start++;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        end--;
    return s.substr(start, end - start);
}

int main(int argc, char* argv[]) {
    // Check for an input filename argument.
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>\n";
        return 1;
    }

    std::string inputFilename = argv[1];

    // Open the original input file.
    std::ifstream fin(inputFilename);
    if (!fin.is_open()) {
        std::cerr << "Error: could not open file: " << inputFilename << "\n";
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

    std::ofstream movesFile("moves.txt");
    std::ofstream boardsFile("boards.txt");

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
