#include "AbaloneAI.h"
#include "Board.h"

#include <limits>
#include <algorithm>
#include <chrono>
#include <iostream>


// Evaluate the board position from BLACK's perspective
// Add this constant
const int STARTING_MARBLES = 14; // Standard Abalone has 14 marbles per side

// Modify evaluatePosition to adjust weights based on game phase
int AbaloneAI::evaluatePosition(const Board& board) {
    nodesEvaluated++;

    // Count marbles for each side
    int blackMarbles = 0;
    int whiteMarbles = 0;
    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] == Occupant::BLACK)
            blackMarbles++;
        else if (board.occupant[i] == Occupant::WHITE)
            whiteMarbles++;
    }

    // Determine game phase (early, mid, late)
    float gameProgress = 1.0f - ((blackMarbles + whiteMarbles) / (float)(2 * STARTING_MARBLES));

    float earlyWeight = 1.0f - gameProgress;   // near 1.0 at start, goes to 0.0 late
    float lateWeight = gameProgress;          // near 0.0 at start, goes to 1.0 late

    // SCALES: 
//   - marbleValue: becomes bigger in the late game (focus on finishing / pushing).
//   - centerValue: more important early, less important late.
//   - cohesionValue: more important early, somewhat less late.
//   - edgeValue: potentially more important in mid-late, but you can also
//     do a mild increase if you want. In the snippet, we keep it constant to show an example.

    int marbleValue = (int)(MARBLE_VALUE * (0.5f + 1.5f * lateWeight));
    //   - when gameProgress=0 (start), factor = 0.5 => half the usual MARBLE_VALUE
    //   - when gameProgress=1 (late), factor = 2.0 => double the usual MARBLE_VALUE

    int centerValue = (int)(10 * (1.0f - 0.6f * lateWeight));
    //   - at start: factor = 1.0 => 10
    //   - at end:   factor = 0.4 => 4

    int cohesionValue = (int)(5 * (1.0f - 0.5f * lateWeight));
    //   - at start: factor = 1.0 => 5
    //   - at end:   factor = 0.5 => 2 or 3

    int edgeValue = 15;
    //   - you could also do: edgeValue = (int)(15 * (0.8f + 0.2f * lateWeight)) 
    //     if you want to boost it slightly late game.

    int mobilityValue = (int)(3 * (1.0f - 0.7f * lateWeight));
    //   - at start: factor=1.0 => 3
    //   - at end:   factor=0.3 => ~1
    //   This encourages mobility early, but it does not vanish entirely at the end.

    int formationValue = (int)(12 * (0.8f + 0.2f * earlyWeight));
    //   - maybe more relevant in mid-early, so we keep it fairly large
    //   - you can pick any function you like

    int threatValue = 10;  // If you want, you could do a partial scale, or keep it constant
    int sumitoValue = (int)(15 * (0.5f + 0.5f * lateWeight));
    //   - sumito advantage can be valuable any time, but is often more important mid-late

    int positionValue = (int)(8 * (1.0f - 0.5f * lateWeight));
    //   - for your strategic positions


    // Calculate component values
    int score = (blackMarbles - whiteMarbles) * marbleValue;

    // Center control
    int blackCenterControl = 0;
    int whiteCenterControl = 0;
    std::vector<int> centerCells = {
        Board::notationToIndex("E5"),
        Board::notationToIndex("D5"),
        Board::notationToIndex("F5"),
        Board::notationToIndex("E4"),
        Board::notationToIndex("E6")
    };
    for (int idx : centerCells) {
        if (idx >= 0) {
            if (board.occupant[idx] == Occupant::BLACK)
                blackCenterControl++;
            else if (board.occupant[idx] == Occupant::WHITE)
                whiteCenterControl++;
        }
    }
    score += (blackCenterControl - whiteCenterControl) * centerValue;

    // Group cohesion
    int blackCohesion = calculateCohesion(board, Occupant::BLACK);
    int whiteCohesion = calculateCohesion(board, Occupant::WHITE);
    score += (blackCohesion - whiteCohesion) * cohesionValue;

    // Edge danger
    int blackEdgeDanger = calculateEdgeDanger(board, Occupant::BLACK);
    int whiteEdgeDanger = calculateEdgeDanger(board, Occupant::WHITE);
    score -= (blackEdgeDanger - whiteEdgeDanger) * edgeValue;

    // Threat potential
    int blackThreats = calculateThreatPotential(board, Occupant::BLACK);
    int whiteThreats = calculateThreatPotential(board, Occupant::WHITE);
    score += (blackThreats - whiteThreats) * threatValue;

    // mobility
    int blackMobility = calculateMobility(board, Occupant::BLACK);
    int whiteMobility = calculateMobility(board, Occupant::WHITE);
    score += (blackMobility - whiteMobility) * mobilityValue;

    // positional advantage
    int blackPositionalAdv = calculatePositionalAdvantage(board, Occupant::BLACK);
    int whitePositionalAdv = calculatePositionalAdvantage(board, Occupant::WHITE);
    score += (blackPositionalAdv - whitePositionalAdv) * positionValue;


    // // Positional advantages that make the bot a pussy

    // // Sumito advantages
    // int blackSumito = calculateSumitoAdvantages(board, Occupant::BLACK);
    // int whiteSumito = calculateSumitoAdvantages(board, Occupant::WHITE);
    // score += (blackSumito - whiteSumito) * sumitoValue;

    // // Formations
    // int blackFormations = calculateFormations(board, Occupant::BLACK);
    // int whiteFormations = calculateFormations(board, Occupant::WHITE);
    // score += (blackFormations - whiteFormations) * formationValue;

    return score;
}

