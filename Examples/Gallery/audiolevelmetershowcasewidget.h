#pragma once

#include <QFrame>

class AudioLevelMeterShowcaseWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit AudioLevelMeterShowcaseWidget( QWidget* parent = nullptr );
};
