#include"executinginvitecmdmgr.h"
#include"redisclient.h"
#include"downdatarestorercommon.h"
#include"mutexlock.h"
#include"mutexlockguard.h"
#include"glog/logging.h"

const string ExecutingInviteCmdMgr::s_set_key_prefix="gbddrexecutinginvitecmdset";
const string ExecutingInviteCmdMgr::s_key_prefix="gbddrexecutinginvitecmdset";

ExecutingInviteCmdMgr::ExecutingInviteCmdMgr(RedisClient* redis_client, RedisMode redis_mode, int worker_thread_num)
	:redis_client_(redis_client),
	redis_mode_(redis_mode),
	worker_thread_num_(worker_thread_num),
	rwmutexes_(NULL)
//	modify_mutex_list_(NULL)
{
//	modify_mutex_list_=new MutexLock[worker_thread_num];
	rwmutexes_=new RWMutex[worker_thread_num];
	LOG(INFO)<<"ExecutingInviteCmdMgr constructed";
}

ExecutingInviteCmdMgr::~ExecutingInviteCmdMgr()
{
//	if(modify_mutex_list_)
//		delete[] modify_mutex_list_;
	if(rwmutexes_)
		delete[] rwmutexes_;
}

int ExecutingInviteCmdMgr::Load(vector<ExecutingInviteCmdList>& cmd_lists)
{
	cmd_lists.resize(worker_thread_num_);
	for(int i=0; i<worker_thread_num_; i++)
	{
		Load(i, cmd_lists[i]);
	}
	return 0;
}

int ExecutingInviteCmdMgr::Load(int worker_thread_no, ExecutingInviteCmdList& cmd_list)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
	ReadGuard guard(rwmutexes_[worker_thread_no]);
	
	list<string> cmd_id_list;
	redis_client_->smembers(s_set_key_prefix+ToString(worker_thread_no), cmd_id_list);
	for(list<string>::iterator it=cmd_id_list.begin(); it!=cmd_id_list.end(); it++)
	{
		ExecutingInviteCmd cmd;
		if(redis_client_->getSerial(s_key_prefix+ToString(worker_thread_no)+*it, cmd))
		{
			cmd.SetSipRestarted(true);
			cmd_list.push_back(cmd);
		}
	}

	return 0;
}

//int ExecutingInviteCmdMgr::Search(const string& executing_invite_cmd_id, ExecutingInviteCmd& cmd)
//{
//	for(int i=0; i<worker_thread_num_; ++i)
//	{
//		ReadGuard guard(rwmutexes_[i]);
//		bool exist=redis_client_->sismember(s_set_key_prefix+ToString(i), executing_invite_cmd_id);
//		if(exist)
//		{
//			if(redis_client_->getSerial(s_key_prefix+ToString(i)+executing_invite_cmd_id, cmd))
//			{
//				return 0;
//			}
//		}	
//	}
//	return -1;
//}

int ExecutingInviteCmdMgr::Search(const string& executing_invite_cmd_id, ExecutingInviteCmd& cmd, int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
	ReadGuard guard(rwmutexes_[worker_thread_no]);
	bool exist=redis_client_->sismember(s_set_key_prefix+ToString(worker_thread_no), executing_invite_cmd_id);
	if(exist)
	{
		if(redis_client_->getSerial(s_key_prefix+ToString(worker_thread_no)+executing_invite_cmd_id, cmd))
		{
			return 0;
		}
	}	

	return -1;
}

int ExecutingInviteCmdMgr::Insert(const ExecutingInviteCmd& cmd, int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;

	if(redis_mode_==STAND_ALONE_OR_PROXY_MODE)
		return InsertInTransaction(cmd, worker_thread_no);
	else //if(redis_mode_==CLUSTER_MODE)
		return InsertInCluster(cmd, worker_thread_no);
}


int ExecutingInviteCmdMgr::InsertInCluster(const ExecutingInviteCmd& cmd, int worker_thread_no)
{
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);

	if(redis_client_->setSerial(s_key_prefix+ToString(worker_thread_no)+cmd.GetId(), cmd)==false)
	{
		return -1;
	}
	
	if(redis_client_->sadd(s_set_key_prefix+ToString(worker_thread_no), cmd.GetId())==true)
	{
		return 0;
	}
	else
	{
		if(redis_client_->del(s_key_prefix+cmd.GetId())==false)
		{
			LOG(ERROR)<<"insert cmd"<<cmd.GetId()<<" failed, undo set failed";
		}
		return -1;
	}
}


int ExecutingInviteCmdMgr::InsertInTransaction(const ExecutingInviteCmd& cmd, int worker_thread_no)
{
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);
	
	RedisConnection *con=NULL;
    if(!redis_client_->PrepareTransaction(&con))
    {
    	LOG(ERROR)<<"PrepareTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
	    LOG(ERROR)<<"StartTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->Set(con, s_key_prefix+ToString(worker_thread_no)+cmd.GetId(), cmd))
    {
	    LOG(ERROR)<<"Set failed";
    	goto FAIL;
    }
    if(!redis_client_->Sadd(con, s_set_key_prefix+ToString(worker_thread_no), cmd.GetId()))
    {
	    LOG(ERROR)<<"Sadd failed";
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
	    LOG(ERROR)<<"ExecTransaction failed";
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
    LOG(INFO)<<"Insert cmd success: "<<cmd.GetId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
    LOG(ERROR)<<"Insert cmd failed: "<<cmd.GetId();
    return -1;
}

int ExecutingInviteCmdMgr::Delete(const ExecutingInviteCmd& cmd, int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
	if(redis_mode_==STAND_ALONE_OR_PROXY_MODE)
		return DeleteInTransaction(cmd, worker_thread_no);
	else //if(redis_mode_==CLUSTER_MODE)
		return DeleteInCluster(cmd, worker_thread_no);
}

int ExecutingInviteCmdMgr::DeleteInCluster(const ExecutingInviteCmd& cmd, int worker_thread_no)
{
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);

	if(redis_client_->del(s_key_prefix+ToString(worker_thread_no)+cmd.GetId())==false)
		return -1;
	
	if(redis_client_->srem(s_set_key_prefix+ToString(worker_thread_no), cmd.GetId())==true)
	{
		LOG(INFO)<<"delete cmd "<<cmd.GetId()<<" ok";
		return 0;
	}
	else
	{
		if(redis_client_->setSerial(s_key_prefix+cmd.GetId(), cmd)==false)
		{
			LOG(ERROR)<<"delete cmd"<<cmd.GetId()<<" failed, undo del failed";
		}
		return -1;
	}
}

