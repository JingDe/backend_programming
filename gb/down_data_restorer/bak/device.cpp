#include"device.h"

Device::Device()
	:device_id_(),
	device_sip_ip_(),
	device_sip_port_(),
	device_state_()
{}

Device::Device(const string& device_id)
	:device_id_(device_id),
	device_sip_ip_(),
	device_sip_port_(),
	device_state_()
{}


void Device::save(DBOutStream &out) const
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

void Device::UpdateRegisterLastTime(time_t t)
{}

void Device::UpdateKeepaliveLastTime(time_t t)
{}
