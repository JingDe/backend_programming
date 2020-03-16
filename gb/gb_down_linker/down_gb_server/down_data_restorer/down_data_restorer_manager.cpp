#include"down_data_restorer_manager.h"
#include"down_data_restorer_redis.h"
#include"down_data_restorer_mysql.h"
#include"down_data_restorer_sqlite.h"

namespace GBGateway {

CDownDataRestorer* CDownDataRestorerManager::AllocateDownDataRestorer(RestorerParam::Type type)
{
	switch(type)
	{
		case RestorerParam::Type::Redis:
			return new CDownDataRestorerRedis();
		case RestorerParam::Type::MySQL:
			return new CDownDataRestorerMysql();
		case RestorerParam::Type::SQLite:
			return new CDownDataRestorerSqlite();
		default:
			return NULL;			
	}
}

void CDownDataRestorerManager::FreeDownDataRestorer(CDownDataRestorer* down_data_restorer)
{
	if(down_data_restorer)
		delete down_data_restorer;
}

} // namespace GBGateway
