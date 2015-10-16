#include "devices.h"

std::vector<const PaDeviceInfo*>  devices::getInputDevices(){
	std::vector<const PaDeviceInfo*> ret;

	for (int i = 0; i < Pa_GetHostApiCount(); i++){
		const PaHostApiInfo * info = Pa_GetHostApiInfo(i);
		ret.push_back(Pa_GetDeviceInfo(info->defaultInputDevice));
	}
	
	return ret;
}

std::vector<const PaDeviceInfo*>  devices::getOutputDevices(){
	std::vector<const PaDeviceInfo*> ret;

	for (int i = 0; i < Pa_GetHostApiCount(); i++){
		const PaHostApiInfo * info = Pa_GetHostApiInfo(i);
		ret.push_back(Pa_GetDeviceInfo(info->defaultOutputDevice));
	}

	return ret;
}