#include "sudokucell.h"

#include <QFocusEvent>
#include <QRegularExpressionValidator>
#include <QStyle>

SudokuCell::SudokuCell(int row, int col, QWidget *parent)
    : QLineEdit(parent), row_(row), col_(col) {
    setMaxLength(1);
    setAlignment(Qt::AlignCenter);
    setValidator(new QRegularExpressionValidator(QRegularExpression("[1-9]"), this));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(44, 44);

    // 位置相关属性（固定不变）：3x3 宫边界在 styles.qss 中匹配
    setProperty("thickLeft", col % 3 == 0);
    setProperty("thickTop", row % 3 == 0);
    setProperty("thickRight", col == 8);
    setProperty("thickBottom", row == 8);
    // 状态属性（动态）
    setProperty("given", false);
    setProperty("error", false);

    connect(this, &QLineEdit::textEdited, this, &SudokuCell::onTextEdited);
}

void SudokuCell::setGiven(bool given) {
    if (given_ == given)
        return;
    given_ = given;
    setReadOnly(given);
    refreshStyle();
}

void SudokuCell::setError(bool error) {
    if (error_ == error)
        return;
    error_ = error;
    refreshStyle();
}

void SudokuCell::setValue(int value) {
    const QString text = (value >= 1 && value <= 9) ? QString::number(value) : QString();
    if (QLineEdit::text() != text)
        setText(text);
}

void SudokuCell::onTextEdited(const QString &text) {
    if (text.isEmpty()) {
        emit valueEdited(row_, col_, 0);
        return;
    }
    bool ok = false;
    const int value = text.toInt(&ok);
    if (ok && value >= 1 && value <= 9) {
        emit valueEdited(row_, col_, value);
    } else {
        setText(QString());  // 非法输入直接清空
    }
}

void SudokuCell::focusInEvent(QFocusEvent *event) {
    QLineEdit::focusInEvent(event);
    selectAll();
}

void SudokuCell::refreshStyle() {
    setProperty("given", given_);
    setProperty("error", error_);
    // 强制 QSS 属性选择器重新求值
    style()->unpolish(this);
    style()->polish(this);
}
