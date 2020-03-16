#include"channel.h"

Channel::Channel()
{
}

Channel::Channel(const string &device_id, const string& channel_device_id)
	:device_id_(device_id),
	channel_device_id_(channel_device_id)
{
}

void Channel::save(DBOutStream &out) const
{
	out<<device_id_;
	out<<device_sip_ip_;
	out<<device_sip_port_;
	out<<channel_device_id_;
	out<<channel_name_;
}


void Channel::load(DBInStream &in)
{
	in>>device_id_;
	in>>device_sip_ip_;
	in>>device_sip_port_;
	in>>channel_device_id_;
	in>>channel_name_;
}


