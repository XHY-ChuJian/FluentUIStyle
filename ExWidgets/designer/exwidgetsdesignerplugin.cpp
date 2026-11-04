// Qt Designer 控件集合。只注册能直接拖到表单上的控件。
// 默认尺寸与各控件 sizeHint() 一致。ExAudioLevelMeter 的 sizeHint()
// 返回 minimumSizeHint()，默认双声道为 96×140。

#include "exaudiolevelmeter.h"
#include "exborderbeam.h"
#include "exborderbeambutton.h"
#include "exliquidgauge.h"
#include "exmultiprogressring.h"
#include "exmultiradialgauge.h"
#include "exprogressring.h"
#include "exradialgauge.h"
#include "exrangeslider.h"
#include "extimerdial.h"

#include <QtUiPlugin/QDesignerCustomWidgetCollectionInterface>
#include <QtUiPlugin/QDesignerCustomWidgetInterface>

#include <QtCore/QByteArray>
#include <QtCore/QPointF>
#include <QtCore/QVariant>
#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

using CreateWidgetFn = QWidget* ( * )( QWidget* parent );

// 拖到表单上时给出能看出来的初始状态。属性同时写在 domXml 里，保存后会进 .ui。
// ExMultiRadialGauge / ExMultiProgressRing 的数据项用 itemN* 动态属性写入 .ui。
// ExRadialGauge 的彩色区间用 rangeNFrom、rangeNTo、rangeNColor。电平表柱子只影响设计器预览。

QWidget* createBorderBeam( QWidget* parent )
{
    return new ExBorderBeam( parent );
}

QWidget* createBorderBeamButton( QWidget* parent )
{
    return new ExBorderBeamButton( QStringLiteral( "按钮" ), parent );
}

QWidget* createRangeSlider( QWidget* parent )
{
    auto* slider = new ExRangeSlider( parent );
    slider->setRange( 0, 100 );
    slider->setValues( 30, 70 );
    return slider;
}

QWidget* createProgressRing( QWidget* parent )
{
    auto* ring = new ExProgressRing( parent );
    ring->setRange( 0, 100 );
    ring->setValue( 68 );
    ring->setTitle( QStringLiteral( "进度" ) );
    return ring;
}

QWidget* createLiquidGauge( QWidget* parent )
{
    auto* gauge = new ExLiquidGauge( parent );
    gauge->setRange( 0, 100 );
    gauge->setValue( 62 );
    return gauge;
}

QWidget* createRadialGauge( QWidget* parent )
{
    auto* gauge = new ExRadialGauge( parent );
    gauge->setRange( 0, 100 );
    gauge->setValue( 72 );
    gauge->setTitle( QStringLiteral( "速度" ) );
    gauge->setUnit( QStringLiteral( "km/h" ) );
    gauge->setLabelsVisible( true );
    return gauge;
}

QWidget* createTimerDial( QWidget* parent )
{
    auto* dial = new ExTimerDial( parent );
    dial->setTime( 45LL * 60 * 1000, 60LL * 60 * 1000, false );
    return dial;
}

