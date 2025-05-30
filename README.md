This Qt App can control settings of StereoMix in Windows OS: enable/disable device, change volume, playback device and power management (mmsys.cpl).  
Used COM and WASAPI without third party libs.

**COM Interfaces:** `IMMDevice, IMMDeviceEnumerator, IMMDeviceCollection, IAudioEndpointVolume, IPropertyStore, IPolicyConfig`.

> [!NOTE]
> EDataFlow enumeration:  
> https://learn.microsoft.com/en-us/windows/win32/api/mmdeviceapi/ne-mmdeviceapi-edataflow
> 
> Recording devices (mmsys.cpl):  
> `[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture]`   

> [!IMPORTANT]
> The COM interface: `IPolicyConfig` is undocumented and the `GUID` can be changed in Windows OS.  
> For debugging, download symbols mmsys.pdb.
