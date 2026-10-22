#pragma once

#include <QWidget>
#include <QPixmap>
#include <QList>
#include <QColor>

#include "exlyricswidget.h"

class QLabel;
class ExSpectrumWidget;
class QPaintEvent;
class QMouseEvent;
class QContextMenuEvent;
class QKeyEvent;
class QEvent;

/**
 * @brief AudiomaticMini 独立沉浸模式视听视图
 * 
 * 极简全屏大波形 + 顶部居中悬浮歌曲信息与单行卡拉OK歌词 + 底部纯净 CPU 标签。
 * 支持双击空白/波形快速退出沉浸模式、右键呼出设置菜单、Esc 秒退。
 */
class AudiomaticImmersiveView : public QWidget
{
    Q_OBJECT

public:
    explicit AudiomaticImmersiveView( QWidget* parent = nullptr );
    ~AudiomaticImmersiveView() override;

    // 音频与波形数据接口
    ExSpectrumWidget* spectrumWidget() const { return m_visualizer; }
    void pushAudioData( const QByteArray& pcmData, int channels = 1 );
    void setAudioData( const QByteArray& pcmData );
    void setSampleRate( int sampleRate );
    void clearAudio();
    void setAccentColor( const QColor& color );
    void setReflectionOpacity( float opacity );
    void syncSpectrumParameters( ExSpectrumWidget* source );

    // 封面图（保持接口兼容）
    void setCoverPixmap( const QPixmap& pixmap );
    QPixmap coverPixmap() const { return m_coverPixmap; }

    // 歌曲信息与歌词接口
    void setTrackInfo( const QString& title, const QString& artist );
    void setLyricsLines( const QList<ExLyricLine>& lines );
    void setLyricPosition( qint64 positionMs );
    void setLyricHighlightColor( const QColor& color );
    void setLyricFontSizeLevel( int level );
    void setLyricStatusMessage( const QString& msg );
    void clearLyrics();

    // CPU 监控显示接口
    void setCpuUsageText( const QString& text, const QString& tip = QString() );

signals:
    void exitRequested();
    void contextMenuRequested( const QPoint& globalPos );

protected:
    void paintEvent( QPaintEvent* event ) override;
    void mouseDoubleClickEvent( QMouseEvent* event ) override;
    void contextMenuEvent( QContextMenuEvent* event ) override;
    void keyPressEvent( QKeyEvent* event ) override;
    void changeEvent( QEvent* event ) override;

private:
    void setupUi();
    void updateThemeStyle();

    ExSpectrumWidget* m_visualizer{ nullptr };
    QWidget* m_headerWidget{ nullptr };
    QLabel* m_labelSongTitle{ nullptr };
    QLabel* m_labelSongArtist{ nullptr };
    ExLyricsWidget* m_lyricsWidget{ nullptr };
    QLabel* m_labelCpuUsage{ nullptr };

    QPixmap m_coverPixmap;
    QString m_trackTitle;
    QString m_trackArtist;
    QColor m_accentColor;
};
