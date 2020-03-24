#include "down_data_restorer_manager.h"
#include "down_data_restorer_redis.h"
#include "down_data_restorer_mysql.h"
#include "down_data_restorer_sqlite.h"

#include <new>

namespace GBDownLinker {

CDownDataRestorer* CDownDataRestorerManager::AllocateDownDataRestorer(RestorerParam::Type type)
{
	switch (type)
	{
	case RestorerParam::Type::Redis:
		return new(std::nothrow) CDownDataRestorerRedis();
	case RestorerParam::Type::MySQL:
		return new(std::nothrow) CDownDataRestorerMySql();
	case RestorerParam::Type::SQLite:
		return new(std::nothrow) CDownDataRestorerSqlite();
	default:
		return NULL;
	}
}

void CDownDataRestorerManager::FreeDownDataRestorer(CDownDataRestorer* down_data_restorer)
{
	if (down_data_restorer)
	{
		delete down_data_restorer;
	}
}

} // namespace GBDownLinker
