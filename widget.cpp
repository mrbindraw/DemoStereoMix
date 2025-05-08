#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    _isAppLoading(true)
{
    ui->setupUi(this);

    SysAudio::getInstance().init();

    ui->cBox_AudioDevices->insertItem(0, "Default Playback Device");

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    ui->cBox_AudioDevices->addItems(devices.keys());
}

void Widget::showEvent(QShowEvent *)
{
    const bool isDeviceEnabled = isStereoMixDeviceEnabled();

    ui->cbEnableSM->setChecked(isDeviceEnabled);

    _isAppLoading = false;

    on_cbEnableSM_toggled(isDeviceEnabled);
}

void Widget::getCurrentPlaybackDevice()
{
    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(getStereoMixDeviceId().toStdWString().c_str(), PKEY_MonitorOutput, outValue))
    {
        qDebug() << "!SysAudio::getInstance().getPropertyValue: " << Q_FUNC_INFO;
        return;
    }

    const QString deviceIdValue = outValue.toString();
    if(deviceIdValue.isEmpty())
    {
        ui->cBox_AudioDevices->setCurrentIndex(0); // set "Default Playback Device"
        return;
    }

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    for (auto it = devices.cbegin(); it != devices.cend(); ++it)
    {
        if(it.value() == deviceIdValue)
        {
            ui->cBox_AudioDevices->setCurrentText(it.key());
            break;
        }
    }
}

CComPtr<IMMDevice> Widget::getStereoMixDevice()
{
    CComPtr<IMMDevice> device = SysAudio::getInstance().getDevice(EDataFlow::eCapture, "Stereo Mix");
    if(!device)
    {
        device = SysAudio::getInstance().getDevice(EDataFlow::eCapture, "Стерео микшер");
    }

    if(!device)
    {
        qDebug() << "ERROR!: Stereo Mix device not found!" << Q_FUNC_INFO;
        return nullptr;
    }

    return device;
}

QString Widget::getStereoMixDeviceId()
{
    QString deviceIdStr;
    CComPtr<IMMDevice> device = getStereoMixDevice();
    if(!device)
    {
        qDebug() << "ERROR!: Stereo Mix device not found!" << Q_FUNC_INFO;
        return QString();
    }

    LPWSTR deviceId = nullptr;
    device->GetId(&deviceId);
    deviceIdStr = QString::fromWCharArray(deviceId);
    CoTaskMemFree(deviceId);
    deviceId = nullptr;

    return deviceIdStr;
}

bool Widget::isStereoMixDeviceEnabled()
{
    CComPtr<IMMDevice> device = getStereoMixDevice();
    if(!device)
    {
        qDebug() << "ERROR!: Stereo Mix device not found!" << Q_FUNC_INFO;
        return false;
    }

    DWORD stateDevice = 0;
    device->GetState(&stateDevice);
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd371410(v=vs.85).aspx
    // https://msdn.microsoft.com/en-us/library/windows/desktop/dd370823(v=vs.85).aspx
    return stateDevice >= 2 ? false : true;
}

bool Widget::isStereoMixDevicePowerSaveEnabled()
{
    CComPtr<IMMDevice> device = getStereoMixDevice();
    if(!device)
    {
        qDebug() << "ERROR!: Stereo Mix device not found!" << Q_FUNC_INFO;
        return false;
    }

    PROPVARIANT PowerMgrState;
    PropVariantInit(&PowerMgrState);

    CComPtr<IPropertyStore> PropertyStore;
    device->OpenPropertyStore(STGM_READ, &PropertyStore);
    PropertyStore->GetValue(PKEY_MonitorPauseOnBattery, &PowerMgrState);
    const bool isPowerSaveEnabled = PowerMgrState.boolVal;
    PropVariantClear(&PowerMgrState);

    return isPowerSaveEnabled;
}

