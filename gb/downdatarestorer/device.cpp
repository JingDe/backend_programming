#include"device.cpp"

void Device::save(DBOutStream &out)
{
	out<<device_id_;
	out<<device_sip_ip_;
	out<<device_sip_port_;
	out<<device_state_;
}

void Device::load(DBInStream &in)
{
	in>>device_id_;
	in>>device_sip_ip_;
	in>>device_sip_port_;
	in>>device_state_;
}

