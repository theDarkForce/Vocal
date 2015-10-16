#include "encoded.h"
#include <stdio.h>

encode::encode(int _SPEEX_NB_MODES){
	speex_bits_init(&encbits);
	encstate = speex_encoder_init(speex_mode_list[_SPEEX_NB_MODES]);
	float quality = 10.0;
	speex_encoder_ctl(encstate, SPEEX_SET_QUALITY, &quality);
	//int enh = 1;
	//speex_encoder_ctl(encstate, SPEEX_SET_ENH, &enh);
	int dtx = 1;
	speex_encoder_ctl(encstate, SPEEX_SET_DTX, &dtx);
	int vad = 1;
	speex_encoder_ctl(encstate, SPEEX_SET_VAD, &vad);
	//int highpass = 1;
	//speex_encoder_ctl(encstate, SPEEX_SET_HIGHPASS, &highpass);

	speex_bits_init(&decbits);
	decstate = speex_decoder_init(speex_mode_list[_SPEEX_NB_MODES]);
}

encode::~encode(){
	speex_bits_destroy(&encbits);
	speex_encoder_destroy(encstate);

	speex_bits_destroy(&decbits);
	speex_decoder_destroy(decstate);
}

int encode::encoded(char * inbuf, int framelen, char * outbuf, int outbuflen){
	speex_bits_reset(&encbits);
	speex_encode_int(encstate, (short*)inbuf, &encbits);
	return speex_bits_write(&encbits, outbuf, outbuflen);
}

int encode::decoded(char * inbuf, int framelen, char * outbuf, int outbuflen){
	speex_bits_read_from(&decbits, inbuf, framelen);
	return speex_decode_int(decstate, &decbits, (short*)outbuf);
}

int encode::getframesize(){
	int frame_size = 0;
	speex_encoder_ctl(encstate, SPEEX_GET_FRAME_SIZE, &frame_size);
	
	return frame_size;
}