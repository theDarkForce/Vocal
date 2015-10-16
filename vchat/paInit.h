#include <portaudio.h>

class paInit{
public:
	paInit(){
		Pa_Initialize();
	}

	~paInit(){
		Pa_Terminate();
	}
};