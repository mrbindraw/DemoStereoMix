#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDebug>
#include <QtTest/QTest>
#include <QQueue>

#include <Windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <Functiondiscoverykeys_devpkey.h>
#include "PolicyConfig.h"
#include <Objbase.h>
#include <Shlwapi.h>
#include <Propvarutil.h>

// from mmsys.pdb (mmsys.cpl)
DEFINE_PROPERTYKEY(PKEY_MonitorOutput, 0x24dbb0fc, 0x9311, 0x4b3d, 0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4, 0);          // Playback device key
DEFINE_PROPERTYKEY(PKEY_MonitorEnabled, 0x24dbb0fc, 0x9311, 0x4b3d, 0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4, 1);         // Listen checker state
DEFINE_PROPERTYKEY(PKEY_MonitorPauseOnBattery, 0x24dbb0fc, 0x9311, 0x4b3d, 0x9c, 0xf0, 0x18, 0xff, 0x15, 0x56, 0x39, 0xd4, 2);  // PowerMgr mode

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
    void on_cbListen_toggled(bool checked);

    void on_cbEnableSM_toggled(bool checked);

    void on_rbDisable_toggled(bool checked);

    void on_rbContinue_toggled(bool checked);

    void on_horizontalSlider_valueChanged(int value);

    void on_cBox_AudioDevices_activated(int index);

private:
    Ui::Widget *ui;

    IMMDeviceEnumerator     *_pDeviceEnumerator;
    IMMDevice               *_pDefaultDevice;
    IMMDeviceCollection     *_pDeviceCollection;
    IAudioEndpointVolume    *_pAudioEndpointVolume;
    IPropertyStore          *_pPropertyStore;
    IPolicyConfig           *_pPolicyConfig;


    std::wstring _wstrSMDevId;
    bool _isEnabledDevice;
    bool _isListenSM;
    bool _isPowerSaveEnabled;
    bool _isAppLoading;

    struct AudioDevice
    {
        QString name;
        std::wstring id;
    } _stAudioDevice;
    QList<AudioDevice> _listAudioDevices;

protected:
    void showEvent(QShowEvent *);
    float getScalarFromValue(unsigned int value);
    unsigned int getValueFromScalar(float value);
    void getStereoMixInfo();
    void getListenFlag();
    void setDefaultRecordDevice(const wchar_t *id);
    void createPlaybackDevicesList();
    void getCurrentPlaybackDevice();
    void refreshStereoMixVolume();
};

#endif // WIDGET_H
