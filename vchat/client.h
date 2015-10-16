#include <portaudio.h>
#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include <boost/atomic.hpp>
#include <WinSock2.h>
#include <speex/speex.h>
#include <speex/speex_preprocess.h>
#include <speex/speex_echo.h>
#include <functional>

struct client{
	client(){
		begin.store(0);
		end.store(0);
	}

	sockaddr_in addr;
	int index;
	int BUFF_LONG;

	struct { int channelcount; int len; char buf[4096]; } inputbuff[16];
	boost::atomic_int begin, end;

	bool read_buff(char * & outputbuff, short & channelcount, int &len){
		while (1){
			int _end = end.load();
			int _new = _end + 1;

			if (_end == begin.load()){
				return false;
			}

			if (_new == 16){
				_new = 0;
			}

			if (end.compare_exchange_strong(_end, _new)){
				outputbuff = inputbuff[_end].buf;
				channelcount = inputbuff[_end].channelcount;
				len = inputbuff[_end].len;
				return true;
			}
		}

		return true;
	}

	void write_buff(char * buff, int buflen, short channelcount){
		while (1){
			int _begin = begin.load();
			int _new = _begin + 1;

			if (_new == end.load()){
				break;
			}

			if (_new == 16){
				_new = 0;
			}

			if (begin.compare_exchange_strong(_begin, _new)){
				memcpy(inputbuff[_begin].buf, buff, buflen);
				inputbuff[_begin].channelcount = channelcount;
				inputbuff[_begin].len = buflen;
				break;
			}
		}
	}

};

client * create_client(int index = 0);

client * get_client(int index);

typedef void(*handle_iterator_client)(std::map<int, client*> & set);
void iterator_client_set(handle_iterator_client fn);
void iterator_client_set(std::function<void(std::map<int, client*> &) > fn);

bool destroy_client(int index);

int client_count();