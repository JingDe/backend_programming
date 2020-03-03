#ifndef DOWN_DATA_RESTORER_SERIALIZE_ENTITY_H
#define DOWN_DATA_RESTORER_SERIALIZE_ENTITY_H

#include"dbstream.h"
#include"channel.h"
#include"device.h"
#include"executing_invite_cmd.h"

namespace GBGateway {

typedef int SerializableType;

template<typename T> 
std::ostream& operator<<(typename std::enable_if<std::is_enum<T>::value, std::ostream>::type& stream, const T& e) 
{
    return stream << static_cast<typename std::underlying_type<T>::type>(e);
}

template<typename T>
void save(const T& entity, DBOutStream& out);

template<typename T>
void load(DBInStream& in, T& entity);

template<>
void save(const SerializableType& entity, DBOutStream& out);

template<>
void load(DBInStream& in, SerializableType& entity);

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
 

} // namespace GBGateway

#endif
