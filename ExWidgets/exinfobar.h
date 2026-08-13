#pragma once

#include "exwidgets_global.h"

#include <QPointer>
#include <QString>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QToolButton;
class QVariantAnimation;

/**
 * \brief 页面内非阻塞状态通知条。
 *
 * ExInfoBar 对齐 WinUI 3 InfoBar 的主要交互：四种严重级别、可选标题、
 * 操作按钮、自定义操作控件、关闭按钮，以及不会遮挡页面内容的展开动画。
 */
class EXWIDGETS_EXPORT ExInfoBar final : public QWidget
{
    Q_OBJECT

public:
    enum Severity
    {
        Informational,
        Success,
        Warning,
        Error
    };
    Q_ENUM( Severity )

    Q_PROPERTY( Severity severity READ severity WRITE setSeverity NOTIFY severityChanged )
    Q_PROPERTY( QString title READ title WRITE setTitle NOTIFY titleChanged )
    Q_PROPERTY( QString message READ message WRITE setMessage NOTIFY messageChanged )
    Q_PROPERTY( QString actionButtonText READ actionButtonText WRITE setActionButtonText NOTIFY actionButtonTextChanged )
    Q_PROPERTY( bool open READ isOpen WRITE setOpen NOTIFY openChanged )
    Q_PROPERTY( bool closable READ isClosable WRITE setClosable NOTIFY closableChanged )
    Q_PROPERTY( bool iconVisible READ isIconVisible WRITE setIconVisible NOTIFY iconVisibleChanged )
    Q_PROPERTY( bool animationEnabled READ isAnimationEnabled WRITE setAnimationEnabled NOTIFY animationEnabledChanged )
    Q_PROPERTY( int animationDuration READ animationDuration WRITE setAnimationDuration NOTIFY animationDurationChanged )

    explicit ExInfoBar( QWidget* parent = nullptr );
    ~ExInfoBar() override;

    [[nodiscard]] Severity severity() const;
    void setSeverity( Severity severity );

    [[nodiscard]] QString title() const;
    void setTitle( const QString& title );

    [[nodiscard]] QString message() const;
    void setMessage( const QString& message );

    [[nodiscard]] QString actionButtonText() const;
    void setActionButtonText( const QString& text );

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool isClosable() const;
    void setClosable( bool closable );
    [[nodiscard]] bool isIconVisible() const;
    void setIconVisible( bool visible );
    [[nodiscard]] bool isAnimationEnabled() const;
    void setAnimationEnabled( bool enabled );
    [[nodiscard]] int animationDuration() const;
    void setAnimationDuration( int duration );

    [[nodiscard]] QPushButton* actionButton() const;
    [[nodiscard]] QWidget* actionWidget() const;
    // 接管 widget 的所有权；替换时删除原自定义操作控件。
    void setActionWidget( QWidget* widget );
    // 解除所有权并返回自定义操作控件。
    [[nodiscard]] QWidget* takeActionWidget();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;
    [[nodiscard]] bool hasHeightForWidth() const override;
    [[nodiscard]] int heightForWidth( int width ) const override;

public Q_SLOTS:
    void setOpen( bool open );
    void dismiss();

Q_SIGNALS:
    void severityChanged( ExInfoBar::Severity severity );
    void titleChanged( const QString& title );
    void messageChanged( const QString& message );
    void actionButtonTextChanged( const QString& text );
    void openChanged( bool open );
    void closableChanged( bool closable );
    void iconVisibleChanged( bool visible );
    void animationEnabledChanged( bool enabled );
    void animationDurationChanged( int duration );
    void actionTriggered();
    void closeButtonClicked();
    void opened();
    void closed();

protected:
    bool event( QEvent* event ) override;
    void paintEvent( QPaintEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    friend class ExInfoBarHost;
    void updateTextVisibility();
    void updateAccessibleText();
    void updateActionVisibility();
    void finishTransition();
    void finishPopupClose();

    class AdaptivePanel;
    class IconWidget;
    class MessageLabel;
    AdaptivePanel* m_adaptivePanel = nullptr;
    IconWidget* m_iconWidget = nullptr;
    QLabel* m_titleLabel = nullptr;
    MessageLabel* m_messageLabel = nullptr;
    QPushButton* m_actionButton = nullptr;
    QToolButton* m_closeButton = nullptr;
    QWidget* m_actionContainer = nullptr;
    QHBoxLayout* m_actionLayout = nullptr;
    QPointer<QWidget> m_actionWidget;
    QMetaObject::Connection m_actionWidgetDestroyedConnection;
    QVariantAnimation* m_heightAnimation = nullptr;
    Severity m_severity = Informational;
    QString m_title;
    QString m_message;
    QString m_actionButtonText;
    bool m_open = false;
    bool m_closable = true;
    bool m_iconVisible = true;
    bool m_animationEnabled = true;
    int m_animationDuration = 167;
};
