#pragma once

#include <QFrame>

class QEvent;

class AudioLevelMeterShowcaseWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit AudioLevelMeterShowcaseWidget( QWidget* parent = nullptr );

protected:
    void changeEvent( QEvent* event ) override;
};
