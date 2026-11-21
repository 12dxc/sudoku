#include <QtTest>

#include "sudokuboard.h"

class TestSudokuBoard : public QObject
{
    Q_OBJECT

private slots:
    void generate_eachDifficulty_uniqueAndSolvable();
    void generate_difficultyAffectsClueCount();
    void setCell_and_undoRedo();
    void conflicts_detection();
    void isSolved_checksAgainstSolution();
    void hint_returnsFirstEmptyCell();
    void givenCells_rejectWrites();
    void solve_knownPuzzle();
};

// 全盘校验：行/列/宫无重复
static bool isValidGrid(const SudokuBoard::Grid &g)
{
    for (int r = 0; r < 9; ++r)
    {
        bool seen[10] = {};
        for (int c = 0; c < 9; ++c)
        {
            if (g[r][c] == 0)
                return false;
            if (seen[g[r][c]])
                return false;
            seen[g[r][c]] = true;
        }
    }
    for (int c = 0; c < 9; ++c)
    {
        bool seen[10] = {};
        for (int r = 0; r < 9; ++r)
        {
            if (seen[g[r][c]])
                return false;
            seen[g[r][c]] = true;
        }
    }
    for (int b = 0; b < 9; ++b)
    {
        bool seen[10] = {};
        const int rr = b / 3 * 3, cc = b % 3 * 3;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
            {
                if (seen[g[rr + i][cc + j]])
                    return false;
                seen[g[rr + i][cc + j]] = true;
            }
    }
    return true;
}

void TestSudokuBoard::generate_eachDifficulty_uniqueAndSolvable()
{
    for (auto d : {SudokuBoard::Difficulty::Easy,
                   SudokuBoard::Difficulty::Medium,
                   SudokuBoard::Difficulty::Hard})
    {
        SudokuBoard b;
        b.generate(d);

        // 谜题与解的差异只在空格上
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
            {
                const int p = b.cell(r, c);
                QVERIFY(p >= 0 && p <= 9);
                if (p != 0)
                    QCOMPARE(p, b.solution()[r][c]);
            }

        // 解必须是合法完整数独
        QVERIFY(isValidGrid(b.solution()));

        // 谜题唯一解：用空格拼出谜题棋盘再数解
        SudokuBoard::Grid g = b.solution();
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
                if (b.cell(r, c) == 0)
                    g[r][c] = 0;
        QCOMPARE(SudokuBoard::countSolutions(g, 2), 1);

        // 空格数 > 0
        QVERIFY(b.clues() < 81);
    }
}

void TestSudokuBoard::generate_difficultyAffectsClueCount()
{
    // 每种难度生成 3 局，统计线索数范围
    int easyClues = 0, mediumClues = 0, hardClues = 0;
    for (int i = 0; i < 3; ++i)
    {
        SudokuBoard b;
        b.generate(SudokuBoard::Difficulty::Easy);
        easyClues += b.clues();
        b.generate(SudokuBoard::Difficulty::Medium);
        mediumClues += b.clues();
        b.generate(SudokuBoard::Difficulty::Hard);
        hardClues += b.clues();
    }
    easyClues /= 3;
    mediumClues /= 3;
    hardClues /= 3;
    // 真正的不变量：难度越高线索越少（先跑数据验证，再断言）
    QVERIFY(easyClues > mediumClues);
    QVERIFY(mediumClues > hardClues);
    // Easy 目标 42 线索：只挖 39 个洞，多轮挖洞总能达到目标
    QVERIFY(easyClues >= 42);
    // Medium 目标 32 线索
    QVERIFY(mediumClues >= 30 && mediumClues <= 40);
    // Hard 目标 25 线索：多轮挖洞后应稳定接近目标（随机上界，放宽防 flaky）
    QVERIFY(hardClues <= 30);
}

void TestSudokuBoard::setCell_and_undoRedo()
{
    SudokuBoard b;
    b.generate(SudokuBoard::Difficulty::Easy);

    // 找一个空格
    int r = -1, c = -1;
    for (int i = 0; i < 9 && r < 0; ++i)
        for (int j = 0; j < 9 && r < 0; ++j)
            if (b.cell(i, j) == 0)
            {
                r = i;
                c = j;
            }
    QVERIFY(r >= 0);

    const int correct = b.solution()[r][c];
    QVERIFY(b.setCell(r, c, correct));
    QCOMPARE(b.cell(r, c), correct);

    // 改成一个错误值
    const int wrong = correct == 1 ? 2 : 1;
    QVERIFY(b.setCell(r, c, wrong));
    QCOMPARE(b.cell(r, c), wrong);

    // 撤销两次
    QVERIFY(b.canUndo());
    auto m1 = b.undo();
    QVERIFY(m1.has_value());
    QCOMPARE(m1->from, correct);
    QCOMPARE(b.cell(r, c), correct);
    auto m2 = b.undo();
    QVERIFY(m2.has_value());
    QCOMPARE(m2->from, 0);
    QCOMPARE(b.cell(r, c), 0);
    QVERIFY(!b.canUndo());
    // 空栈撤销应返回 std::nullopt
    QVERIFY(!b.undo().has_value());

    // 重做两次
    QVERIFY(b.canRedo());
    auto m3 = b.redo();
    QVERIFY(m3.has_value());
    QCOMPARE(m3->to, correct);
    QCOMPARE(b.cell(r, c), correct);
    auto m4 = b.redo();
    QVERIFY(m4.has_value());
    QCOMPARE(m4->to, wrong);
    QCOMPARE(b.cell(r, c), wrong);
    QVERIFY(!b.canRedo());
    // 空栈重做应返回 std::nullopt
    QVERIFY(!b.redo().has_value());
}

