#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QTimer>

#include "sudokucell.h"

namespace {
constexpr char kDifficultyNames[][16] = {"简单", "中等", "困难"};
}  // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);

    timer_ = new QTimer(this);
    timer_->setInterval(1000);
    connect(timer_, &QTimer::timeout, this, &MainWindow::onTimerTick);

    // 棋盘：81 个格子填入 .ui 中的空网格布局
    auto *grid = ui->boardGrid;
    for (int r = 0; r < SudokuBoard::Size; ++r) {
        for (int c = 0; c < SudokuBoard::Size; ++c) {
            auto *cell = new SudokuCell(r, c, ui->boardFrame);
            cells_[r][c] = cell;
            grid->addWidget(cell, r, c);
            connect(cell, &SudokuCell::valueEdited, this, &MainWindow::onCellEdited);
        }
    }

    connect(ui->newGameBtn, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(ui->undoBtn, &QPushButton::clicked, this, &MainWindow::onUndo);
    connect(ui->redoBtn, &QPushButton::clicked, this, &MainWindow::onRedo);
    connect(ui->hintBtn, &QPushButton::clicked, this, &MainWindow::onHint);
    connect(ui->checkBtn, &QPushButton::clicked, this, &MainWindow::onCheckAnswer);

    // 难度分段按钮：标记动态属性供 QSS 只针对这三个按钮生效（不影响底部操作按钮）
    for (auto *btn : {ui->difficultyEasyBtn, ui->difficultyMediumBtn, ui->difficultyHardBtn})
        btn->setProperty("difficulty", true);
    // 点击难度立即重新开局（autoExclusive 先切 checked，onNewGame 读到最新难度）
    connect(ui->difficultyEasyBtn, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(ui->difficultyMediumBtn, &QPushButton::clicked, this, &MainWindow::onNewGame);
    connect(ui->difficultyHardBtn, &QPushButton::clicked, this, &MainWindow::onNewGame);

    onNewGame();
}

MainWindow::~MainWindow() {
    delete ui;
}

SudokuBoard::Difficulty MainWindow::currentDifficulty() const {
    if (ui->difficultyEasyBtn->isChecked())
        return SudokuBoard::Difficulty::Easy;
    if (ui->difficultyHardBtn->isChecked())
        return SudokuBoard::Difficulty::Hard;
    return SudokuBoard::Difficulty::Medium;
}

int MainWindow::currentDifficultyIndex() const {
    if (ui->difficultyEasyBtn->isChecked())
        return 0;
    if (ui->difficultyHardBtn->isChecked())
        return 2;
    return 1;
}

// ---------------------------------------------------------------------------
// 游戏控制
// ---------------------------------------------------------------------------

void MainWindow::onNewGame() {
    board_.generate(currentDifficulty());
    finished_ = false;

    for (int r = 0; r < SudokuBoard::Size; ++r)
        for (int c = 0; c < SudokuBoard::Size; ++c)
            refreshCell(r, c);

    elapsedSeconds_ = 0;
    ui->timerLabel->setText("⏱ " + formatTime(0));
    ui->statusLabel->setText(QString("难度：%1　线索数：%2")
                                 .arg(QString::fromUtf8(kDifficultyNames[currentDifficultyIndex()]))
                                 .arg(board_.clues()));
    statusBar()->showMessage("新游戏开始，祝你顺利！", 3000);
    startTimer();
    updateButtons();
    cells_[0][0]->setFocus();
}

void MainWindow::onCellEdited(int row, int col, int value) {
    if (finished_)
        return;
    if (!board_.setCell(row, col, value))
        return;

    refreshAllCells();
    updateButtons();

    if (board_.isSolved()) {
        finishGame();
        return;
    }
    // 若有冲突，给出轻提示
    if (value != 0 && board_.conflicts(row, col, value)) {
        ui->statusLabel->setText("⚠ 该数字与同行/列/宫冲突");
    } else {
        ui->statusLabel->setText("");
    }
}

void MainWindow::onHint() {
    if (finished_)
        return;

    // 优先提示当前焦点所在的空格
    int hr = -1, hc = -1;
    QWidget *focused = QApplication::focusWidget();
    if (auto *cell = qobject_cast<SudokuCell *>(focused)) {
        if (board_.cell(cell->row(), cell->col()) == 0) {
            hr = cell->row();
            hc = cell->col();
        }
    }
    if (hr < 0) {
        if (auto hint = board_.hintCell()) {
            hr = hint->first;
            hc = hint->second;
        }
    }
    if (hr < 0) {
        ui->statusLabel->setText("没有空格了，检查一下答案吧！");
        return;
    }

    board_.setCell(hr, hc, board_.solution()[hr][hc]);
    refreshAllCells();
    updateButtons();
    ui->statusLabel->setText(QString("提示：第 %1 行第 %2 列 → %3")
                                 .arg(hr + 1).arg(hc + 1).arg(board_.solution()[hr][hc]));
    statusBar()->showMessage("提示已填入（可撤销）", 3000);

    if (board_.isSolved())
        finishGame();
}

void MainWindow::onUndo() {
    if (!board_.canUndo() || finished_)
        return;
    const auto m = board_.undo();
    refreshAllCells();
    updateButtons();
    ui->statusLabel->setText(QString("已撤销 (%1,%2)").arg(m.row + 1).arg(m.col + 1));
    cells_[m.row][m.col]->setFocus();
}

void MainWindow::onRedo() {
    if (!board_.canRedo() || finished_)
        return;
    const auto m = board_.redo();
    refreshAllCells();
    updateButtons();
    ui->statusLabel->setText(QString("已重做 (%1,%2)").arg(m.row + 1).arg(m.col + 1));
    cells_[m.row][m.col]->setFocus();
}

void MainWindow::onCheckAnswer() {
    if (finished_)
        return;
    if (board_.isSolved()) {
        finishGame();
        return;
    }
    // 统计错误格数量（refreshAllCells 会同步高亮它们）
    int errorCount = 0;
    for (int r = 0; r < SudokuBoard::Size; ++r)
        for (int c = 0; c < SudokuBoard::Size; ++c) {
            const int v = board_.cell(r, c);
            if (!board_.isGiven(r, c) && v != 0 && board_.conflicts(r, c, v))
                ++errorCount;
        }
    refreshAllCells();
    ui->statusLabel->setText(errorCount > 0
                                 ? QString("还有 %1 个错误数字，再检查一下吧！").arg(errorCount)
                                 : "没有冲突错误，但还有空格未填完，继续加油！");
    statusBar()->showMessage("检查完成", 2000);
}

// ---------------------------------------------------------------------------
// 计时
// ---------------------------------------------------------------------------

void MainWindow::startTimer() {
    elapsedSeconds_ = 0;
    timer_->start();
}

void MainWindow::stopTimer() {
    timer_->stop();
}

void MainWindow::onTimerTick() {
    if (finished_)
        return;
    ++elapsedSeconds_;
    ui->timerLabel->setText("⏱ " + formatTime(elapsedSeconds_));
}

QString MainWindow::formatTime(int seconds) const {
    return QString("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}

// ---------------------------------------------------------------------------
// 内部工具
// ---------------------------------------------------------------------------

void MainWindow::refreshCell(int row, int col) {
    auto *cell = cells_[row][col];
    const int v = board_.cell(row, col);
    cell->setValue(v);
    cell->setGiven(board_.isGiven(row, col));
    const bool err = !board_.isGiven(row, col) && v != 0 && board_.conflicts(row, col, v);
    cell->setError(err);
}

void MainWindow::refreshAllCells() {
    for (int r = 0; r < SudokuBoard::Size; ++r)
        for (int c = 0; c < SudokuBoard::Size; ++c)
            refreshCell(r, c);
}

void MainWindow::updateButtons() {
    ui->undoBtn->setEnabled(board_.canUndo() && !finished_);
    ui->redoBtn->setEnabled(board_.canRedo() && !finished_);
    ui->hintBtn->setEnabled(!finished_);
    ui->checkBtn->setEnabled(!finished_);
}

void MainWindow::finishGame() {
    finished_ = true;
    stopTimer();
    refreshAllCells();
    updateButtons();

    const QString msg = QString("🎉 恭喜你完成了 %1 难度数独！\n\n用时：%2")
                            .arg(QString::fromUtf8(kDifficultyNames[currentDifficultyIndex()]))
                            .arg(formatTime(elapsedSeconds_));
    QMessageBox::information(this, "完成！", msg);
    ui->statusLabel->setText("🎉 已完成！点击「新游戏」再来一局");
    ui->timerLabel->setText("🏆 " + formatTime(elapsedSeconds_));
}
