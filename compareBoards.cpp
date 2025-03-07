#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iterator>

// Helper: trim leading and trailing whitespace from a string.
std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// Normalize a board configuration line:
// Split by commas, trim each cell, sort them, and rejoin them into a canonical string.
std::string normalizeBoardLine(const std::string& line) {
    std::stringstream ss(line);
    std::string token;
    std::vector<std::string> cells;

    while (std::getline(ss, token, ',')) {
        std::string trimmed = trim(token);
        if (!trimmed.empty())
            cells.push_back(trimmed);
    }

    // Sort the cell tokens (order no longer matters)
    std::sort(cells.begin(), cells.end());

    // Join them back together with commas
    std::string normalized;
    for (size_t i = 0; i < cells.size(); ++i) {
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

// Compare the board configurations from the desired file with those from the actual file.
// This comparison is order-independent (each line is normalized to a canonical form).
void compareBoards(const std::string& desiredFilename, const std::string& actualFilename) {
    std::set<std::string> desiredSet = readNormalizedLines(desiredFilename);
    std::set<std::string> actualSet = readNormalizedLines(actualFilename);

    // Legal configurations: present in both
    std::set<std::string> legal;
    std::set_intersection(desiredSet.begin(), desiredSet.end(),
        actualSet.begin(), actualSet.end(),
        std::inserter(legal, legal.begin()));

    // Missing: in desired but not in actual
    std::set<std::string> missing;
    std::set_difference(desiredSet.begin(), desiredSet.end(),
        actualSet.begin(), actualSet.end(),
        std::inserter(missing, missing.begin()));

    // Illegal (or extra): in actual but not in desired
    std::set<std::string> illegal;
    std::set_difference(actualSet.begin(), actualSet.end(),
        desiredSet.begin(), desiredSet.end(),
        std::inserter(illegal, illegal.begin()));

    std::cout << "=== Legal Board Configurations (present in both files) ===\n";
    if (legal.empty()) {
        std::cout << "None\n";
    }
    else {
        for (const auto& line : legal)
            std::cout << line << "\n";
    }

    std::cout << "\n=== Missing Board Configurations (in desired but not in actual) ===\n";
    if (missing.empty()) {
        std::cout << "None\n";
    }
    else {
        for (const auto& line : missing)
            std::cout << line << "\n";
    }

    std::cout << "\n=== Illegal Board Configurations (in actual but not in desired) ===\n";
    if (illegal.empty()) {
        std::cout << "None\n";
    }
    else {
        for (const auto& line : illegal)
            std::cout << line << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <desired_output_file> <actual_output_file>\n";
        return 1;
    }
    std::string desiredFilename = argv[1];
    std::string actualFilename = argv[2];

    compareBoards(desiredFilename, actualFilename);
    return 0;
}
