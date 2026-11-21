#include "sudokuboard.h"

#include <algorithm>
#include <random>

namespace {

/** value 放在 (row,col) 是否不与 g 中已有数字冲突（行、列、宫统一检查）。
 *  g 中该格自身也参与检查；需要忽略该格旧值时（见 conflicts），调用方先将其清零。 */
bool canPlace(const SudokuBoard::Grid &g, int row, int col, int value) {
    for (int i = 0; i < SudokuBoard::Size; ++i)
        if (g[row][i] == value || g[i][col] == value)
            return false;
    const int rr = row / 3 * 3, cc = col / 3 * 3;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            if (g[rr + i][cc + j] == value)
                return false;
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// 求解器
// ---------------------------------------------------------------------------

bool SudokuBoard::solve(Grid &g) {
    // MRV：找候选值最少的空格
    int bestRow = -1, bestCol = -1;
    int bestCandidates = 10;
    for (int r = 0; r < Size; ++r) {
        for (int c = 0; c < Size; ++c) {
            if (g[r][c] != 0)
                continue;
            int candidates = 0;
            for (int v = 1; v <= 9; ++v)
                if (canPlace(g, r, c, v))
                    ++candidates;
            if (candidates == 0)
                return false;  // 某空格无候选值，死局
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
        if (!canPlace(g, bestRow, bestCol, v))
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
                        if (canPlace(g, r, c, v))
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
                if (!canPlace(g, br, bc, v))
                    continue;
                g[br][bc] = v;
                if (run())
                    return true;
                g[br][bc] = 0;
            }
            return false;
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

    // 随机化填充：每个空格按随机顺序尝试候选数字，保证每次生成的解不同
    struct Filler {
        Grid &g;
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
            // 栈上 Fisher-Yates 洗牌 1..9，避免递归内堆分配
            std::array<int, Size> order = {1, 2, 3, 4, 5, 6, 7, 8, 9};
            for (int i = Size - 1; i > 0; --i) {
                std::uniform_int_distribution<int> dist(0, i);
                std::swap(order[i], order[dist(rng)]);
            }
            for (int v : order) {
                if (!canPlace(g, br, bc, v))
                    continue;
                g[br][bc] = v;
                if (run())
                    return true;
                g[br][bc] = 0;
            }
            return false;
        }
    } filler{g, rng};

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

    // 多轮迭代挖洞：单遍贪心挖失败的格不再重试会导致达不到难度目标；
    // 每轮重新洗牌重试之前失败的格，直到达到目标线索数或整轮无进展。
    constexpr int kMaxRounds = 100;
    for (int round = 0; round < kMaxRounds && holes < targetHoles; ++round) {
        std::shuffle(cells.begin(), cells.end(), rng);
        bool progressed = false;
        for (auto [r, c] : cells) {
            if (holes >= targetHoles)
                break;
            if (puzzle_[r][c] == 0)
                continue;  // 本轮之前已挖掉
            const int saved = puzzle_[r][c];
            puzzle_[r][c] = 0;
            if (hasUniqueSolution(puzzle_)) {
                ++holes;
                progressed = true;
            } else {
                puzzle_[r][c] = saved;  // 挖掉会破坏唯一解，回填
            }
        }
        if (!progressed)
            break;  // 整轮无进展：剩余格都无法再挖
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

std::optional<SudokuBoard::Move> SudokuBoard::undo() {
    if (undoStack_.empty())
        return std::nullopt;
    Move m = undoStack_.back();
    undoStack_.pop_back();
    current_[m.row][m.col] = m.from;
    redoStack_.push_back(m);
    return m;
}

std::optional<SudokuBoard::Move> SudokuBoard::redo() {
    if (redoStack_.empty())
        return std::nullopt;
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
    Q_ASSERT(row >= 0 && row < Size && col >= 0 && col < Size);
    if (value == 0)
        return false;
    Grid g = current_;
    g[row][col] = 0;  // 忽略该格自身旧值
    return !canPlace(g, row, col, value);
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
