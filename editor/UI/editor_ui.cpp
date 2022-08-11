#include "ecs_tapi.h"
#include "pecfManager/pecfManager.h"

ECSPLUGIN_ENTRY(ecsHnd, reloadFlag) {
  PECF_MANAGER_PLUGIN_TYPE pecfMngr = (PECF_MANAGER_PLUGIN_TYPE)ecsHnd->getSystem(PECF_MANAGER_PLUGIN_NAME);
  

  
}

ECSPLUGIN_EXIT(ecsHnd, reloadFlag) {

}