// 与下面的 MultiRadialGaugeProperties 保持一致。名称左右错开，避免三项叠在轴心上。
// 颜色和偏移与 Gallery 里那张 Good / Better / Perfect 图一致。
void configureMultiRadialGauge( ExMultiRadialGauge* gauge )
{
    gauge->setHubColor( QColor( 0xFA, 0xC8, 0x58 ) );
    gauge->setTitleFontPixelSize( 9 );
    gauge->setDetailFontPixelSize( 9 );
    gauge->setDetailBadgePadding( 6.0 );

    const struct ItemPreview
    {
        const char* label;
        double value;
        QColor color;
        QPointF titleOffset;
        QPointF detailOffset;
    } items[] = {
        { "Good", 20.0, QColor( 0x54, 0x70, 0xC6 ), QPointF( -0.4, 0.8 ), QPointF( -0.4, 0.95 ) },
        { "Better", 40.0, QColor( 0xB8, 0xDE, 0x29 ), QPointF( 0.0, 0.8 ), QPointF( 0.0, 0.95 ) },
        { "Perfect", 60.0, QColor( 0x55, 0x56, 0x72 ), QPointF( 0.4, 0.8 ), QPointF( 0.4, 0.95 ) },
    };

    for ( int index = 0; index < int( sizeof( items ) / sizeof( items[ 0 ] ) ); ++index )
    {
        const QByteArray prefix = QByteArray( "item" ) + QByteArray::number( index + 1 );
        const auto assign = [&]( const char* field, const QVariant& propertyValue )
        {
            const QByteArray name = prefix + field;
            gauge->setProperty( name.constData(), propertyValue );
        };
        const ItemPreview& item = items[ index ];
        assign( "Label", QString::fromUtf8( item.label ) );
        assign( "Value", item.value );
        assign( "Color", item.color );
        assign( "Visible", true );
        assign( "TitleOffset", item.titleOffset );
        assign( "DetailOffset", item.detailOffset );
    }
}

constexpr char MultiRadialGaugeProperties[] = R"(  <property name="hubColor"><color><red>250</red><green>200</green><blue>88</blue></color></property>
  <property name="titleFontPixelSize"><number>9</number></property>
  <property name="detailFontPixelSize"><number>9</number></property>
  <property name="detailBadgePadding"><double>6</double></property>
  <property name="item1Label" stdset="0"><string>Good</string></property>
  <property name="item1Value" stdset="0"><double>20</double></property>
  <property name="item1Color" stdset="0"><color><red>84</red><green>112</green><blue>198</blue></color></property>
  <property name="item1Visible" stdset="0"><bool>true</bool></property>
  <property name="item1TitleOffset" stdset="0"><pointf><x>-0.4</x><y>0.8</y></pointf></property>
  <property name="item1DetailOffset" stdset="0"><pointf><x>-0.4</x><y>0.95</y></pointf></property>
  <property name="item2Label" stdset="0"><string>Better</string></property>
  <property name="item2Value" stdset="0"><double>40</double></property>
  <property name="item2Color" stdset="0"><color><red>184</red><green>222</green><blue>41</blue></color></property>
  <property name="item2Visible" stdset="0"><bool>true</bool></property>
  <property name="item2TitleOffset" stdset="0"><pointf><x>0</x><y>0.8</y></pointf></property>
  <property name="item2DetailOffset" stdset="0"><pointf><x>0</x><y>0.95</y></pointf></property>
  <property name="item3Label" stdset="0"><string>Perfect</string></property>
  <property name="item3Value" stdset="0"><double>60</double></property>
  <property name="item3Color" stdset="0"><color><red>85</red><green>86</green><blue>114</blue></color></property>
  <property name="item3Visible" stdset="0"><bool>true</bool></property>
  <property name="item3TitleOffset" stdset="0"><pointf><x>0.4</x><y>0.8</y></pointf></property>
  <property name="item3DetailOffset" stdset="0"><pointf><x>0.4</x><y>0.95</y></pointf></property>
)";

QWidget* createMultiRadialGauge( QWidget* parent )
{
    auto* gauge = new ExMultiRadialGauge( parent );
    configureMultiRadialGauge( gauge );
    return gauge;
}

QWidget* createMultiProgressRing( QWidget* parent )
{
    auto* ring = new ExMultiProgressRing( parent );
    ring->addItem( QStringLiteral( "完成" ), 72.0, QColor( 0x00, 0x78, 0xD4 ) );
    ring->addItem( QStringLiteral( "进行" ), 45.0, QColor( 0x00, 0xB7, 0xC3 ) );
    ring->addItem( QStringLiteral( "等待" ), 18.0, QColor( 0x10, 0x7C, 0x10 ) );
    return ring;
}