// Evaluate a move quickly for ordering purposes
int AbaloneAI::evaluateMove(const Board& board, const Move& move, Occupant side) {
    int score = 0;

    // Prioritize captures
    if (move.pushCount > 0) {
        score += 1000 * move.pushCount;  // Higher score for more captures
    }

    // Calculate center of the board (approximately E5 in standard notation)
    int centerIdx = Board::notationToIndex("E5");

    // Prioritize moves towards the center
    // We'll use the end positions of the marbles after the move
    Board tempBoard = board;
    tempBoard.applyMove(move);

    // Check if the move improves centralization
    double beforeCentralization = 0;
    double afterCentralization = 0;

    for (int idx : move.marbleIndices) {
        if (idx >= 0) {
            // Before position - distance from center
            auto beforeCoord = board.s_indexToCoord[idx];
            auto centerCoord = board.s_indexToCoord[centerIdx];
            int distBefore = std::abs(beforeCoord.first - centerCoord.first) +
                std::abs(beforeCoord.second - centerCoord.second);
            beforeCentralization += distBefore;

            // Calculate where this marble ended up (approximately)
            // This is a simplification, as the exact end position depends on the move mechanics
            int endIdx = board.neighbors[idx][move.direction];
            if (endIdx >= 0) {
                auto afterCoord = board.s_indexToCoord[endIdx];
                int distAfter = std::abs(afterCoord.first - centerCoord.first) +
                    std::abs(afterCoord.second - centerCoord.second);
                afterCentralization += distAfter;
            }
        }
    }

    // Add points if the move improves centralization (lower distance is better)
    if (afterCentralization < beforeCentralization) {
        score += (beforeCentralization - afterCentralization) * 10;
    }

    // Prioritize group-forming moves
    int beforeCohesion = calculateCohesion(board, side);
    int afterCohesion = calculateCohesion(tempBoard, side);
    score += (afterCohesion - beforeCohesion) * 5;

    // Penalize moves that put marbles in danger
    int beforeDanger = calculateEdgeDanger(board, side);
    int afterDanger = calculateEdgeDanger(tempBoard, side);
    score -= (afterDanger - beforeDanger) * 15;

    // Prioritize moves that increase threat potential
    int beforeThreats = calculateThreatPotential(board, side);
    int afterThreats = calculateThreatPotential(tempBoard, side);
    score += (afterThreats - beforeThreats) * 10;


    // Bonus for pushing opponent marbles off the edge
    if (move.pushCount > 0) {
        score += 50 * move.pushCount;
    }

    // Bonus for inline moves (usually more powerful)
    if (move.isInline) {
        score += 20;
    }

    // Bonus for moves that go away from the edge if we're already in danger
    if (beforeDanger > 0) {
        score += (beforeDanger - afterDanger) * 20;
    }

    return score;
}

