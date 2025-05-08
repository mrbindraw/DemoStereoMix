#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDebug>
#include <QtTest/QTest>
#include <QQueue>

#include "sysaudio.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

private slots:
    void on_cbEnableSM_toggled(bool checked);

    void on_horizontalSlider_valueChanged(int value);

    void on_cbListen_toggled(bool checked);

    void on_cBox_AudioDevices_activated(int index);

    void on_rbContinue_toggled(bool checked);

    void on_rbDisable_toggled(bool checked);

private:
    Ui::Widget *ui;

    std::wstring _wstrSMDevId;
    bool _isEnabledDevice;
    bool _isListenSM;
    bool _isPowerSaveEnabled;
    bool _isAppLoading;

protected:
    void showEvent(QShowEvent *);
    void getStereoMixInfo();
    void getCurrentPlaybackDevice();
    void refreshStereoMixVolume();
};

#endif // WIDGET_H
