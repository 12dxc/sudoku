#include "sudokuboard.h"

#include <algorithm>
#include <random>

namespace {

constexpr int kRegion[SudokuBoard::Size][SudokuBoard::Size] = {
    {0, 0, 0, 1, 1, 1, 2, 2, 2},
    {0, 0, 0, 1, 1, 1, 2, 2, 2},
    {0, 0, 0, 1, 1, 1, 2, 2, 2},
    {3, 3, 3, 4, 4, 4, 5, 5, 5},
    {3, 3, 3, 4, 4, 4, 5, 5, 5},
    {3, 3, 3, 4, 4, 4, 5, 5, 5},
    {6, 6, 6, 7, 7, 7, 8, 8, 8},
    {6, 6, 6, 7, 7, 7, 8, 8, 8},
    {6, 6, 6, 7, 7, 7, 8, 8, 8},
};

/** 返回随机打乱的 1..9 序列。 */
std::vector<int> shuffledDigits(std::mt19937 &rng) {
    std::vector<int> digits{1, 2, 3, 4, 5, 6, 7, 8, 9};
    std::shuffle(digits.begin(), digits.end(), rng);
    return digits;
}

}  // namespace

// ---------------------------------------------------------------------------
// 求解器
// ---------------------------------------------------------------------------

bool SudokuBoard::solve(Grid &g) {
    // 找第一个空格
    int bestRow = -1, bestCol = -1;
    int bestCandidates = 10;
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            if (g[r][c] != 0)
                continue;
            int candidates = 0;
            for (int v = 1; v <= 9; ++v) {
                bool ok = true;
                for (int i = 0; i < Size && ok; ++i) {
                    if (g[r][i] == v || g[i][c] == v)
                        ok = false;
                }
                if (ok) {
                    const int rr = kRegion[r][c] / 3 * 3, cc = kRegion[r][c] % 3 * 3;
                    for (int i = 0; i < 3 && ok; ++i)
                        for (int j = 0; j < 3 && ok; ++j)
                            if (g[rr + i][cc + j] == v)
                                ok = false;
                }
                if (ok)
                    ++candidates;
            }
            if (candidates < bestCandidates) {
                bestCandidates = candidates;
                bestRow = r;
                bestCol = c;
                if (candidates == 1)
                    break;
            }
        }
        if (bestCandidates == 1)
            break;
    }

    if (bestRow < 0)
        return true;  // 没有空格，完成

    for (int v = 1; v <= 9; ++v) {
        bool ok = true;
        for (int i = 0; i < Size && ok; ++i)
            if (g[bestRow][i] == v || g[i][bestCol] == v)
                ok = false;
        if (ok) {
            const int rr = kRegion[bestRow][bestCol] / 3 * 3;
            const int cc = kRegion[bestRow][bestCol] % 3 * 3;
            for (int i = 0; i < 3 && ok; ++i)
                for (int j = 0; j < 3 && ok; ++j)
                    if (g[rr + i][cc + j] == v)
                        ok = false;
        }
        if (!ok)
            continue;
        g[bestRow][bestCol] = v;
        if (solve(g))
            return true;
        g[bestRow][bestCol] = 0;
    }
    return false;
}

