#pragma once

#include <QtGlobal>

#include <array>
#include <optional>
#include <utility>
#include <vector>

/**
 * SudokuBoard —— 数独核心逻辑（纯 C++，仅依赖 QtGlobal 的 Q_ASSERT 做越界断言）。
 *
 * 职责：
 *  - 生成唯一解的谜题（三种难度）
 *  - 求解（回溯算法）
 *  - 冲突检测 / 完成检测
 *  - 提示（返回第一个空格坐标）
 *  - 撤销 / 重做历史（Move 栈）
 *
 * 棋盘约定：0 表示空格，1..9 为数字。坐标均为 0..8。
 */
class SudokuBoard {
public:
    enum class Difficulty { Easy, Medium, Hard };

    static constexpr int Size = 9;
    using Grid = std::array<std::array<int, Size>, Size>;

    SudokuBoard() = default;

    /** 生成一个指定难度的新谜题（唯一解）。 */
    void generate(Difficulty d);

    // ---------- 读取 ----------
    int cell(int row, int col) const {
        Q_ASSERT(row >= 0 && row < Size && col >= 0 && col < Size);
        return current_[row][col];
    }
    /** 该格是否为开局给出的固定数字。 */
    bool isGiven(int row, int col) const {
        Q_ASSERT(row >= 0 && row < Size && col >= 0 && col < Size);
        return given_[row][col];
    }
    const Grid &solution() const { return solution_; }
    Difficulty difficulty() const { return difficulty_; }
    /** 开局给出的数字个数（线索数）。 */
    int clues() const;

    // ---------- 操作 ----------
    /** 尝试放置数字。若该格是固定格或数字非法则返回 false。 */
    bool setCell(int row, int col, int value);

    /** value 放在 (row,col) 是否与当前棋盘冲突（忽略该格自身旧值）。 */
    bool conflicts(int row, int col, int value) const;

    /** 棋盘是否已全部填完且正确。 */
    bool isSolved() const;

    /** 返回第一个空格的坐标（用于提示）；其正确数字由调用方通过 solution() 查询。 */
    std::optional<std::pair<int, int>> hintCell() const;

    // ---------- 撤销 / 重做 ----------
    struct Move {
        int row = 0, col = 0;
        int from = 0, to = 0;  // from: 旧值, to: 新值
    };

    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    /** 撤销最近一步并应用到棋盘；栈空时返回 std::nullopt。 */
    std::optional<Move> undo();
    /** 重做最近一步并应用到棋盘；栈空时返回 std::nullopt。 */
    std::optional<Move> redo();
    void clearHistory();
    std::size_t undoCount() const { return undoStack_.size(); }
    std::size_t redoCount() const { return redoStack_.size(); }

    // ---------- 静态工具 ----------
    /** 求解；有解返回 true 并填充 g。 */
    static bool solve(Grid &g);
    /** 统计解的数量（最多数到 limit）。 */
    static int countSolutions(const Grid &g, int limit = 2);

private:
    static int cluesTarget(Difficulty d);
    void generateSolution();
    void carveHoles(int targetClues);
    bool hasUniqueSolution(const Grid &g) const;

    Grid puzzle_{};      // 开局谜题（0 为空格）
    Grid solution_{};    // 完整解
    Grid current_{};     // 当前玩家棋盘
    std::array<std::array<bool, Size>, Size> given_{};  // 是否为固定格
    Difficulty difficulty_ = Difficulty::Medium;
    std::vector<Move> undoStack_;
    std::vector<Move> redoStack_;
};
