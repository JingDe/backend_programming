#include"serialize_entity.h"

namespace GBGateway {


template<>
void save(const SerializableType& entity, DBOutStream& out)
{}

template<>
void load(DBInStream& in, SerializableType& entity)
{}


} // namespace GBGateway