int AbaloneAI::calculateCohesion(const Board& board, Occupant side) {
    int cohesion = 0;
    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] == side) {
            for (int d = 0; d < Board::NUM_DIRECTIONS; d++) {
                int neighbor = board.neighbors[i][d];
                if (neighbor >= 0 && board.occupant[neighbor] == side)
                    cohesion++;
            }
        }
    }
    return cohesion;
}

int AbaloneAI::calculateEdgeDanger(const Board& board, Occupant side) {
    int edgeCount = 0;
    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] == side) {
            bool onEdge = false;
            for (int d = 0; d < Board::NUM_DIRECTIONS; d++) {
                if (board.neighbors[i][d] < 0) {  // neighbor off-board
                    onEdge = true;
                    break;
                }
            }
            if (onEdge)
                edgeCount++;
        }
    }
    return edgeCount;
}

int AbaloneAI::calculateThreatPotential(const Board& board, Occupant side) {
    int threatScore = 0;

    // Check for potential threats in each direction
    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] == side) {
            for (int d = 0; d < Board::NUM_DIRECTIONS; d++) {
                int neighbor = board.neighbors[i][d];
                if (neighbor >= 0 && board.occupant[neighbor] == Occupant::EMPTY) {
                    // Check if the next cell is an opponent
                    int nextNeighbor = board.neighbors[neighbor][d];
                    if (nextNeighbor >= 0 && board.occupant[nextNeighbor] != side) {
                        threatScore++;
                    }
                }
            }
        }
    }

    return threatScore;
}

int AbaloneAI::calculateMobility(const Board& board, Occupant side) {
    int mobilityScore = 0;

    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] == side) {
            for (int d = 0; d < Board::NUM_DIRECTIONS; d++) {
                int neighbor = board.neighbors[i][d];
                // Count empty neighbors (simple mobility)
                if (neighbor >= 0 && board.occupant[neighbor] == Occupant::EMPTY) {
                    mobilityScore++;
                }
            }
        }
    }

    return mobilityScore;
}



int AbaloneAI::calculatePositionalAdvantage(const Board& board, Occupant side) {
    int positionScore = 0;

    // Define strategic positions (beyond just center)
    std::vector<int> strategicPositions = {
        // Example positions - you would customize these
        Board::notationToIndex("D4"), Board::notationToIndex("F4"),
        Board::notationToIndex("C5"), Board::notationToIndex("G5"),
        Board::notationToIndex("D6"), Board::notationToIndex("F6")
    };

    // Count control of strategic positions
    for (int idx : strategicPositions) {
        if (idx >= 0 && board.occupant[idx] == side) {
            positionScore += 8;
        }
    }

    return positionScore;
}

int AbaloneAI::calculateSumitoAdvantages(const Board& board, Occupant side) {
    int sumitoScore = 0;
    Occupant opponent = (side == Occupant::BLACK) ? Occupant::WHITE : Occupant::BLACK;

    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] != side) continue;

        for (int d = 0; d < Board::NUM_DIRECTIONS; d++) {
            // Count friendly marbles in a row
            int friendlyCount = 1;
            int currentIdx = i;

            while (friendlyCount < 3) { // Maximum 3 marbles can push in Abalone
                int nextIdx = board.neighbors[currentIdx][d];
                if (nextIdx < 0 || board.occupant[nextIdx] != side) break;
                friendlyCount++;
                currentIdx = nextIdx;
            }

            // Now check for opponent marbles in the opposite direction
            if (friendlyCount >= 2) { // Need at least 2 marbles to push
                int oppositeDir = (d + 3) % 6; // Assuming 6 directions, get opposite
                int oppositeIdx = board.neighbors[i][oppositeDir];

                if (oppositeIdx >= 0 && board.occupant[oppositeIdx] == opponent) {
                    // Count opponent marbles
                    int opponentCount = 1;
                    currentIdx = oppositeIdx;

                    while (true) {
                        int nextIdx = board.neighbors[currentIdx][oppositeDir];
                        if (nextIdx < 0 || board.occupant[nextIdx] != opponent) break;
                        opponentCount++;
                        currentIdx = nextIdx;
                    }

                    // Check if we have sumito advantage (can push opponent)
                    if (friendlyCount > opponentCount) {
                        // Higher score for pushing more marbles or being close to edge
                        int edgeDistance = 0;
                        currentIdx = oppositeIdx;
                        while (opponentCount-- > 0) {
                            edgeDistance++;
                            int nextIdx = board.neighbors[currentIdx][oppositeDir];
                            if (nextIdx < 0) {
                                // Can push off the edge!
                                sumitoScore += 100;
                                break;
                            }
                            currentIdx = nextIdx;
                        }

                        // Regular sumito advantage
                        sumitoScore += (10 - edgeDistance * 3);
                    }
                }
            }
        }
    }

    return sumitoScore;
}

