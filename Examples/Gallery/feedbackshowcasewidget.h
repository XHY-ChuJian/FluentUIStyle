#pragma once

#include <QFrame>
#include <QPointer>

class ExInfoBarHost;

class FeedbackShowcaseWidget final : public QFrame
{
    Q_OBJECT

public:
    explicit FeedbackShowcaseWidget( QWidget* parent = nullptr );

private:
    QPointer<ExInfoBarHost> m_infoBarHost;
    int m_popupSerial = 0;
};
