#ifndef DOWN_DATA_RESTORER_EXECUTING_INVITE_CMD_H
#define DOWN_DATA_RESTORER_EXECUTING_INVITE_CMD_H

#include"dbstream.h"
#include<string>

using std::string;

class ExecutingInviteCmd : public DBSerialize {
public:

	string GetId() const;
    void SetSipRestared(){
        sip_restarted_=true;
    }	
	void save(DBOutStream &out);
    void load(DBInStream &in);

private:
	string cmd_sender_id_;
	string device_id_;
	int cmd_type;
	int cmd_seq_;
	bool sip_restarted_;
	// TODO

};

#endif