int ExecutingInviteCmdMgr::DeleteInTransaction(const ExecutingInviteCmd& cmd, int worker_thread_no)
{
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);
	
	RedisConnection *con=NULL;
    if(!redis_client_->PrepareTransaction(&con))
    {
    	LOG(ERROR)<<"PrepareTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->StartTransaction(con))
    {
	    LOG(ERROR)<<"StartTransaction failed";
    	goto FAIL;
    }
    if(!redis_client_->Del(con, s_key_prefix+ToString(worker_thread_no)+cmd.GetId()))
    {
	    LOG(ERROR)<<"Del failed";
    	goto FAIL;
    }
    if(!redis_client_->Srem(con, s_set_key_prefix+ToString(worker_thread_no), cmd.GetId()))
    {
	    LOG(ERROR)<<"Srem failed";
    	goto FAIL;
    }
    if(!redis_client_->ExecTransaction(con))
    {
	    LOG(ERROR)<<"ExecTransaction failed";
    	goto FAIL;
    }
            
    redis_client_->FinishTransaction(&con);
    LOG(INFO)<<"Delete cmd success: "<<cmd.GetId();
	return 0;

FAIL:
    if(con)
    {                                                                            
        redis_client_->FinishTransaction(&con);
    }
    LOG(ERROR)<<"Delete cmd failed: "<<cmd.GetId();
    return -1;
}

int ExecutingInviteCmdMgr::Clear()
{	
	for(int i=0; i<worker_thread_num_; ++i)
	{
		Clear(i);
	}
    return 0;
}

int ExecutingInviteCmdMgr::Clear(int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);
	
	list<string> cmd_id_list;
	redis_client_->smembers(s_set_key_prefix+ToString(worker_thread_no), cmd_id_list);
	redis_client_->del(s_set_key_prefix+ToString(worker_thread_no));
	for(list<string>::iterator it=cmd_id_list.begin(); it!=cmd_id_list.end(); it++)
	{
		redis_client_->del(s_key_prefix+ToString(worker_thread_no)+*it);
	}

    return 0;
}

int ExecutingInviteCmdMgr::ClearWithLockHeld(int worker_thread_no)
{	
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
	list<string> cmd_id_list;
	redis_client_->smembers(s_set_key_prefix+ToString(worker_thread_no), cmd_id_list);
	redis_client_->del(s_set_key_prefix+ToString(worker_thread_no));
	for(list<string>::iterator it=cmd_id_list.begin(); it!=cmd_id_list.end(); it++)
	{
		redis_client_->del(s_key_prefix+ToString(worker_thread_no)+*it);
	}

    return 0;
}

int ExecutingInviteCmdMgr::Update(const vector<ExecutingInviteCmdList>& cmd_lists)
{
	if((int)cmd_lists.size() != worker_thread_num_)
	{
		LOG(ERROR)<<"update cmdlists failed";
		return -1;
	}

	for(size_t i=0; i<cmd_lists.size(); ++i)
	{
		Update(cmd_lists[i], i);
	}
    return 0;
}

int ExecutingInviteCmdMgr::Update(const ExecutingInviteCmdList& cmd_list, int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);

	ClearWithLockHeld(worker_thread_no);
	
	for(ExecutingInviteCmdList::const_iterator it=cmd_list.begin(); it!=cmd_list.end(); ++it)
	{		
		redis_client_->sadd(s_set_key_prefix+ToString(worker_thread_no), it->GetId());
		redis_client_->setSerial(s_key_prefix+ToString(worker_thread_no)+it->GetId(), *it);
	}
    return 0;
}

int ExecutingInviteCmdMgr::Update(const list<void*>& cmd_list, int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
//	MutexLockGuard guard(&modify_mutex_list_[worker_thread_no]);
	WriteGuard guard(rwmutexes_[worker_thread_no]);
	
	ClearWithLockHeld(worker_thread_no);

	for(list<void*>::const_iterator it=cmd_list.begin(); it!=cmd_list.end(); ++it)
	{
		ExecutingInviteCmd* cmd=(ExecutingInviteCmd*)(*it);
		redis_client_->sadd(s_set_key_prefix+ToString(worker_thread_no), cmd->GetId());
		redis_client_->setSerial(s_key_prefix+ToString(worker_thread_no)+cmd->GetId(), *cmd);
	}
    return 0;
}

int ExecutingInviteCmdMgr::GetExecutingInviteCmdListSize(int worker_thread_no)
{
	if(worker_thread_no<0  ||  worker_thread_no>=worker_thread_num_)
		return -1;
	ReadGuard guard(rwmutexes_[worker_thread_no]);
	return redis_client_->scard(s_set_key_prefix+ToString(worker_thread_no));
}

