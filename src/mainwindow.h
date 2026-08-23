#pragma once

#include <QMainWindow>

#include "sudokuboard.h"

class QTimer;
class SudokuCell;

namespace Ui {
class MainWindow;
}

/**
 * MainWindow —— 数独游戏主窗口。
 *
 * 布局定义在 mainwindow.ui（Qt Designer），样式在 styles.qss，
 * 本类只负责游戏逻辑与信号连接。
 *
 * 功能：多难度新游戏、错误高亮、计时器、提示、
 *       撤销/重做、检查答案。
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onNewGame();
    void onCellEdited(int row, int col, int value);
    void onHint();
    void onUndo();
    void onRedo();
    void onCheckAnswer();
    void onTimerTick();

private:
    SudokuBoard::Difficulty currentDifficulty() const;
    int currentDifficultyIndex() const;
    void refreshCell(int row, int col);
    void refreshAllCells();
    void updateButtons();
    void startTimer();
    void stopTimer();
    void finishGame();
    QString formatTime(int seconds) const;

    Ui::MainWindow *ui;
    SudokuBoard board_;
    SudokuCell *cells_[SudokuBoard::Size][SudokuBoard::Size]{};
    QTimer *timer_ = nullptr;
    int elapsedSeconds_ = 0;
    bool finished_ = false;
};
