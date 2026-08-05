#pragma once

#include <QWidget>

class CardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CardWidget(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
};
