#pragma once

#include "exwidgets_global.h"

#include <QPointer>
#include <QString>
#include <QWidget>

class QVariantAnimation;
class QVBoxLayout;

/**
 * \brief 带可替换标题区域和内容区域的折叠容器。
 *
 * Header 始终可见，Content 在展开后参与正常布局，不覆盖相邻控件。
 * 支持向上或向下展开，并可在运行时替换任意 Header/Content QWidget。
 */
class EXWIDGETS_EXPORT ExExpander final : public QWidget
{
    Q_OBJECT

public:
    enum ExpandDirection
    {
        Down,
        Up
    };
    Q_ENUM( ExpandDirection )

    Q_PROPERTY( QString header READ header WRITE setHeader NOTIFY headerChanged )
    Q_PROPERTY( bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged )
    Q_PROPERTY( ExpandDirection expandDirection READ expandDirection WRITE setExpandDirection NOTIFY expandDirectionChanged )
    Q_PROPERTY( bool animationEnabled READ isAnimationEnabled WRITE setAnimationEnabled NOTIFY animationEnabledChanged )
    Q_PROPERTY( int animationDuration READ animationDuration WRITE setAnimationDuration NOTIFY animationDurationChanged )

    explicit ExExpander( QWidget* parent = nullptr );
    ~ExExpander() override;

    [[nodiscard]] QString header() const;
    void setHeader( const QString& header );

    [[nodiscard]] bool isExpanded() const;
    [[nodiscard]] ExpandDirection expandDirection() const;
    void setExpandDirection( ExpandDirection direction );
    [[nodiscard]] bool isAnimationEnabled() const;
    void setAnimationEnabled( bool enabled );
    [[nodiscard]] int animationDuration() const;
    void setAnimationDuration( int duration );

    [[nodiscard]] QWidget* headerWidget() const;
    // 接管 widget 的所有权；可放入图标、状态和按钮等任意复合内容。
    // 交互子控件处理自身点击，Header 的其余区域仍用于展开/收起。
    void setHeaderWidget( QWidget* widget );
    [[nodiscard]] QWidget* takeHeaderWidget();

    [[nodiscard]] QWidget* contentWidget() const;
    // 接管 widget 的所有权；替换时删除原 Content 控件。
    void setContentWidget( QWidget* widget );
    [[nodiscard]] QWidget* takeContentWidget();

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

public Q_SLOTS:
    void setExpanded( bool expanded );
    void toggle();

Q_SIGNALS:
    void headerChanged( const QString& header );
    void expandedChanged( bool expanded );
    void expandDirectionChanged( ExExpander::ExpandDirection direction );
    void animationEnabledChanged( bool enabled );
    void animationDurationChanged( int duration );
    // 与 WinUI Expander 一致：展开前发出，收起动画结束后发出。
    void expanding();
    void collapsed();
    void expansionFinished( bool expanded );

protected:
    void changeEvent( QEvent* event ) override;

private:
    class HeaderButton;
    class ContentPanel;
    class ContentViewport;
    void rebuildLayout();
    void finishTransition();

    QVBoxLayout* m_rootLayout = nullptr;
    HeaderButton* m_headerButton = nullptr;
    ContentPanel* m_contentPanel = nullptr;
    ContentViewport* m_contentContainer = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QPointer<QWidget> m_headerWidget;
    QPointer<QWidget> m_contentWidget;
    QMetaObject::Connection m_headerWidgetDestroyedConnection;
    QMetaObject::Connection m_contentWidgetDestroyedConnection;
    QVariantAnimation* m_expansionAnimation = nullptr;
    QString m_header;
    bool m_expanded = false;
    ExpandDirection m_expandDirection = Down;
    bool m_animationEnabled = true;
    // 与 WinUI3 一致：收起 167 ms，展开约 333 ms。
    int m_animationDuration = 167;
    qreal m_expansionProgress = 0.0;
};
