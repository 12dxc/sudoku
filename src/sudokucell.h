#pragma once

#include <QLineEdit>

/**
 * SudokuCell —— 数独棋盘中的单个格子（QLineEdit 子类）。
 *
 * 职责：
 *  - 只允许输入 1..9 单个数字
 *  - 通过动态属性驱动 QSS 样式（styles.qss）：
 *      given / error / thickLeft / thickTop / thickRight / thickBottom
 *  - 属性变化时刷新样式（unpolish + polish）
 */
class SudokuCell : public QLineEdit {
    Q_OBJECT
public:
    explicit SudokuCell(int row, int col, QWidget *parent = nullptr);

    /** 设置为开局给定数字（只读）。 */
    void setGiven(bool given);
    bool isGiven() const { return given_; }

    /** 设置错误高亮状态。 */
    void setError(bool error);
    bool hasError() const { return error_; }

    /** 设置数字（0 表示清空）。 */
    void setValue(int value);

    int row() const { return row_; }
    int col() const { return col_; }

signals:
    /** 用户输入完成一个数字；value 为 0 表示清空。 */
    void valueEdited(int row, int col, int value);

protected:
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void onTextEdited(const QString &text);

private:
    void refreshStyle();

    int row_;
    int col_;
    bool given_ = false;
    bool error_ = false;
};
