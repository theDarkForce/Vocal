#include "client.h"

static std::map<int, client*> vclient;
static boost::mutex _mu;

client * create_client(int index){
	client * _data = 0;

	std::map<int, client*>::iterator find = vclient.find(index);
	if (find == vclient.end()){
		_data = new client;
		if (index == 0){
			if (!vclient.empty()){
				std::map<int, client*>::reverse_iterator end = vclient.rbegin();
				_data->index = end->second->index + 1;
			} else{
				_data->index = 1;
			}
		} else{
			_data->index = index;
		}
		vclient.insert(std::make_pair(index, _data));
	} else{
		_data = find->second;
	}

	return _data;
}

client * get_client(int index){
	std::map<int, client*>::iterator find = vclient.find(index);
	if (find == vclient.end()){
		return 0;
	}

	return find->second;
}

void iterator_client_set(handle_iterator_client fn){
	boost::mutex::scoped_lock l(_mu);
	fn(vclient);
}

void iterator_client_set(std::function<void(std::map<int, client*> &) > fn){
	boost::mutex::scoped_lock l(_mu);
	fn(vclient);
}

bool destroy_client(int index){
	std::map<int, client*>::iterator find = vclient.find(index);
	if (find == vclient.end()){
		return false;
	}
	delete find->second;
	vclient.erase(find);

	return true;
}

int client_count(){
	return vclient.size();
}