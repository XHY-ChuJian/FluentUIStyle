#pragma once

#include "exwidgets_global.h"

#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>

class QVariantAnimation;
class QVBoxLayout;

/**
 * \brief 带可替换标题区域和内容区域的折叠容器。
 *
 * Header 始终可见，Content 在展开后参与正常布局，不覆盖相邻控件。
 * 支持向上或向下展开，并可追加多个任意 QWidget 作为独立 Content 面板。
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

    [[nodiscard]] QList<QWidget*> contentWidgets() const;
    // 追加独立 Content 面板并接管 widget 的所有权。
    void addContentWidget( QWidget* widget );

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
    class ContentStack;
    class ContentViewport;
    bool hasContentWidgets() const;
    bool isInternalWidget( const QWidget* widget ) const;
    bool isContentWidgetOrDescendant( const QWidget* widget ) const;
    void scheduleContentRefresh();
    void rebuildContentLayout();
    void refreshContentGeometry();
    void rebuildLayout();
    void finishTransition();

    QVBoxLayout* m_rootLayout = nullptr;
    HeaderButton* m_headerButton = nullptr;
    ContentStack* m_contentStack = nullptr;
    ContentViewport* m_contentContainer = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QPointer<QWidget> m_headerWidget;
    QList<QPointer<QWidget>> m_contentWidgets;
    QList<ContentPanel*> m_contentPanels;
    QList<QMetaObject::Connection> m_contentWidgetDestroyedConnections;
    QMetaObject::Connection m_headerWidgetDestroyedConnection;
    QVariantAnimation* m_expansionAnimation = nullptr;
    QString m_header;
    bool m_expanded = false;
    ExpandDirection m_expandDirection = Down;
    bool m_animationEnabled = true;
    bool m_contentRefreshScheduled = false;
    // 与 WinUI3 一致：收起 167 ms，展开约 333 ms。
    int m_animationDuration = 167;
    qreal m_expansionProgress = 0.0;
};
