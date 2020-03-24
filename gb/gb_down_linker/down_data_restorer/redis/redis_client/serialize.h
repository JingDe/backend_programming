// specialize load/save template functions for any type you want to store in Redis

#ifndef REDIS_CLIENT_SERIALIZE_ENTITY_H
#define REDIS_CLIENT_SERIALIZE_ENTITY_H

#include "dbstream.h"

namespace GBDownLinker {

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


} // namespace GBDownLinker

#endif
