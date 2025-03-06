#include "Board.h"
#include <iostream>
#include <fstream> 

int main() {
    Board board;
    board.loadFromInputFile("Test1.input");
    // Suppose nextToMove is now Occupant::BLACK or Occupant::WHITE

    // Generate moves
    auto moves = board.generateMoves(board.nextToMove);

    std::ofstream movesFile("1-moves.txt");
    std::ofstream boardsFile("1-boards.txt");

    // For each move:
    for (auto& m : moves) {
        // Write the move in the doc notation
        std::string moveNotation = Board::moveToNotation(m, board.nextToMove);
        movesFile << moveNotation << "\n";

        // Create a copy to apply the move
        Board copy = board;
        copy.applyMove(m);

        // Convert to occupant string
        std::string occupantStr = copy.toBoardString();
        boardsFile << occupantStr << "\n";
    }

    return 0;
}