bool Widget::isStereoMixDeviceListenFromDefaultDevice()
{
    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(getStereoMixDeviceId().toStdWString().c_str(), PKEY_MonitorEnabled, outValue))
    {
        qDebug() << "!SysAudio::getInstance().getPropertyValue: " << Q_FUNC_INFO;
        return false;
    }

    return outValue.value<bool>();
}

void Widget::refreshStereoMixVolume()
{
    if(!ui->cbEnableSM->isChecked())
    {
        ui->horizontalSlider->setValue(0);
        ui->lbl_Value->setText(QString::number(0));
        return;
    }

    //SysAudio::getInstance().setDefaultDevice(getStereoMixDeviceId().toStdWString().c_str()); // set StereoMix

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume = SysAudio::getInstance().getDeviceVolume();
    if(!AudioEndpointVolume)
    {
        qDebug() << "!AudioEndpointVolume: " << Q_FUNC_INFO;
        return;
    }

    float scalarVolume = 0.0f;
    AudioEndpointVolume->GetMasterVolumeLevelScalar(&scalarVolume);
    float volume = SysAudio::getValueFromScalar(scalarVolume);
    ui->horizontalSlider->setValue(volume);
    ui->lbl_Value->setText(QString::number(volume));
}

// Enable/Disable StereoMix device
void Widget::on_cbEnableSM_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    SysAudio::getInstance().setDefaultDevice(getStereoMixDeviceId().toStdWString().c_str()); // set StereoMix

    ui->cbListen->setChecked(isStereoMixDeviceListenFromDefaultDevice());
    isStereoMixDevicePowerSaveEnabled() ? ui->rbDisable->setChecked(true) : ui->rbContinue->setChecked(true);

    qDebug() << Q_FUNC_INFO << checked;

    ui->horizontalSlider->setEnabled(checked);
    ui->cbListen->setEnabled(checked);
    ui->cBox_AudioDevices->setEnabled(checked);
    ui->rbContinue->setEnabled(checked);
    ui->rbDisable->setEnabled(checked);

    if(!SysAudio::getInstance().setEndpointVisibility(getStereoMixDeviceId().toStdWString().c_str(), (int)checked)) // 0/1 - disable/enable StereoMix device
    {
        qDebug() << "!SysAudio::getInstance().setEndpointVisibility: " << Q_FUNC_INFO;
        return;
    }

    this->getCurrentPlaybackDevice();

    this->refreshStereoMixVolume();
}

// Change volume StereoMix
void Widget::on_horizontalSlider_valueChanged(int value)
{
    if(!ui->cbEnableSM->isChecked())
    {
        return;
    }

    CComPtr<IAudioEndpointVolume> AudioEndpointVolume = SysAudio::getInstance().getDeviceVolume();
    if(!AudioEndpointVolume)
    {
        qDebug() << "!AudioEndpointVolume: " << Q_FUNC_INFO;
        return;
    }

    float val = SysAudio::getScalarFromValue(value);
    AudioEndpointVolume->SetMasterVolumeLevelScalar(val, nullptr);
    ui->lbl_Value->setText(QString::number(value));
}

// Change Listen checker state
void Widget::on_cbListen_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId().toStdWString().c_str(), PKEY_MonitorEnabled, QVariant::fromValue(checked));
}

// Choose Playback audio device
void Widget::on_cBox_AudioDevices_activated(int index)
{
    qDebug() << Q_FUNC_INFO << index;

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    const auto &deviceId = devices[ui->cBox_AudioDevices->currentText()];
    const QVariant varValue = QVariant::fromValue(deviceId);
    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId().toStdWString().c_str(), PKEY_MonitorOutput, varValue);
}

// Power Management control mode
void Widget::on_rbContinue_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    if(!checked) // don't call twice SetPropertyValue when change state Power Management
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId().toStdWString().c_str(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(false));
}

void Widget::on_rbDisable_toggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    if(!checked) // don't call twice SetPropertyValue when change state Power Management
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId().toStdWString().c_str(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(true));
}

Widget::~Widget()
{
    delete ui;
}

