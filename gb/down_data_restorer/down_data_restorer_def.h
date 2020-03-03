#ifndef DOWN_DATA_RESTORER_DEF_H
#define DOWN_DATA_RESTORER_DEF_H

#include<list>
#include<memory>
using std::list;

namespace GBGateway {

class ExecutingInviteCmd;

typedef list<ExecutingInviteCmd> ExecutingInviteCmdList;

typedef std::unique_ptr<ExecutingInviteCmd> ExecutingInviteCmdPtr;

typedef list<ExecutingInviteCmdPtr> ExecutingInviteCmdPtrList;

} // namespace GBGateway

#endif
