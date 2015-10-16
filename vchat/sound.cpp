#include "sound.h"
#include <exception>

#define CLAMP_MIN -32000
#define CLAMP_MAX 32000

sound::sound() : _encode(SPEEX_MODEID_UWB){
	BUFF_LONG = _encode.getframesize();

	isopenecho = false;
	
	begin.store(0);
	end.store(0);
}

sound::~sound(){
}	

bool sound::setOutputDevice(PaDeviceIndex index){
	indexOutputDevice = index;
	return true;
}

bool sound::setInputDevice(PaDeviceIndex index){
	indexInputDevice = index;
	return true;
}

void sound::setechostate(bool on){
	isopenecho = on;
}

int sound::outputcallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData){
	sound * psound = (sound*)userData;
	short * voiceBuffer = (short *)output;
	memset(voiceBuffer, 0, psound->BUFF_LONG * sizeof(short) * psound->outputStreamParameters.channelCount);

	int empty = 0;
	iterator_client_set([&empty, psound, frameCount, &voiceBuffer](std::map<int, client*> & set){
		for (auto var : set){
			char * outbuf = 0;
			short channelcount = 0;
			int len = 0;
			if (!var.second->read_buff(outbuf, channelcount, len)){
				empty++;
				continue;
			}
			short voicebuf[4096];
			memset(voicebuf, 0, 4096 * sizeof(short));
			psound->_encode.decoded(outbuf, len, (char*)voicebuf, psound->BUFF_LONG * psound->outputStreamParameters.channelCount);

			for (unsigned int i = 0; i < frameCount; i++)
			{
				int src = frameCount - i - 1;
				int dst = src *  psound->outputStreamParameters.channelCount;

				if (voiceBuffer[dst] > 0 && voicebuf[src] > CLAMP_MAX - voiceBuffer[dst]){
					voiceBuffer[dst] = CLAMP_MAX;
				} else if (voiceBuffer[i] <= 0 && voicebuf[src] < CLAMP_MIN - voiceBuffer[dst]){
					voiceBuffer[dst] = CLAMP_MIN;
				} else{
					voiceBuffer[dst] += voicebuf[src];
				}
			}

			for (int channel = 1; channel < psound->outputStreamParameters.channelCount; channel++)
			{
				for (unsigned int i = 0; i < frameCount; i++)
				{
					int src = frameCount - i - 1;// (frameCount - i - 1) * psound->outputStreamParameters.channelCount;
					int dst = src + channel;

					if (voiceBuffer[dst] > 0 && voicebuf[src] > CLAMP_MAX - voiceBuffer[dst]){
						voiceBuffer[dst] = CLAMP_MAX;
					} else if (voiceBuffer[i] <= 0 && voicebuf[src] < CLAMP_MIN - voiceBuffer[dst]){
						voiceBuffer[dst] = CLAMP_MIN;
					} else{
						voiceBuffer[dst] += voicebuf[src];
					}
				}
			}
		}
	});
	if (empty == client_count()){
		return paContinue;
	}

	if (psound->isopenecho == true){
		while (1){
			int _begin = psound->begin.load();
			int _new = _begin + 1;

			if (_new == psound->end.load()){
				break;
			}

			if (_new == 16){
				_new = 0;
			}

			if (psound->begin.compare_exchange_strong(_begin, _new)){
				memcpy(psound->inputbuff[_begin].buf, voiceBuffer, frameCount*psound->outputStreamParameters.channelCount);
				psound->inputbuff[_begin].len = frameCount*psound->outputStreamParameters.channelCount;
				break;
			}
		}
	}
	
	return paContinue;
}

int sound::inputcallback(const void *input, void *output, unsigned long frameCount, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData){
	sound * psound = (sound*)userData;

	if (psound->speexppstate){
		speex_preprocess_run(psound->speexppstate, (short *)input);
	}

	short * outputbuff = 0;
	while (1){
		int _end = psound->end.load();
		int _new = _end + 1;

		if (_end == psound->begin.load()){
			break;
		}

		if (_new == 16){
			_new = 0;
		}

		if (psound->end.compare_exchange_strong(_end, _new)){
			outputbuff = (short *)psound->inputbuff[_end].buf;
			break;
		}
	}

	if (psound->isopenecho == true){
		if (outputbuff != 0){
			speex_echo_cancellation(psound->speexechostate, (short*)input, outputbuff, (short*)input);
		}
	}

	char tmp[4096];
	*((short*)tmp) = psound->inputStreamParameters.channelCount;
	int len = psound->_encode.encoded((char*)input, frameCount, tmp+2, 4096);

	psound->sigCapture((char*)tmp, len + 2);

	return paContinue;
}

void sound::start(){
	const PaDeviceInfo * info = Pa_GetDeviceInfo(indexInputDevice);
	inputStreamParameters.device = indexInputDevice;
	inputStreamParameters.channelCount = 1;
	inputStreamParameters.hostApiSpecificStreamInfo = 0;
	inputStreamParameters.sampleFormat = paInt16;
	inputStreamParameters.suggestedLatency = info->defaultLowInputLatency;

	PaError err = Pa_OpenStream(&input, &inputStreamParameters, 0, info->defaultSampleRate, BUFF_LONG, paNoFlag, sound::inputcallback, this);
	if (err != paNoError){
		throw std::exception("open stream err=%d", err);
	}
	err = Pa_StartStream(input);
	if (err != paNoError){
		throw std::exception("start stream err=%d", err);
	}

	speexppstate = speex_preprocess_state_init(BUFF_LONG*inputStreamParameters.channelCount, (int)info->defaultSampleRate);
	spx_int32_t on = 1;
	speex_preprocess_ctl(speexppstate, SPEEX_PREPROCESS_SET_AGC, &on);
	float agcLevel = 24000;
	speex_preprocess_ctl(speexppstate, SPEEX_PREPROCESS_SET_AGC_LEVEL, &agcLevel);    
	int denoise = 1;
	speex_preprocess_ctl(speexppstate, SPEEX_PREPROCESS_SET_DENOISE, &denoise); //½µÔë
	int noiseSuppress = -25;
	speex_preprocess_ctl(speexppstate, SPEEX_PREPROCESS_SET_NOISE_SUPPRESS, &noiseSuppress); //ÉèÖÃÔëÉùµÄdB   

	speexechostate = speex_echo_state_init(BUFF_LONG*inputStreamParameters.channelCount, BUFF_LONG*inputStreamParameters.channelCount);
	
	info = Pa_GetDeviceInfo(indexOutputDevice);
	outputStreamParameters.device = indexOutputDevice;
	outputStreamParameters.channelCount = 1;
	outputStreamParameters.hostApiSpecificStreamInfo = 0;
	outputStreamParameters.sampleFormat = paInt16;
	outputStreamParameters.suggestedLatency = info->defaultLowOutputLatency;

	err = Pa_OpenStream(&output, 0, &outputStreamParameters, info->defaultSampleRate, BUFF_LONG, paNoFlag, sound::outputcallback, this);
	if (err != paNoError){
		throw std::exception("open stream err=%d", err);
	}
	err = Pa_StartStream(output);
	if (err != paNoError){
		throw std::exception("start stream err=%d", err);
	}
}

void sound::stop(){
	Pa_StopStream(input);
	Pa_AbortStream(input);
	Pa_CloseStream(input);
	input = 0;

	Pa_StopStream(output);
	Pa_AbortStream(output);
	Pa_CloseStream(output);
	output = 0;
}