// Add this function to recognize and reward strong formations
int AbaloneAI::calculateFormations(const Board& board, Occupant side) {
    int formationScore = 0;

    // Check for lines of 3 or more marbles in each direction
    for (int i = 0; i < Board::NUM_CELLS; i++) {
        if (board.occupant[i] != side) continue;

        for (int d = 0; d < Board::NUM_DIRECTIONS; d++) {
            // Count consecutive marbles in this direction
            int count = 1;
            int currentIdx = i;

            while (true) {
                int nextIdx = board.neighbors[currentIdx][d];
                if (nextIdx < 0 || board.occupant[nextIdx] != side) break;
                count++;
                currentIdx = nextIdx;
            }

            // Reward formations of 3+ marbles (stronger with more marbles)
            if (count >= 3) {
                formationScore += (count - 2) * 5;

                // Extra bonus for formations not on the edge
                bool isEdgeFormation = false;
                for (int j = 0; j < count; j++) {
                    for (int dir = 0; dir < Board::NUM_DIRECTIONS; dir++) {
                        if (board.neighbors[currentIdx][dir] < 0) {
                            isEdgeFormation = true;
                            break;
                        }
                    }
                    if (isEdgeFormation) break;
                }

                if (!isEdgeFormation) {
                    formationScore += count * 2;
                }
            }
        }
    }

    return formationScore;
}

bool AbaloneAI::isTimeUp() {
    if (timeLimit <= 0)
        return false;
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    return elapsed >= timeLimit;
}

// Helper method to update killer moves
void AbaloneAI::updateKillerMove(const Move& move, int depth) {
    // Don't store captures as killer moves (they're already prioritized)
    if (move.pushCount > 0)
        return;

    // Don't store the move if it's already the first killer move
    if (killerMoves[depth][0] == move)
        return;

    // Shift the existing killer move to the second position
    killerMoves[depth][1] = killerMoves[depth][0];

    // Store the new killer move in the first position
    killerMoves[depth][0] = move;
}

// Helper function to check if a move is a killer move
bool AbaloneAI::isKillerMove(const Move& move, int depth) const {
    return (depth < killerMoves.size() &&
        (killerMoves[depth][0] == move || killerMoves[depth][1] == move));
}

// Helper function to sort moves based on their evaluation
void AbaloneAI::orderMoves(std::vector<Move>& moves, const Board& board, Occupant side, const Move& ttMove) {
    // Define a struct to hold moves and their scores
    struct ScoredMove {
        Move move;
        int score;

        ScoredMove(const Move& m, int s) : move(m), score(s) {}

        // For sorting in descending order (highest score first)
        bool operator<(const ScoredMove& other) const {
            return score > other.score;
        }
    };

    std::vector<ScoredMove> scoredMoves;

    // Score each move
    for (const Move& move : moves) {
        int moveScore = 0;

        // 1. Highest priority: Transposition table move
        if (move == ttMove) {
            moveScore = 100000;  // Very high score
        }
        // 2. Second priority: Killer moves
        else if (isKillerMove(move, maxDepth)) {
            moveScore = 10000;  // High score, but lower than TT move

            // First killer move gets higher priority than second
            if (move == killerMoves[maxDepth][0]) {
                moveScore += 1000;
            }
        }
        // 3. Third priority: Move evaluation heuristic
        else {
            moveScore = evaluateMove(board, move, side);
        }

        scoredMoves.push_back(ScoredMove(move, moveScore));
    }

    // Sort moves by score
    std::sort(scoredMoves.begin(), scoredMoves.end());

    // Update the original vector with sorted moves
    for (size_t i = 0; i < moves.size(); i++) {
        moves[i] = scoredMoves[i].move;
    }
}

