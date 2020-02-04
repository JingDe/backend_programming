#include"channel.h"

void Channel::save(DBOutStream &out)
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


