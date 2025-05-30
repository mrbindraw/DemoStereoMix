This Demo show how to change settings StereoMix 
(mmsys.cpl Recording->Stereo Mix: Enable/Disable device, Volume, ListenChecker, Playback device, Power Management)
without thirdparty libs.

COM Interfaces: IMMDevice, IMMDeviceEnumerator, IMMDeviceCollection, IAudioEndpointVolume, IPropertyStore, IPolicyConfig


-------------
Correct test:
-------------
OS: Windows x64 Vista, 7, 8, 8.1, 10
IDE: Qt5.5.0 - 5.11.1
WindowsSDK 8.1, 10


-------------
Info:
-------------
EDataFlow enumeration
https://msdn.microsoft.com/en-us/library/windows/desktop/dd370828(v=vs.85).aspx
[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture] Recording devices

IPolicyConfig undocumented COM interface GUID interface can be chenged and prototype methods as well. 
Check to debug mmsys.cpl (mmsys.pdb)
