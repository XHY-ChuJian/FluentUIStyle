#pragma once

#include <QMainWindow>

class QShowEvent;

class AudiomaticPlayerWidget;
#ifdef AUDIOMATIC_ENABLE_FRAMELESS
class FluentWindowFrame;
#endif

class PlayerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PlayerWindow(QWidget *parent = nullptr);
    ~PlayerWindow() override;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void applyWindowChrome();

#ifdef AUDIOMATIC_ENABLE_FRAMELESS
    void setupTitleBar();
    FluentWindowFrame *m_windowFrame{nullptr};
#endif
    AudiomaticPlayerWidget *m_playerWidget{nullptr};
};
