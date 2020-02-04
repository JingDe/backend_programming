#include"executinginvitecmd.h"
#include"downdatarestorercommon.h"

string ExecutingInviteCmd::GetId() const
{
	return ToString(cmd_seq_);
}

void ExecutingInviteCmd::save(DBOutStream &out)
{
	out<<cmd_sender_id_
		<<device_id_
		<<cmd_type
		<<cmd_seq_;
}

void ExecutingInviteCmd::load(DBInStream &in)
{
	in>>cmd_sender_id_
		>>device_id_
		>>cmd_type
		>>cmd_seq_;
}
    

