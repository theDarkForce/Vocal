#include <portaudio.h>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <boost/atomic.hpp>
#include <WinSock2.h>
#include <speex/speex.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>

#include "encoded.h"
#include "client.h"

class sound{
public:
	sound();
	~sound();

	void start();
	void stop();

	void th_func();

	boost::signals2::signal<void(char *, int)> sigCapture;
	
	bool setOutputDevice(PaDeviceIndex index);
	bool setInputDevice(PaDeviceIndex index);

	void setsoundsize();

	void setechostate(bool on);

private:
	static int inputcallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo,
							 PaStreamCallbackFlags statusFlags, void *userData);
	static int outputcallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, 
							  PaStreamCallbackFlags statusFlags, void *userData);

private:
	encode _encode;

	int BUFF_LONG;

	struct { int channelcount; int len; char buf[4096]; } inputbuff[16];
	boost::atomic_int begin, end;

	PaStream * input;
	PaStream * output;

	PaStreamParameters inputStreamParameters;
	PaStreamParameters outputStreamParameters;

	PaDeviceIndex indexInputDevice;
	PaDeviceIndex indexOutputDevice;

	SpeexPreprocessState * speexppstate;

	bool isopenecho;
	SpeexEchoState * speexechostate;

};