int AbaloneAI::minimax(Board& board, int depth, int alpha, int beta, bool maximizingPlayer) {
    if (isTimeUp()) {
        timeoutOccurred = true;
        return evaluatePosition(board);
    }
    if (depth == 0)
        return evaluatePosition(board);

    // Check transposition table first
    int origAlpha = alpha;
    int origBeta = beta;
    Move bestMove;
    int score;
    MoveType moveType;

    // Check if the current board position is already stored in the transposition table
    if (transpositionTable.probeEntry(board, depth, score, moveType, bestMove)) {
        // TT hit - use stored information
        if (moveType == MoveType::EXACT) {
            return score;
        }
        else if (moveType == MoveType::LOWERBOUND) {
            alpha = std::max(alpha, score);
        }
        else if (moveType == MoveType::UPPERBOUND) {
            beta = std::min(beta, score);
        }

        if (alpha >= beta) {
            pruningCount++;
            return score;
        }
    }

    Occupant currentPlayer = maximizingPlayer ? Occupant::BLACK : Occupant::WHITE;
    std::vector<Move> possibleMoves = board.generateMoves(currentPlayer);

    // Game over check: no legal moves
    if (possibleMoves.empty())
        return maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

    // Get best move from TT (for move ordering)
    Move ttBestMove;
    bool hasTTMove = transpositionTable.getBestMove(board, ttBestMove);

    // Order moves using the new ordering function
    orderMoves(possibleMoves, board, currentPlayer, hasTTMove ? ttBestMove : Move());

    MoveType entryType = MoveType::UPPERBOUND;
    Move localBestMove;

    if (maximizingPlayer) {
        int value = std::numeric_limits<int>::min();
        for (const Move& move : possibleMoves) {
            Board tempBoard = board;
            tempBoard.applyMove(move);
            int eval = minimax(tempBoard, depth - 1, alpha, beta, false);

            if (eval > value) {
                value = eval;
                localBestMove = move;  // Remember best move for TT
            }

            alpha = std::max(alpha, value);
            if (beta <= alpha) {
                pruningCount++;
                // this move caused a beta cutoff, so update killer moves
                updateKillerMove(move, depth);
                break;  // Beta cutoff
            }
        }

        // Update TT entry type
        if (value <= origAlpha) {
            entryType = MoveType::UPPERBOUND;
        }
        else if (value >= beta) {
            entryType = MoveType::LOWERBOUND;
        }
        else {
            entryType = MoveType::EXACT;
        }

        // Store in transposition table
        transpositionTable.storeEntry(board, depth, value, entryType, localBestMove);

        return value;
    }
    else {
        int value = std::numeric_limits<int>::max();
        for (const Move& move : possibleMoves) {
            Board tempBoard = board;
            tempBoard.applyMove(move);
            int eval = minimax(tempBoard, depth - 1, alpha, beta, true);

            if (eval < value) {
                value = eval;
                localBestMove = move;  // Remember best move for TT
            }

            beta = std::min(beta, value);
            if (beta <= alpha) {
                pruningCount++;
                // this move caused an alpha cutoff, so update killer moves
                updateKillerMove(move, depth);
                break;  // Alpha cutoff
            }
        }

        // Update TT entry type
        if (value <= origAlpha) {
            entryType = MoveType::UPPERBOUND;
        }
        else if (value >= beta) {
            entryType = MoveType::LOWERBOUND;
        }
        else {
            entryType = MoveType::EXACT;
        }

        // Store in transposition table
        transpositionTable.storeEntry(board, depth, value, entryType, localBestMove);

        return value;
    }
}

AbaloneAI::AbaloneAI(int depth, int timeLimitMs, size_t ttSizeInMB)
    : maxDepth(depth), nodesEvaluated(0), timeLimit(timeLimitMs),
    timeoutOccurred(false), transpositionTable(ttSizeInMB),
    killerMoves(depth + 1) {
    pruningCount = 0;
}

