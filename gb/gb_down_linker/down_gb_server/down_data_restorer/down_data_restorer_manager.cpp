#include"down_data_restorer_manager.h"
#include"down_data_restorer_redis.h"
#include"down_data_restorer_mysql.h"
#include"down_data_restorer_sqlite.h"

namespace GBGateway {

CDownDataRestorer* CDownDataRestorerManager::AllocateDownDataRestorer(RestorerParam::Type type)
{
	//type_=type;
	switch(type)
	{
		case RestorerParam::Type::Redis:
			return new CDownDataRestorerRedis();
		case RestorerParam::Type::MySQL:
			return new CDownDataRestorerMySql();
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
//	if(down_data_restorer==NULL)
//		return;
//	switch(type_)
//	{
//		case RestorerParam::Type::Redis:
//			delete static_cast<CDownDataRestorerRedis*>(down_data_restorer);
//		case RestorerParam::Type::MySQL:
//			delete static_cast<CDownDataRestorerMySql*>(down_data_restorer);
//		case RestorerParam::Type::SQLite:
//			delete static_cast<CDownDataRestorerSqlite*>(down_data_restorer);
//		default:
//			return;
//	}	
}

} // namespace GBGateway
