#ifndef DOWN_DATA_RESTORER_CHANNEL_H
#define DOWN_DATA_RESTORER_CHANNEL_H

#include"dbstream.h"
#include<string>

using std::string;

class Channel : public DBSerialize {
public:

	string GetId() const {
		return device_id_+channel_device_id_;
	}

	void save(DBOutStream &out);
    void load(DBInStream &in);
    
private:

	string channel_id; // device_id_+channel_device_id_
	string device_id_;
	string device_sip_ip_;
	string device_sip_port_;
	string channel_device_id_;
	string channel_name_;
	// TODO
};

#endif
