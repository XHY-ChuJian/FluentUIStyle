#pragma once

#include <QColor>
#include <QDialog>

class AudiomaticColorDialogPrivate;

/**
 * @brief AudiomaticMini 专属 Fluent 强调色选择弹窗。
 *
 * 提供 Windows 11 经典预设强调色色块网格、跟随系统主题选项及自定义 RGB 调色；
 * 支持实时波形跟随预览，点“确定”保存，点“取消”或 Esc 自动回滚为打开前的初始颜色。
 */
class AudiomaticColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AudiomaticColorDialog( const QColor& initialColor, QWidget* parent = nullptr );
    ~AudiomaticColorDialog() override;

    /// 获取当前选中的颜色（若为无效色 QColor() 则表示“跟随系统主题”）
    QColor selectedColor() const;

    /// 设置选中的颜色
    void setSelectedColor( const QColor& color );

signals:
    /// 用户在弹窗内点击或调整颜色时的实时预览信号
    void previewColorChanged( const QColor& color );

    /// 用户点击“确定”最终选定颜色时发出的信号
    void colorSelected( const QColor& color );

protected:
    void done( int result ) override;

private:
    AudiomaticColorDialogPrivate* d{ nullptr };
};
