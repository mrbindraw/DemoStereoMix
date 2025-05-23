#include "widget.h"
#include "ui_widget.h"


Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    _isAppLoading(true)
{
    ui->setupUi(this);

    connect(ui->ckbDeviceEnable, &QCheckBox::toggled, this, &Widget::handleDeviceEnableOnToggled);
    connect(ui->hsldDeviceVolume, &QSlider::valueChanged, this, &Widget::handleDeviceVolumeOnValueChanged);
    connect(ui->ckbDeviceListen, &QCheckBox::toggled, this, &Widget::handleDeviceListenOnToggled);
    connect(ui->cmbPlaybackDevices, &QComboBox::activated, this, &Widget::handleDevicePlaybackOnActivated);
    connect(ui->rbtnPowerMgrContinue, &QRadioButton::toggled, this, &Widget::handlePowerMgrContinueOnToggled);
    connect(ui->rbtnPowerMgrDisable, &QRadioButton::toggled, this, &Widget::handlePowerMgrDisableOnToggled);

    SysAudio::getInstance().init();

    ui->cmbPlaybackDevices->insertItem(0, "Default Playback Device");

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    ui->cmbPlaybackDevices->addItems(devices.keys());
}

void Widget::showEvent(QShowEvent *)
{
    CComPtr<IMMDevice> device = getStereoMixDevice();
    if(!device)
    {
        QMessageBox::critical(this, "ERROR!", "Stereo Mix device not found!");
        return;
    }

    const bool isDeviceEnabled = SysAudio::getInstance().isDeviceEnabled(device);

    ui->ckbDeviceEnable->setChecked(isDeviceEnabled);

    _isAppLoading = false;

    handleDeviceEnableOnToggled(isDeviceEnabled);
}

QString Widget::getPlaybackDeviceName() const
{
    QVariant outValue;
    if(!SysAudio::getInstance().getPropertyValue(getStereoMixDeviceId(), PKEY_MonitorOutput, outValue))
    {
        qDebug() << "!SysAudio::getInstance().getPropertyValue: " << Q_FUNC_INFO;
        return QString();
    }

    const QString deviceIdValue = outValue.toString();
    if(deviceIdValue.isEmpty())
    {
        return QString();
    }

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    for (auto it = devices.cbegin(); it != devices.cend(); ++it)
    {
        if(it.value() == deviceIdValue)
        {
            return it.key();
        }
    }

    return QString();
}

CComPtr<IMMDevice> Widget::getStereoMixDevice() const
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

QString Widget::getStereoMixDeviceId() const
{
    return SysAudio::getInstance().getDeviceId(getStereoMixDevice());
}

void Widget::refreshStereoMixVolume()
{
    // This is fix for crash on exit app. Launch app when no recording devices in the system or all devices disabled in mmsys.cpl.
    int volume = SysAudio::getInstance().getDeviceVolume(getStereoMixDeviceId());
    if(!ui->ckbDeviceEnable->isChecked())
    {
        ui->hsldDeviceVolume->setValue(0);
        ui->lbDeviceVolume->setText(QString::number(0));
        return;
    }

    //int volume = SysAudio::getInstance().getDeviceVolume(getStereoMixDeviceId());
    ui->hsldDeviceVolume->setValue(volume);
    ui->lbDeviceVolume->setText(QString::number(volume));
}

// Enable/Disable StereoMix device
void Widget::handleDeviceEnableOnToggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    SysAudio::getInstance().setDefaultDevice(getStereoMixDeviceId()); // set StereoMix

    const bool isListenDevice = SysAudio::getInstance().isListenDevice(getStereoMixDeviceId());
    ui->ckbDeviceListen->setChecked(isListenDevice);
    SysAudio::getInstance().isDevicePowerSaveEnabled(getStereoMixDevice()) ? ui->rbtnPowerMgrDisable->setChecked(true) : ui->rbtnPowerMgrContinue->setChecked(true);

    qDebug() << Q_FUNC_INFO << checked;

    ui->hsldDeviceVolume->setEnabled(checked);
    ui->ckbDeviceListen->setEnabled(checked);
    ui->cmbPlaybackDevices->setEnabled(checked);
    ui->rbtnPowerMgrContinue->setEnabled(checked);
    ui->rbtnPowerMgrDisable->setEnabled(checked);

    if(!SysAudio::getInstance().setEndpointVisibility(getStereoMixDeviceId(), checked)) // 0/1 - disable/enable StereoMix device
    {
        qDebug() << "!SysAudio::getInstance().setEndpointVisibility: " << Q_FUNC_INFO;
        return;
    }

    const QString playbackDeviceName = getPlaybackDeviceName();
    playbackDeviceName.isEmpty() ? ui->cmbPlaybackDevices->setCurrentIndex(0) : ui->cmbPlaybackDevices->setCurrentText(playbackDeviceName);

    this->refreshStereoMixVolume();
}

// Change volume StereoMix
void Widget::handleDeviceVolumeOnValueChanged(int value)
{
    if(_isAppLoading)
    {
        return;
    }

    if(!ui->ckbDeviceEnable->isChecked())
    {
        return;
    }

    SysAudio::getInstance().setDeviceVolume(getStereoMixDeviceId(), value);
    ui->lbDeviceVolume->setText(QString::number(value));
}

// Change Listen checker state
void Widget::handleDeviceListenOnToggled(bool checked)
{
    if(_isAppLoading)
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << checked;

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorEnabled, QVariant::fromValue(checked));
}

// Choose Playback audio device
void Widget::handleDevicePlaybackOnActivated(int index)
{
    if(_isAppLoading)
    {
        return;
    }

    qDebug() << Q_FUNC_INFO << index;

    const auto &devices = SysAudio::getInstance().getDevices(EDataFlow::eRender, DEVICE_STATE_ACTIVE);
    const auto &deviceId = devices[ui->cmbPlaybackDevices->currentText()];
    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorOutput, QVariant::fromValue(deviceId));
}

// Power Management control mode
void Widget::handlePowerMgrContinueOnToggled(bool checked)
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

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(false));
}

void Widget::handlePowerMgrDisableOnToggled(bool checked)
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

    SysAudio::getInstance().setPropertyValue(getStereoMixDeviceId(), PKEY_MonitorPauseOnBattery, QVariant::fromValue(true));
}

Widget::~Widget()
{
    delete ui;
}