std::pair<Move, int> AbaloneAI::findBestMove(Board& board) {
    nodesEvaluated = 0;
    timeoutOccurred = false;
    startTime = std::chrono::high_resolution_clock::now();

    // Clear transposition table before a new search
    // transpositionTable.clearTable();
    transpositionTable.incrementAge();

    // Reset killer moves for a new search
    killerMoves = std::vector<std::array<Move, MAX_KILLER_MOVES>>(maxDepth + 1);

    Occupant currentPlayer = board.nextToMove;
    bool maximizingPlayer = (currentPlayer == Occupant::BLACK);
    std::vector<Move> possibleMoves = board.generateMoves(currentPlayer);

    if (possibleMoves.empty()) {
        Move noMove;
        return std::make_pair(noMove, 0);
    }

    // Order the root moves
    Move ttBestMove;
    bool hasTTMove = transpositionTable.getBestMove(board, ttBestMove);
    orderMoves(possibleMoves, board, currentPlayer, hasTTMove ? ttBestMove : Move());

    Move bestMove = possibleMoves[0];
    int bestScore = maximizingPlayer ? std::numeric_limits<int>::min() : std::numeric_limits<int>::max();

    // Gather statistics for move ordering effectiveness
    int totalMoves = 0;
    int bestMoveIndex = 0;

    for (size_t i = 0; i < possibleMoves.size(); i++) {
        const Move& move = possibleMoves[i];
        totalMoves++;

        Board tempBoard = board;
        tempBoard.applyMove(move);
        int score = minimax(tempBoard, maxDepth - 1,
            std::numeric_limits<int>::min(),
            std::numeric_limits<int>::max(),
            !maximizingPlayer);
        if ((maximizingPlayer && score > bestScore) || (!maximizingPlayer && score < bestScore)) {
            bestScore = score;
            bestMove = move;
            bestMoveIndex = i;
        }
        if (isTimeUp()) {
            timeoutOccurred = true;
            break;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - startTime).count();
    std::cout << "Nodes evaluated: " << nodesEvaluated << std::endl;
    std::cout << "Time taken: " << elapsed << " ms" << std::endl;
    std::cout << "Timeout occurred: " << (timeoutOccurred ? "Yes" : "No") << std::endl;
    std::cout << "Best move score: " << bestScore << std::endl;

    // Report move ordering effectiveness
    if (totalMoves > 0) {
        std::cout << "Move ordering effectiveness: best move was #" << (bestMoveIndex + 1)
            << " out of " << totalMoves << " moves" << std::endl;
        double effectiveness = 100.0 * (1.0 - static_cast<double>(bestMoveIndex) / totalMoves);
        std::cout << "Move ordering efficiency: " << effectiveness << "%" << std::endl;
    }

    return std::make_pair(bestMove, bestScore);
}

std::pair<Move, int> AbaloneAI::findBestMoveIterativeDeepening(Board& board, int maxSearchDepth) {
    nodesEvaluated = 0;
    timeoutOccurred = false;
    startTime = std::chrono::high_resolution_clock::now();

    Move bestMove;
    int bestScore = 0;
    bool foundMove = false;

    for (int depth = 1; depth <= maxSearchDepth; depth++) {
        std::cout << "Searching at depth " << depth << "..." << std::endl;

        // Check if total elapsed time exceeds the time limit
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();

        if (elapsed >= timeLimit) {
            std::cout << "Total time limit exceeded. Stopping search." << std::endl;
            break;
        }

        // Adjust remaining time for this depth
        int remainingTime = timeLimit - elapsed;

        // Temporarily modify the time limit for this depth's search
        int originalTimeLimit = timeLimit;
        timeLimit = remainingTime;

        int originalMaxDepth = maxDepth;
        maxDepth = depth;

        auto result = findBestMove(board);

        // Restore original time limit and max depth
        timeLimit = originalTimeLimit;
        maxDepth = originalMaxDepth;

        if (!timeoutOccurred) {
            bestMove = result.first;
            bestScore = result.second;
            foundMove = true;
            std::cout << "Completed depth " << depth << std::endl;
        }
        else {
            std::cout << "Timeout at depth " << depth << ", using previous result" << std::endl;
            break;
        }
    }

    if (!foundMove) {
        std::cout << "Warning: No complete depth search finished. Using 1-ply search." << std::endl;
        maxDepth = 1;
        auto result = findBestMove(board);
        bestMove = result.first;
        bestScore = result.second;
    }

    // At the end of search, print TT usage statistics
    std::cout << "Transposition table usage: " << transpositionTable.getUsage() << "%" << std::endl;
    std::cout << "TT hit rate: " << transpositionTable.getHitRate() << "%" << std::endl;

    std::cout << "Pruning count: " << pruningCount << std::endl;

    return std::make_pair(bestMove, bestScore);
}