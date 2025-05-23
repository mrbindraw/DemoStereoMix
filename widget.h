#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDebug>
#include <QMessageBox>

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
    void handleDeviceEnableOnToggled(bool checked);

    void handleDeviceVolumeOnValueChanged(int value);

    void handleDeviceListenOnToggled(bool checked);

    void handleDevicePlaybackOnActivated(int index);

    void handlePowerMgrContinueOnToggled(bool checked);

    void handlePowerMgrDisableOnToggled(bool checked);

private:
    Ui::Widget *ui;
    bool _isAppLoading;

protected:
    void showEvent(QShowEvent *);

private:
    QString getPlaybackDeviceName() const;
    void refreshStereoMixVolume();
    CComPtr<IMMDevice> getStereoMixDevice() const;
    QString getStereoMixDeviceId() const;
};

#endif // WIDGET_H
