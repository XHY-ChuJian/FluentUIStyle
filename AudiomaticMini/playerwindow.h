#pragma once

#include <QMainWindow>

class AudiomaticPlayerWidget;
class FluentWindowFrame;

class PlayerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    ~PlayerWindow() override;

private:
    void setupTitleBar();

    FluentWindowFrame *m_windowFrame{nullptr};
    AudiomaticPlayerWidget *m_playerWidget{nullptr};
};
