#pragma once

#include "systemprovider.h"

#include <QPoint>
#include <QWidget>

class QLabel;
class QProgressBar;
class QToolButton;

class MiniCapsuleWindow final : public QWidget
{
    Q_OBJECT

public:
    explicit MiniCapsuleWindow(QWidget *parent = nullptr);

signals:
    void restoreMainWindowRequested();

public slots:
    void onSnapshotUpdated(const SystemSnapshot &snapshot);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUi();

    QLabel *m_cpuLabel = nullptr;
    QProgressBar *m_cpuBar = nullptr;

    QLabel *m_memLabel = nullptr;
    QProgressBar *m_memBar = nullptr;

    QLabel *m_netDownLabel = nullptr;
    QLabel *m_netUpLabel = nullptr;

    QPoint m_dragPosition;
};