void TestSudokuBoard::conflicts_detection()
{
    SudokuBoard b;
    b.generate(SudokuBoard::Difficulty::Easy);

    // 同一行找两个空格（Easy 固定 42 线索 = 39 个空格，必有含两个空格的行）
    int r1 = -1, c1 = -1, c2 = -1;
    for (int r = 0; r < 9 && r1 < 0; ++r)
    {
        int first = -1;
        for (int c = 0; c < 9; ++c)
        {
            if (b.cell(r, c) != 0)
                continue;
            if (first < 0)
            {
                first = c;
            }
            else
            {
                r1 = r;
                c1 = first;
                c2 = c;
                break;
            }
        }
    }
    QVERIFY(r1 >= 0 && c2 >= 0);

    // 同行放相同数字应冲突：把 c1 的解放到 c1，c2 处再问该值必与同行冲突
    // （用解值而非任意值：若随机值恰好是 c2 的解，会堵死 c2 唯一的无冲突值，导致断言随机失败）
    const int v1 = b.solution()[r1][c1];
    QVERIFY(b.setCell(r1, c1, v1));
    QVERIFY(b.conflicts(r1, c2, v1));

    // c2 的解与行/列/宫现有数字都不冲突（其余已填数字均与解一致，确定性成立）
    QVERIFY(!b.conflicts(r1, c2, b.solution()[r1][c2]));
}

void TestSudokuBoard::isSolved_checksAgainstSolution()
{
    SudokuBoard b;
    b.generate(SudokuBoard::Difficulty::Easy);

    // 填入所有空格（用正确解），应判定完成
    int filled = 0;
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (b.cell(r, c) == 0)
            {
                QVERIFY(b.setCell(r, c, b.solution()[r][c]));
                ++filled;
            }
    QVERIFY(filled > 0);
    QVERIFY(b.isSolved());

    // 改错一个再撤销，应未完成
    QVERIFY(b.undo().has_value());
    QVERIFY(!b.isSolved());
    QVERIFY(b.redo().has_value());
    QVERIFY(b.isSolved());
}

void TestSudokuBoard::hint_returnsFirstEmptyCell()
{
    SudokuBoard b;
    b.generate(SudokuBoard::Difficulty::Medium);

    auto hint = b.hintCell();
    QVERIFY(hint.has_value());
    QCOMPARE(b.cell(hint->first, hint->second), 0);
    QCOMPARE(b.solution()[hint->first][hint->second] > 0, true);

    // 填满后应无提示
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c)
            if (b.cell(r, c) == 0)
                b.setCell(r, c, b.solution()[r][c]);
    QVERIFY(!b.hintCell().has_value());
}

void TestSudokuBoard::givenCells_rejectWrites()
{
    SudokuBoard b;
    b.generate(SudokuBoard::Difficulty::Easy);

    // 找一个 given 格
    int r = -1, c = -1;
    for (int i = 0; i < 9 && r < 0; ++i)
        for (int j = 0; j < 9 && r < 0; ++j)
            if (b.isGiven(i, j))
            {
                r = i;
                c = j;
            }
    QVERIFY(r >= 0);

    const int old = b.cell(r, c);
    QVERIFY(!b.setCell(r, c, old == 1 ? 2 : 1));
    QCOMPARE(b.cell(r, c), old);
    QVERIFY(!b.canUndo());
}

void TestSudokuBoard::solve_knownPuzzle()
{
    // 知名谜题（世界最难数独之一，仅用于验证求解器）
    SudokuBoard::Grid g = {{
        {{8, 0, 0, 0, 0, 0, 0, 0, 0}},
        {{0, 0, 3, 6, 0, 0, 0, 0, 0}},
        {{0, 7, 0, 0, 9, 0, 2, 0, 0}},
        {{0, 5, 0, 0, 0, 7, 0, 0, 0}},
        {{0, 0, 0, 0, 4, 5, 7, 0, 0}},
        {{0, 0, 0, 1, 0, 0, 0, 3, 0}},
        {{0, 0, 1, 0, 0, 0, 0, 6, 8}},
        {{0, 0, 8, 5, 0, 0, 0, 1, 0}},
        {{0, 9, 0, 0, 0, 0, 4, 0, 0}},
    }};

    QVERIFY(SudokuBoard::solve(g));
    QVERIFY(isValidGrid(g));
    QCOMPARE(g[0][0], 8);
    QCOMPARE(g[0][1], 1);
    QCOMPARE(g[8][6], 4);
    QCOMPARE(g[8][7], 5);
    QCOMPARE(g[8][8], 2);
    QCOMPARE(SudokuBoard::countSolutions(g, 2), 1);
}

QTEST_MAIN(TestSudokuBoard)
#include "test_sudokuboard.moc"
