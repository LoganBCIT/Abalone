#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iterator>

// Helper: trim leading and trailing whitespace.
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Normalize a board configuration line:
// Split by commas, trim each token, sort tokens, and rejoin them.
std::string normalizeBoardLine(const std::string& line) {
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> cells;
    while (std::getline(ss, token, ',')) {
        std::string trimmed = trim(token);
        if (!trimmed.empty())
            cells.push_back(trimmed);
    }
    std::sort(cells.begin(), cells.end());
    std::string normalized;
    for (size_t i = 0; i < cells.size(); i++) {
        if (i > 0)
            normalized += ",";
        normalized += cells[i];
    }
    return normalized;
}

// Read all normalized board configurations from a file into a set.
std::set<std::string> readNormalizedLines(const std::string& filename) {
    std::set<std::string> normLines;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file: " << filename << "\n";
        return normLines;
    }
    std::string line;
    while (std::getline(file, line)) {
        std::string norm = normalizeBoardLine(line);
        if (!norm.empty())
            normLines.insert(norm);
    }
    file.close();
    return normLines;
}

// Read a file line-by-line (without normalization) into a vector.
std::vector<std::string> readLines(const std::string& filename) {
    std::vector<std::string> lines;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file: " << filename << "\n";
        return lines;
    }
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(trim(line));
    }
    file.close();
    return lines;
}

void compareBoardsAndMoves(const std::string& desiredFilename,
    const std::string& actualBoardFilename,
    const std::string& movesFilename) {

    // --- Debug: Output raw and normalized actual board file ---
    std::cout << "=== Raw Actual Board Lines (" << actualBoardFilename << ") ===\n";
    std::vector<std::string> actualLines = readLines(actualBoardFilename);
    for (const auto& line : actualLines)
        std::cout << line << "\n";

    std::cout << "\n=== Normalized Actual Board Lines ===\n";
    std::vector<std::string> normalizedActual;
    for (const auto& line : actualLines) {
        std::string norm = normalizeBoardLine(line);
        normalizedActual.push_back(norm);
        std::cout << norm << "\n";
    }

    // --- Debug: Output raw and normalized desired board file ---
    std::cout << "\n=== Raw Desired Board Lines (" << desiredFilename << ") ===\n";
    std::vector<std::string> desiredRaw = readLines(desiredFilename);
    for (const auto& line : desiredRaw)
        std::cout << line << "\n";

    std::cout << "\n=== Normalized Desired Board Lines ===\n";
    std::set<std::string> desiredSet;
    for (const auto& line : desiredRaw) {
        std::string norm = normalizeBoardLine(line);
        desiredSet.insert(norm);
        std::cout << norm << "\n";
    }

    // Read moves file (line-by-line); assume each line corresponds to a board configuration.
    std::vector<std::string> moveLines = readLines(movesFilename);

    // Create set for actual normalized boards.
    std::set<std::string> actualSet(normalizedActual.begin(), normalizedActual.end());

    // Compute missing: desired configurations not in actual.
    std::set<std::string> missing;
    std::set_difference(desiredSet.begin(), desiredSet.end(),
        actualSet.begin(), actualSet.end(),
        std::inserter(missing, missing.begin()));

    // Compute illegal/extra: configurations in actual not in desired.
    std::set<std::string> illegal;
    std::set_difference(actualSet.begin(), actualSet.end(),
        desiredSet.begin(), desiredSet.end(),
        std::inserter(illegal, illegal.begin()));

    // Output comparisons.
    std::cout << "\n=== Legal Board Configurations (present in both files) ===\n";
    {
        std::set<std::string> legal;
        std::set_intersection(desiredSet.begin(), desiredSet.end(),
            actualSet.begin(), actualSet.end(),
            std::inserter(legal, legal.begin()));
        if (legal.empty())
            std::cout << "None\n";
        else {
            for (const auto& line : legal)
                std::cout << line << "\n";
        }
    }

    std::cout << "\n=== Missing Board Configurations (in desired but not in actual) ===\n";
    if (missing.empty())
        std::cout << "None\n";
    else {
        for (const auto& line : missing)
            std::cout << line << "\n";
    }

    std::cout << "\n=== Illegal Board Configurations (in actual but not in desired) ===\n";
    if (illegal.empty())
        std::cout << "None\n";
    else {
        bool foundAny = false;
        for (size_t i = 0; i < normalizedActual.size(); i++) {
            if (illegal.find(normalizedActual[i]) != illegal.end()) {
                std::cout << "Line " << (i + 1) << " illegal board: " << normalizedActual[i] << "\n";
                if (i < moveLines.size())
                    std::cout << "  Corresponding move: " << moveLines[i] << "\n";
                else
                    std::cout << "  (No corresponding move found)\n";
                foundAny = true;
            }
        }
        if (!foundAny)
            std::cout << "None\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <desired_board_file> <actual_board_file> <moves_file>\n";
        return 1;
    }
    std::string desiredFilename = argv[1];
    std::string actualFilename = argv[2];
    std::string movesFilename = argv[3];

    compareBoardsAndMoves(desiredFilename, actualFilename, movesFilename);
    return 0;
}