QWidget* createAudioLevelMeter( QWidget* parent )
{
    auto* meter = new ExAudioLevelMeter( parent );
    // 关掉衰减，预览柱子才会停在画面上。这个开关不写入 .ui，运行时仍按类默认值动画。
    meter->setAnimationEnabled( false );
    meter->setColorMode( ExAudioLevelMeter::ThresholdColors );
    meter->setStereoLevels( -8.0, -16.0 );
    return meter;
}

enum class DesignerIcon
{
    BorderBeam,
    BorderBeamButton,
    RangeSlider,
    ProgressRing,
    LiquidGauge,
    RadialGauge,
    TimerDial,
    MultiRadialGauge,
    MultiProgressRing,
    AudioLevelMeter
};

namespace
{

QPen roundPen( const QColor& color, qreal width )
{
    return QPen( color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin );
}

QPixmap paintDesignerIcon( DesignerIcon kind, int size )
{
    QPixmap pixmap( size, size );
    pixmap.fill( Qt::transparent );

    QPainter painter( &pixmap );
    painter.setRenderHint( QPainter::Antialiasing, true );
    painter.scale( size / 24.0, size / 24.0 );

    const QColor accent( 96, 205, 255 );
    const QColor accentHot( 165, 229, 255 );
    const QColor secondary( 0, 183, 195 );
    const QColor success( 108, 203, 95 );
    const QColor warn( 255, 159, 45 );
    const QColor track( 140, 140, 140, 200 );

    painter.setBrush( Qt::NoBrush );

    switch ( kind )
    {
    case DesignerIcon::BorderBeam:
    {
        painter.setPen( roundPen( track, 1.7 ) );
        painter.drawRoundedRect( QRectF( 3.5, 5.5, 17.0, 13.0 ), 3.4, 3.4 );
        painter.setPen( roundPen( accent, 1.3 ) );
        painter.drawLine( QPointF( 7.2, 5.5 ), QPointF( 14.4, 5.5 ) );
        painter.setPen( roundPen( accentHot, 0.8 ) );
        painter.drawLine( QPointF( 9.0, 5.5 ), QPointF( 12.2, 5.5 ) );
        break;
    }
    case DesignerIcon::BorderBeamButton:
    {
        painter.setPen( roundPen( track, 1.7 ) );
        painter.drawRoundedRect( QRectF( 3.8, 7.6, 16.4, 8.8 ), 4.4, 4.4 );
        painter.setPen( roundPen( accent, 1.3 ) );
        painter.drawLine( QPointF( 7.0, 7.6 ), QPointF( 13.8, 7.6 ) );
        painter.setPen( roundPen( accentHot, 0.8 ) );
        painter.drawLine( QPointF( 8.6, 7.6 ), QPointF( 11.8, 7.6 ) );
        break;
    }
    case DesignerIcon::RangeSlider:
    {
        painter.setPen( roundPen( track, 2.0 ) );
        painter.drawLine( QPointF( 3.2, 12.0 ), QPointF( 20.8, 12.0 ) );
        painter.setPen( roundPen( accent, 2.4 ) );
        painter.drawLine( QPointF( 8.0, 12.0 ), QPointF( 16.2, 12.0 ) );
        painter.setPen( roundPen( QColor( 50, 50, 50, 160 ), 0.9 ) );
        painter.setBrush( accent );
        painter.drawEllipse( QRectF( 6.4, 9.4, 5.2, 5.2 ) );
        painter.setBrush( accentHot );
        painter.drawEllipse( QRectF( 13.6, 9.4, 5.2, 5.2 ) );
        break;
    }
    case DesignerIcon::ProgressRing:
    {
        const QRectF ring( 4.2, 4.2, 15.6, 15.6 );
        painter.setPen( roundPen( track, 2.4 ) );
        painter.drawArc( ring, 0, 360 * 16 );
        painter.setPen( roundPen( accent, 2.4 ) );
        painter.drawArc( ring, 90 * 16, -245 * 16 );
        break;
    }
    case DesignerIcon::LiquidGauge:
    {
        const QRectF bowl( 4.4, 4.4, 15.2, 15.2 );
        painter.setPen( roundPen( track, 1.7 ) );
        painter.drawEllipse( bowl );
        QPainterPath clip;
        clip.addEllipse( bowl.adjusted( 1.0, 1.0, -1.0, -1.0 ) );
        painter.save();
        painter.setClipPath( clip );
        QPainterPath wave;
        wave.moveTo( 4.4, 13.2 );
        wave.cubicTo( 8.4, 11.0, 11.6, 15.4, 19.6, 13.0 );
        wave.lineTo( 19.6, 20.0 );
        wave.lineTo( 4.4, 20.0 );
        wave.closeSubpath();
        painter.fillPath( wave, accent );
        painter.restore();
        painter.setPen( roundPen( track, 1.7 ) );
        painter.setBrush( Qt::NoBrush );
        painter.drawEllipse( bowl );
        break;
    }
    case DesignerIcon::RadialGauge:
    {
        const QRectF dial( 4.0, 4.2, 16.0, 16.0 );
        painter.setPen( roundPen( track, 1.8 ) );
        painter.drawArc( dial, 225 * 16, -270 * 16 );
        painter.setPen( roundPen( accent, 1.8 ) );
        painter.drawLine( QPointF( 12.0, 13.2 ), QPointF( 16.8, 7.4 ) );
        painter.setPen( Qt::NoPen );
        painter.setBrush( accent );
        painter.drawEllipse( QRectF( 10.6, 11.8, 2.8, 2.8 ) );
        break;
    }
    case DesignerIcon::TimerDial:
    {
        const QRectF dial( 4.2, 4.2, 15.6, 15.6 );
        painter.setPen( roundPen( track, 2.3 ) );
        painter.drawArc( dial, 0, 360 * 16 );
        painter.setPen( roundPen( accent, 2.3 ) );
        painter.drawArc( dial, 90 * 16, -270 * 16 );
        painter.setPen( roundPen( accentHot, 1.5 ) );
        painter.drawLine( QPointF( 12.0, 12.0 ), QPointF( 12.0, 7.4 ) );
        painter.setPen( Qt::NoPen );
        painter.setBrush( accent );
        painter.drawEllipse( QRectF( 10.7, 10.7, 2.6, 2.6 ) );
        break;
    }
    case DesignerIcon::MultiRadialGauge:
    {
        const QRectF dial( 4.0, 4.2, 16.0, 16.0 );
        painter.setPen( roundPen( track, 1.8 ) );
        painter.drawArc( dial, 225 * 16, -270 * 16 );
        painter.setPen( roundPen( accent, 1.7 ) );
        painter.drawLine( QPointF( 12.0, 13.0 ), QPointF( 7.2, 8.0 ) );
        painter.setPen( roundPen( warn, 1.7 ) );
        painter.drawLine( QPointF( 12.0, 13.0 ), QPointF( 17.2, 8.4 ) );
        painter.setPen( Qt::NoPen );
        painter.setBrush( QColor( 250, 200, 88 ) );
        painter.drawEllipse( QRectF( 10.5, 11.5, 3.0, 3.0 ) );
        break;
    }
    case DesignerIcon::MultiProgressRing:
    {
        painter.setPen( roundPen( accent, 2.2 ) );
        painter.drawArc( QRectF( 3.6, 3.6, 16.8, 16.8 ), 90 * 16, -230 * 16 );
        painter.setPen( roundPen( secondary, 2.2 ) );
        painter.drawArc( QRectF( 7.0, 7.0, 10.0, 10.0 ), 90 * 16, -160 * 16 );
        painter.setPen( roundPen( success, 2.0 ) );
        painter.drawArc( QRectF( 10.0, 10.0, 4.0, 4.0 ), 90 * 16, -110 * 16 );
        break;
    }
    case DesignerIcon::AudioLevelMeter:
    {
        const qreal xs[] = { 5.2, 10.4, 15.6 };
        const qreal hs[] = { 8.5, 13.2, 10.0 };
        const QColor fills[] = { accent, success, warn };
        painter.setPen( Qt::NoPen );
        for ( int i = 0; i < 3; ++i )
        {
            painter.setBrush( fills[ i ] );
            painter.drawRoundedRect( QRectF( xs[ i ], 18.4 - hs[ i ], 3.4, hs[ i ] ), 1.1, 1.1 );
        }
        break;
    }
    }

    return pixmap;
}

QIcon designerIcon( DesignerIcon kind )
{
    static_assert( int( DesignerIcon::AudioLevelMeter ) + 1 == 10, "designerIcon cache size" );
    static QIcon cache[ 10 ];
    const int index = int( kind );
    if ( cache[ index ].isNull() )
    {
        QIcon icon;
        icon.addPixmap( paintDesignerIcon( kind, 22 ) );
        icon.addPixmap( paintDesignerIcon( kind, 32 ) );
        icon.addPixmap( paintDesignerIcon( kind, 44 ) );
        cache[ index ] = icon;
    }
    return cache[ index ];
}

} // namespace

