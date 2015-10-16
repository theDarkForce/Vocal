#include "vchat.h"
#include <boost/thread.hpp>
#include <boost/atomic.hpp>
#include <iostream>
#include <WinSock2.h>

#include <packon.h>
#pragma comment(lib, "ws2_32.lib")
#include <packoff.h>

SOCKET s;
sockaddr_in raddr;

void senddata(char * buf, int len){
	char sendbuf[8192];
	memset(sendbuf, 0, 8192);
	*((int*)sendbuf) = len;
	memcpy(sendbuf + 4, buf, len);
	sendto(s, sendbuf, len + 4, 0, (sockaddr*)&raddr, sizeof(sockaddr_in));
}

int main(){
	WSAData data;
	WSAStartup(2, &data);

	paInit init;

	sound _sound;
	
	_sound.setInputDevice(Pa_GetDefaultInputDevice());
	_sound.setOutputDevice(Pa_GetDefaultOutputDevice());

	s = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in laddr;
	laddr.sin_family = AF_INET;
	laddr.sin_port = 0;
	laddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (bind(s, (sockaddr*)&laddr, sizeof(laddr)) != 0){
		printf("bind error\n");
		return -1;
	}
		
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(7777);
	raddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");

	_sound.sigCapture.connect(senddata);

	_sound.start();

	char buff[65536];
	while (1){
		sockaddr_in faddr;
		int flen = sizeof(sockaddr_in);
		int len = recvfrom(s, buff, 65536, 0, (sockaddr*)&faddr, &flen);

		if (len <= 0){
			int error = WSAGetLastError();
			closesocket(s);
			break;
		}

		int * pbuflen = ((int*)buff);
		int buflen = *pbuflen++;
		short * pchannelcount = (short*)pbuflen;
		short channelcount = *pchannelcount++;
		char * voicebuf = (char*)pchannelcount;
		int index = buff[buflen + 4];

		client  * _data = get_client(index);
		if (_data == 0){
			_data = create_client(index);
		}

		_data->write_buff(voicebuf, buflen - 2, channelcount);
	}

	WSACleanup();

	return 1;
}