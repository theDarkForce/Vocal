#include <speex/speex.h>

class encode{
public:
	encode(int _SPEEX_NB_MODES);
	~encode();

	int encoded(char * inbuf, int framelen, char * outbuf, int outbuflen);
	int decoded(char * inbuf, int framelen, char * outbuf, int outbuflen);

	int getframesize();

private:
	SpeexBits encbits;
	void * encstate;

	SpeexBits decbits;
	void * decstate;

};