struct WidgetSpec
{
    const char* className;
    const char* objectName;
    const char* includeFile;
    const char* toolTip;
    const char* properties;
    int width;
    int height;
    bool container;
    DesignerIcon icon;
    CreateWidgetFn create;
};

class ExDesignerWidgetPlugin final : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES( QDesignerCustomWidgetInterface )

public:
    explicit ExDesignerWidgetPlugin( const WidgetSpec& spec, QObject* parent = nullptr )
        : QObject( parent )
        , m_spec( spec )
    {
    }

    [[nodiscard]] bool isContainer() const override
    {
        return m_spec.container;
    }

    [[nodiscard]] bool isInitialized() const override
    {
        return m_initialized;
    }

    [[nodiscard]] QIcon icon() const override
    {
        return designerIcon( m_spec.icon );
    }

    [[nodiscard]] QString domXml() const override
    {
        return QString::fromLatin1(
                   "<ui language=\"c++\">\n"
                   " <widget class=\"%1\" name=\"%2\">\n"
                   "  <property name=\"geometry\">\n"
                   "   <rect>\n"
                   "    <x>0</x>\n"
                   "    <y>0</y>\n"
                   "    <width>%3</width>\n"
                   "    <height>%4</height>\n"
                   "   </rect>\n"
                   "  </property>\n"
                   "%5"
                   " </widget>\n"
                   "</ui>" )
            .arg( QLatin1String( m_spec.className ),
                  QLatin1String( m_spec.objectName ),
                  QString::number( m_spec.width ),
                  QString::number( m_spec.height ),
                  QString::fromUtf8( m_spec.properties ) );
    }

    [[nodiscard]] QString group() const override
    {
        return QStringLiteral( "ExWidgets" );
    }

    [[nodiscard]] QString includeFile() const override
    {
        return QLatin1String( m_spec.includeFile );
    }

    [[nodiscard]] QString name() const override
    {
        return QLatin1String( m_spec.className );
    }

    [[nodiscard]] QString toolTip() const override
    {
        return QString::fromUtf8( m_spec.toolTip );
    }

    [[nodiscard]] QString whatsThis() const override
    {
        return toolTip();
    }

    [[nodiscard]] QWidget* createWidget( QWidget* parent ) override
    {
        return m_spec.create( parent );
    }

    void initialize( QDesignerFormEditorInterface* ) override
    {
        if ( m_initialized )
        {
            return;
        }
        m_initialized = true;
    }

