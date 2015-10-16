#include <portaudio.h>
#include <vector>

class devices{
public:
	static std::vector<const PaDeviceInfo*>  getInputDevices();
	static std::vector<const PaDeviceInfo*>  getOutputDevices();

};