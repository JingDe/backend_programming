#ifndef DOWN_DATA_RESTORER_DEVICE_H
#define DOWN_DATA_RESTORER_DEVICE_H


#include"dbstream.h"
#include<string>

using std::string;

class Device : public DBSerialize {
public:
	Device();
	string GetDeviceId() const { return device_id_; }
    void UpdateRegisterLastTime(time_t t);
    void UpdateKeepaliveLastTime(time_t t);
	
	void save(DBOutStream &out);
    void load(DBInStream &in);
    
private:
	string device_id_;
	string device_sip_ip_;
	uint16_t device_sip_port_;
	int device_state_;
	// TODO

};

#endif
