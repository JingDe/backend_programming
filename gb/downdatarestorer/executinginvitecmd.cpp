#include"executinginvitecmd.h"
#include"downdatarestorercommon.h"

ExecutingInviteCmd::ExecutingInviteCmd(int cmd_seq)
	:cmd_seq_(cmd_seq)
{
	
}

string ExecutingInviteCmd::GetId() const
{
	return ToString(cmd_seq_);
}

void ExecutingInviteCmd::save(DBOutStream &out) const
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
    

