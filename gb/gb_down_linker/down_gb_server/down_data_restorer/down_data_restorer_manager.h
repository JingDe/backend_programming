#ifndef GB_DOWNLINKER_DOWN_DATA_RESTORER_MANAGER_H
#define GB_DOWNLINKER_DOWN_DATA_RESTORER_MANAGER_H

#include"down_data_restorer_def.h"

namespace GBGateway {

class CDownDataRestorer;

class CDownDataRestorerManager {
public:
	static CDownDataRestorer* AllocateDownDataRestorer(RestorerParam::Type type);
	static void FreeDownDataRestorer(CDownDataRestorer* down_data_restorer);

};

} // namespace GBGateway

#endif
