#ifndef DOWN_DATA_RESTORER_SERIALIZE_ENTITY_H
#define DOWN_DATA_RESTORER_SERIALIZE_ENTITY_H

#include "dbstream.h"
#include "channel.h"
#include "device.h"
#include "executing_invite_cmd.h"
#include "redis_client/serialize.h"

namespace GBDownLinker {

void savetime(const TimePtr& timeptr, DBOutStream& out);
void loadtime(DBInStream& in, TimePtr& timeptr);
void savechannellist(const ChannelListPtr& channellist, DBOutStream& out);
void loadchannellist(DBInStream& in, ChannelListPtr& channellist);


template<>
void save(const Channel& channel, DBOutStream& out);

template<>
void load(DBInStream& in, Channel& channel);

template<>
void save(const Device& device, DBOutStream& out);

template<>
void load(DBInStream& in, Device& device);

template<>
void save(const ExecutingInviteCmd& cmd, DBOutStream& out);

template<>
void load(DBInStream& in, ExecutingInviteCmd& cmd);


} // namespace GBDownLinker

#endif
