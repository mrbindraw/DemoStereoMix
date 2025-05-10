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
    bool _isAppLoading;

protected:
    void showEvent(QShowEvent *);
    void getCurrentPlaybackDevice();
    void refreshStereoMixVolume();

private:
    CComPtr<IMMDevice> getStereoMixDevice() const;
    QString getStereoMixDeviceId() const;
};

#endif // WIDGET_H