private:
    WidgetSpec m_spec;
    bool m_initialized = false;
};

class ExWidgetsDesignerCollection final : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface" )
    Q_INTERFACES( QDesignerCustomWidgetCollectionInterface )

public:
    explicit ExWidgetsDesignerCollection( QObject* parent = nullptr )
        : QObject( parent )
    {
        const WidgetSpec specs[] = {
            { "ExBorderBeam", "exBorderBeam", "exborderbeam.h", "渐变边框容器", "", 320, 180, true,
              DesignerIcon::BorderBeam, &createBorderBeam },
            { "ExBorderBeamButton", "exBorderBeamButton", "exborderbeambutton.h", "渐变边框按钮",
              "  <property name=\"text\"><string>按钮</string></property>\n", 120, 40, false,
              DesignerIcon::BorderBeamButton, &createBorderBeamButton },
            { "ExRangeSlider", "exRangeSlider", "exrangeslider.h", "双端范围滑块",
              "  <property name=\"lowerValue\"><number>30</number></property>\n"
              "  <property name=\"upperValue\"><number>70</number></property>\n",
              160, 32, false, DesignerIcon::RangeSlider, &createRangeSlider },
            { "ExProgressRing", "exProgressRing", "exprogressring.h", "环形进度",
              "  <property name=\"value\"><number>68</number></property>\n"
              "  <property name=\"title\"><string>进度</string></property>\n",
              120, 120, false, DesignerIcon::ProgressRing, &createProgressRing },
            { "ExLiquidGauge", "exLiquidGauge", "exliquidgauge.h", "水波进度",
              "  <property name=\"value\"><number>62</number></property>\n", 180, 180, false, DesignerIcon::LiquidGauge,
              &createLiquidGauge },
            { "ExRadialGauge", "exRadialGauge", "exradialgauge.h", "径向仪表",
              "  <property name=\"minimum\"><number>0</number></property>\n"
              "  <property name=\"maximum\"><number>100</number></property>\n"
              "  <property name=\"value\"><number>72</number></property>\n"
              "  <property name=\"title\"><string>速度</string></property>\n"
              "  <property name=\"unit\"><string>km/h</string></property>\n"
              "  <property name=\"labelsVisible\"><bool>true</bool></property>\n",
              240, 240, false, DesignerIcon::RadialGauge, &createRadialGauge },
            { "ExTimerDial", "exTimerDial", "extimerdial.h", "计时表盘",
              "  <property name=\"totalMilliseconds\"><longlong>3600000</longlong></property>\n"
              "  <property name=\"remainingMilliseconds\"><longlong>2700000</longlong></property>\n",
              260, 260, false, DesignerIcon::TimerDial, &createTimerDial },
            { "ExMultiRadialGauge", "exMultiRadialGauge", "exmultiradialgauge.h", "多指针径向仪表",
              MultiRadialGaugeProperties, 320, 260, false, DesignerIcon::MultiRadialGauge, &createMultiRadialGauge },
            { "ExMultiProgressRing", "exMultiProgressRing", "exmultiprogressring.h", "多环进度", "", 220, 220, false,
              DesignerIcon::MultiProgressRing, &createMultiProgressRing },
            { "ExAudioLevelMeter", "exAudioLevelMeter", "exaudiolevelmeter.h", "音频电平表",
              "  <property name=\"colorMode\"><enum>ExAudioLevelMeter::ThresholdColors</enum></property>\n", 96, 140,
              false, DesignerIcon::AudioLevelMeter, &createAudioLevelMeter },
        };

        m_widgets.reserve( int( sizeof( specs ) / sizeof( specs[ 0 ] ) ) );
        for ( const WidgetSpec& spec : specs )
        {
            m_widgets.append( new ExDesignerWidgetPlugin( spec, this ) );
        }
    }

    [[nodiscard]] QList<QDesignerCustomWidgetInterface*> customWidgets() const override
    {
        return m_widgets;
    }

private:
    QList<QDesignerCustomWidgetInterface*> m_widgets;
};

#include "exwidgetsdesignerplugin.moc"