int SudokuBoard::countSolutions(const Grid &g, int limit) {
    // 若棋盘本身冲突（某行/列/宫有重复），提前返回 0
    for (int r = 0; r < Size; ++r) {
        bool seen[10] = {};
        for (int c = 0; c < Size; ++c) {
            const int v = g[r][c];
            if (v != 0 && seen[v])
                return 0;
            if (v != 0)
                seen[v] = true;
        }
    }
    for (int c = 0; c < Size; ++c) {
        bool seen[10] = {};
        for (int r = 0; r < Size; ++r) {
            const int v = g[r][c];
            if (v != 0 && seen[v])
                return 0;
            if (v != 0)
                seen[v] = true;
        }
    }
    for (int b = 0; b < Size; ++b) {
        bool seen[10] = {};
        const int rr = b / 3 * 3, cc = b % 3 * 3;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                const int v = g[rr + i][cc + j];
                if (v != 0 && seen[v])
                    return 0;
                if (v != 0)
                    seen[v] = true;
            }
    }

    Grid copy = g;
    int count = 0;

    // 递归计数（值升序尝试，配合冲突检测）
    struct Counter {
        Grid &g;
        int limit;
        int &count;
        bool run() {
            int br = -1, bc = -1, best = 10;
            for (int r = 0; r < Size && best > 1; ++r)
                for (int c = 0; c < Size; ++c) {
                    if (g[r][c] != 0)
                        continue;
                    int cand = 0;
                    for (int v = 1; v <= 9; ++v)
                        if (canPlace(r, c, v))
                            ++cand;
                    if (cand < best) {
                        best = cand;
                        br = r;
                        bc = c;
                    }
                }
            if (br < 0) {
                ++count;
                return count >= limit;
            }
            for (int v = 1; v <= 9; ++v) {
                if (!canPlace(br, bc, v))
                    continue;
                g[br][bc] = v;
                if (run())
                    return true;
                g[br][bc] = 0;
            }
            return false;
        }
        bool canPlace(int r, int c, int v) const {
            for (int i = 0; i < SudokuBoard::Size; ++i)
                if (g[r][i] == v || g[i][c] == v)
                    return false;
            const int rr = r / 3 * 3, cc = c / 3 * 3;
            for (int i = 0; i < 3; ++i)
                for (int j = 0; j < 3; ++j)
                    if (g[rr + i][cc + j] == v)
                        return false;
            return true;
        }
    } counter{copy, limit, count};

    counter.run();
    return count;
}

// ---------------------------------------------------------------------------
// 生成
// ---------------------------------------------------------------------------

void SudokuBoard::generate(Difficulty d) {
    difficulty_ = d;
    generateSolution();
    carveHoles(cluesTarget(d));
    current_ = puzzle_;
    undoStack_.clear();
    redoStack_.clear();
}

int SudokuBoard::clues() const {
    int n = 0;
    for (int r = 0; r < Size; ++r)
        for (int c = 0; c < Size; ++c)
            if (puzzle_[r][c] != 0)
                ++n;
    return n;
}

int SudokuBoard::cluesTarget(Difficulty d) {
    switch (d) {
        case Difficulty::Easy:   return 42;  // 挖 39 个洞
        case Difficulty::Medium: return 32;  // 挖 49 个洞
        case Difficulty::Hard:   return 25;  // 挖 56 个洞
    }
    return 32;
}

void SudokuBoard::generateSolution() {
    std::mt19937 rng{std::random_device{}()};
    Grid g{};

    // 随机化填充：随机打乱数字顺序再回溯，保证每次生成的解不同
    std::vector<int> digits = shuffledDigits(rng);
    struct Filler {
        Grid &g;
        const std::vector<int> &digits;
        std::mt19937 &rng;
        bool run() {
            int br = -1, bc = -1;
            for (int r = 0; r < Size; ++r) {
                for (int c = 0; c < Size; ++c) {
                    if (g[r][c] == 0) {
                        br = r;
                        bc = c;
                        break;
                    }
                }
                if (br >= 0)
                    break;
            }
            if (br < 0)
                return true;
            // 随机顺序尝试候选数字
            std::vector<int> order = digits;
            std::shuffle(order.begin(), order.end(), rng);
            for (int v : order) {
                bool ok = true;
                for (int i = 0; i < Size && ok; ++i)
                    if (g[br][i] == v || g[i][bc] == v)
                        ok = false;
                if (ok) {
                    const int rr = br / 3 * 3, cc = bc / 3 * 3;
                    for (int i = 0; i < 3 && ok; ++i)
                        for (int j = 0; j < 3 && ok; ++j)
                            if (g[rr + i][cc + j] == v)
                                ok = false;
                }
                if (!ok)
                    continue;
                g[br][bc] = v;
                if (run())
                    return true;
                g[br][bc] = 0;
            }
            return false;
        }
    } filler{g, digits, rng};

    if (!filler.run()) {
        // 理论上不会发生；兜底用确定性求解
        Grid fallback{};
        solve(fallback);
        g = fallback;
    }
    solution_ = g;
}

