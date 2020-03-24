#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_DEF_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_DEF_H

#include <string>
#include <list>
#include <memory>
#include <functional>
#include <variant>

namespace GBDownLinker {

#define RESTORER_OK         0   
#define RESTORER_FAIL       1

class Device;
class Channel;
class ExecutingInviteCmd;

typedef std::unique_ptr<Device> DevicePtr;
typedef std::unique_ptr<Channel> ChannelPtr;
typedef std::unique_ptr<ExecutingInviteCmd> ExecutingInviteCmdPtr;


typedef std::list<ExecutingInviteCmd> ExecutingInviteCmdList;
typedef std::list<ExecutingInviteCmdPtr> ExecutingInviteCmdPtrList;



typedef struct RestorerParam
{
	enum class Type : uint8_t
	{
		Redis,
		MySQL,
		SQLite,
	};

	typedef struct RedisConfig
	{
		typedef struct Url
		{
			std::string ip;
			unsigned int port = 0;
		} Url;

		using UrlPtr = std::unique_ptr<Url>;

		std::list<UrlPtr> urlList;
		std::string masterName;

	} RedisConfig;

	typedef struct MySQLConfig
	{
		//TODO
	} MySQLConfig;

	typedef struct SQLiteConfig
	{
		//TODO
	} SQLiteConfig;

	using Config = std::variant<
		RedisConfig,
		MySQLConfig,
		SQLiteConfig>;

	Type type;
	Config config;

} RestorerParam;

using RestorerParamPtr = std::unique_ptr<RestorerParam>;

//typedef void (*ErrorReportCallback)();
//using ErrorReportCallback = std::function<void()>;

} // namespace GBDownLinker

#endif
