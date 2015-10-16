#include <boost/thread.hpp>
#include <vector>
#include <WinSock2.h>

#include <packon.h>
#pragma comment(lib, "ws2_32.lib")
#include <packoff.h>

SOCKET s_accept;
bool brun = true;

std::vector<sockaddr_in> vsession;
boost::mutex vsession_mu;

void do_recv(){
	char buff[65536];
	while (brun){
	_break:
		sockaddr_in raddr;
		memset(&raddr, 0, sizeof(sockaddr_in));
		int flen = sizeof(sockaddr);
		int len = recvfrom(s_accept, buff, 65536, 0, (sockaddr*)&raddr, &flen);

		boost::mutex::scoped_lock lock(vsession_mu);

		if (len > 0)
		{
			for (unsigned int i = 0; i < vsession.size(); i++){
				if (vsession[i].sin_addr.S_un.S_addr != raddr.sin_addr.S_un.S_addr || vsession[i].sin_port != raddr.sin_port){
					int len = *((int*)buff);
					buff[len + 4] = i+1;
					sendto(s_accept, buff, len + 4 + 1, 0, (sockaddr*)&vsession[i], sizeof(sockaddr_in));
				}
			}

			for (auto se : vsession){
				if (se.sin_addr.S_un.S_addr == raddr.sin_addr.S_un.S_addr && se.sin_port == raddr.sin_port){
					goto _break;
				}
			}
			vsession.push_back(raddr);
		}

		if (len == 0){
			for (std::vector<sockaddr_in>::iterator iter = vsession.begin(); iter != vsession.end(); iter++){
				if (iter->sin_addr.S_un.S_addr == raddr.sin_addr.S_un.S_addr && iter->sin_port == raddr.sin_port){
					vsession.erase(iter);
					break;
				}
			}
		}
	}
}

int main(){
	WSAData data;
	WSAStartup(2, &data);

	boost::thread_group _group;
	
	s_accept = socket(AF_INET, SOCK_DGRAM, 0);

	sockaddr_in laddr;
	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(7777);
	laddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	if (bind(s_accept, (sockaddr*)&laddr, sizeof(laddr)) != 0){
		printf("bind error=%d\n", WSAGetLastError());
		return -1;
	}

	_group.create_thread(do_recv);

	_group.join_all();

	WSACleanup();

	return 0;
}