void SudokuBoard::carveHoles(int targetClues) {
    puzzle_ = solution_;
    given_ = {};

    std::mt19937 rng{std::random_device{}()};
    std::vector<std::pair<int, int>> cells;
    cells.reserve(Size * Size);
    for (int r = 0; r < Size; ++r)
        for (int c = 0; c < Size; ++c)
            cells.emplace_back(r, c);
    std::shuffle(cells.begin(), cells.end(), rng);

    int holes = 0;
    const int targetHoles = Size * Size - targetClues;
    for (auto [r, c] : cells) {
        if (holes >= targetHoles)
            break;
        const int saved = puzzle_[r][c];
        puzzle_[r][c] = 0;
        if (hasUniqueSolution(puzzle_)) {
            ++holes;
        } else {
            puzzle_[r][c] = saved;  // 挖掉会破坏唯一解，回填
        }
    }
    for (int r = 0; r < Size; ++r)
        for (int c = 0; c < Size; ++c)
            given_[r][c] = puzzle_[r][c] != 0;
}

bool SudokuBoard::hasUniqueSolution(const Grid &g) const {
    return countSolutions(g, 2) == 1;
}

// ---------------------------------------------------------------------------
// 操作
// ---------------------------------------------------------------------------

bool SudokuBoard::setCell(int row, int col, int value) {
    if (row < 0 || row >= Size || col < 0 || col >= Size)
        return false;
    if (given_[row][col])
        return false;
    if (value < 0 || value > 9)
        return false;
    if (current_[row][col] == value)
        return false;

    undoStack_.push_back({row, col, current_[row][col], value});
    redoStack_.clear();
    current_[row][col] = value;
    return true;
}

SudokuBoard::Move SudokuBoard::undo() {
    if (undoStack_.empty())
        return {};
    Move m = undoStack_.back();
    undoStack_.pop_back();
    current_[m.row][m.col] = m.from;
    redoStack_.push_back(m);
    return m;
}

SudokuBoard::Move SudokuBoard::redo() {
    if (redoStack_.empty())
        return {};
    Move m = redoStack_.back();
    redoStack_.pop_back();
    current_[m.row][m.col] = m.to;
    undoStack_.push_back(m);
    return m;
}

void SudokuBoard::clearHistory() {
    undoStack_.clear();
    redoStack_.clear();
}

bool SudokuBoard::conflicts(int row, int col, int value) const {
    if (value == 0)
        return false;
    for (int i = 0; i < Size; ++i) {
        if (i != col && current_[row][i] == value)
            return true;
        if (i != row && current_[i][col] == value)
            return true;
    }
    const int rr = row / 3 * 3, cc = col / 3 * 3;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) {
            const int r = rr + i, c = cc + j;
            if ((r != row || c != col) && current_[r][c] == value)
                return true;
        }
    return false;
}

bool SudokuBoard::isSolved() const {
    for (int r = 0; r < Size; ++r)
        for (int c = 0; c < Size; ++c) {
            if (current_[r][c] == 0)
                return false;
            if (current_[r][c] != solution_[r][c])
                return false;
        }
    return true;
}

std::optional<std::pair<int, int>> SudokuBoard::hintCell() const {
    for (int r = 0; r < Size; ++r)
        for (int c = 0; c < Size; ++c)
            if (current_[r][c] == 0)
                return std::make_pair(r, c);
    return std::nullopt;
}
