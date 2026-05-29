#pragma once

#include "exwidgets_global.h"

#include <QMessageBox>

class QDialogButtonBox;
class ExMessageBoxPrivate;

/**
 * 使用QMessageBox模仿WinUI3的ContentDialog样式
 */
class EXWIDGETS_EXPORT ExMessageBox : public QMessageBox
{
    Q_OBJECT

public:
    explicit ExMessageBox(QWidget *parent = nullptr);
    ~ExMessageBox() override;
    ExMessageBox(Icon icon,
                 const QString &title,
                 const QString &text,
                 StandardButtons buttons = NoButton,
                 QWidget *parent = nullptr,
                 Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint);

    static StandardButton information(QWidget *parent,
                                      const QString &title,
                                      const QString &text,
                                      StandardButtons buttons = Ok,
                                      StandardButton defaultButton = NoButton);

    static StandardButton warning(QWidget *parent,
                                  const QString &title,
                                  const QString &text,
                                  StandardButtons buttons = Ok,
                                  StandardButton defaultButton = NoButton);

    static StandardButton critical(QWidget *parent,
                                   const QString &title,
                                   const QString &text,
                                   StandardButtons buttons = Ok,
                                   StandardButton defaultButton = NoButton);

    static StandardButton question(QWidget *parent,
                                   const QString &title,
                                   const QString &text,
                                   StandardButtons buttons = StandardButtons(Yes | No),
                                   StandardButton defaultButton = NoButton);

    void setVisible(bool visible) override;

    void setDetailedText(const QString &text);

    void setContentWidget(QWidget *widget);
    void setCenterButtons(bool center);
    bool centerButtons() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool event(QEvent *event) override;

private:
    Q_DECLARE_PRIVATE(ExMessageBox)
    ExMessageBoxPrivate *d_ptr;
};
