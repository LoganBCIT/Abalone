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

// Write normalized version of a file to an output file.
void writeNormalizedFile(const std::string& inputFilename, const std::string& outputFilename) {
    std::vector<std::string> lines = readLines(inputFilename);
    std::ofstream out(outputFilename);
    if (!out.is_open()) {
        std::cerr << "Error: could not open output file: " << outputFilename << "\n";
        return;
    }
    for (const auto& line : lines) {
        std::string norm = normalizeBoardLine(line);
        out << norm << "\n";
    }
    out.close();
}

void compareBoardsAndMoves(const std::string& desiredFilename,
    const std::string& actualBoardFilename,
    const std::string& movesFilename) {

    // First, normalize both files and write them out.
    writeNormalizedFile(desiredFilename, "test1Normalized.board");
    writeNormalizedFile(actualBoardFilename, "1-boardsNormalized.txt");

    // Read normalized files into vectors.
    std::vector<std::string> normalizedDesired = readLines("test1Normalized.board");
    std::vector<std::string> normalizedActual = readLines("1-boardsNormalized.txt");

    // Also build sets (unique normalized lines) for set operations.
    std::set<std::string> desiredSet(normalizedDesired.begin(), normalizedDesired.end());
    std::set<std::string> actualSet(normalizedActual.begin(), normalizedActual.end());

    // Compute legal (the intersection)
    std::set<std::string> legalSet;
    std::set_intersection(desiredSet.begin(), desiredSet.end(),
        actualSet.begin(), actualSet.end(),
        std::inserter(legalSet, legalSet.begin()));

    // Compute missing: desired configurations not in actual.
    std::set<std::string> missingSet;
    std::set_difference(desiredSet.begin(), desiredSet.end(),
        actualSet.begin(), actualSet.end(),
        std::inserter(missingSet, missingSet.begin()));

    // Compute illegal/extra: configurations in actual not in desired.
    std::set<std::string> illegalSet;
    std::set_difference(actualSet.begin(), actualSet.end(),
        desiredSet.begin(), desiredSet.end(),
        std::inserter(illegalSet, illegalSet.begin()));

    // Next, count line-by-line matches.
    // For each line in normalizedActual (which came from 1-boardsNormalized.txt),
    // count if it exists in desiredSet.
    int matchCount = 0;
    for (const auto& line : normalizedActual) {
        if (desiredSet.find(line) != desiredSet.end())
            matchCount++;
    }
    // Missing count as per your definition: 
    // (total lines in test1Normalized.board) - (count of matching lines from 1-boardsNormalized.txt)
    int missingCount = normalizedDesired.size() - matchCount;

    // Output results.
    std::cout << "\n=== Legal Board Configurations (unique, normalized) ===\n";
    if (legalSet.empty())
        std::cout << "None\n";
    else {
        for (const auto& line : legalSet)
            std::cout << line << "\n";
    }
    std::cout << "Count of legal (unique normalized) configurations: " << legalSet.size() << "\n";

    std::cout << "\n=== Count of lines in 1-boardsNormalized.txt that appear in test1Normalized.board ===\n";
    std::cout << matchCount << " out of " << normalizedActual.size() << "\n";

    std::cout << "\n=== Missing Board Configurations (in desired but not in actual) ===\n";
    if (missingSet.empty())
        std::cout << "None\n";
    else {
        for (const auto& line : missingSet)
            std::cout << line << "\n";
    }
    std::cout << "Missing count (as defined): " << missingCount << "\n";

    std::cout << "\n=== Illegal Board Configurations (in actual but not in desired) ===\n";
    if (illegalSet.empty())
        std::cout << "None\n";
    else {
        for (const auto& line : illegalSet)
            std::cout << line << "\n";
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
