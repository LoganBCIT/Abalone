#include "Board.h"
#include <iostream>

int main() {
    Board b;
    // Load from file
    if (b.loadFromInputFile("Test1.input")) {
        std::cout << "Board loaded. Next to move: "
            << (b.nextToMove == Occupant::BLACK ? "Black" : "White")
            << "\n";
    }
    else {
        std::cerr << "Failed to load board.\n";
    }

    // Or you can do standard arrangement:
    // b.initStandardLayout();

    // Or Belgian Daisy:
    // b.initBelgianDaisyLayout();

    // etc.
    return 0;
}
