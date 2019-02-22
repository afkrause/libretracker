#pragma once

/*
MIT License

Copyright(c) 2018 David Gil de Gómez Pérez(Studiosi)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Windows.h>
#include <dshow.h>

#pragma comment(lib, "strmiids")

#include <map>
#include <string>


class DeviceEnumerator
{

public:
	struct Device
	{
		int id; // This can be used to open the device in OpenCV
		std::string devicePath;
		std::string deviceName; // This can be used to show the devices to the user
	};

	DeviceEnumerator() = default;

	// Returns a map of id and devices that can be used
	std::map<int, Device> getDevicesMap(const GUID deviceClass)
	{
		std::map<int, Device> deviceMap;

		HRESULT hr = CoInitialize(nullptr);
		if (FAILED(hr)) {
			return deviceMap; // Empty deviceMap as an error
		}

		// Create the System Device Enumerator
		ICreateDevEnum *pDevEnum;
		hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnum));

		// If succeeded, create an enumerator for the category
		IEnumMoniker *pEnum = NULL;
		if (SUCCEEDED(hr)) {
			hr = pDevEnum->CreateClassEnumerator(deviceClass, &pEnum, 0);
			if (hr == S_FALSE) {
				hr = VFW_E_NOT_FOUND;
			}
			pDevEnum->Release();
		}

		// Now we check if the enumerator creation succeeded
		int deviceId = -1;
		if (SUCCEEDED(hr)) {
			// Fill the map with id and friendly device name
			IMoniker *pMoniker = NULL;
			while (pEnum->Next(1, &pMoniker, NULL) == S_OK) {

				IPropertyBag *pPropBag;
				HRESULT hr = pMoniker->BindToStorage(0, 0, IID_PPV_ARGS(&pPropBag));
				if (FAILED(hr)) {
					pMoniker->Release();
					continue;
				}

				// Create variant to hold data
				VARIANT var;
				VariantInit(&var);

				std::string deviceName;
				std::string devicePath;

				// Read FriendlyName or Description
				hr = pPropBag->Read(L"Description", &var, 0); // Read description
				if (FAILED(hr)) {
					// If description fails, try with the friendly name
					hr = pPropBag->Read(L"FriendlyName", &var, 0);
				}
				// If still fails, continue with next device
				if (FAILED(hr)) {
					VariantClear(&var);
					continue;
				}
				// Convert to string
				else {
					deviceName = ConvertBSTRToMBS(var.bstrVal);
				}

				VariantClear(&var); // We clean the variable in order to read the next value

									// We try to read the DevicePath
				hr = pPropBag->Read(L"DevicePath", &var, 0);
				if (FAILED(hr)) {
					VariantClear(&var);
					continue; // If it fails we continue with next device
				}
				else {
					devicePath = ConvertBSTRToMBS(var.bstrVal);
				}

				// We populate the map
				deviceId++;
				Device currentDevice;
				currentDevice.id = deviceId;
				currentDevice.deviceName = deviceName;
				currentDevice.devicePath = devicePath;
				deviceMap[deviceId] = currentDevice;

			}
			pEnum->Release();
		}
		CoUninitialize();
		return deviceMap;
	}


	std::map<int, Device> DeviceEnumerator::getVideoDevicesMap()
	{
		return getDevicesMap(CLSID_VideoInputDeviceCategory);
	}

	std::map<int, Device> DeviceEnumerator::getAudioDevicesMap()
	{
		return getDevicesMap(CLSID_AudioInputDeviceCategory);
	}


private:




	/*
	This two methods were taken from
	https://stackoverflow.com/questions/6284524/bstr-to-stdstring-stdwstring-and-vice-versa
	*/

	std::string DeviceEnumerator::ConvertBSTRToMBS(BSTR bstr)
	{
		int wslen = ::SysStringLen(bstr);
		return ConvertWCSToMBS((wchar_t*)bstr, wslen);
	}

	std::string DeviceEnumerator::ConvertWCSToMBS(const wchar_t* pstr, long wslen)
	{
		int len = ::WideCharToMultiByte(CP_ACP, 0, pstr, wslen, NULL, 0, NULL, NULL);

		std::string dblstr(len, '\0');
		len = ::WideCharToMultiByte(CP_ACP, 0 /* no flags */,
			pstr, wslen /* not necessary NULL-terminated */,
			&dblstr[0], len,
			NULL, NULL /* no default char */);

		return dblstr;
	}




};
