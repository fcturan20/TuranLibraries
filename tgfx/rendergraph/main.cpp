#include "ecs_tapi.h"
#include "tgfx_core.h"
#include <stdio.h>

ECSPLUGIN_ENTRY(ecsSYS, reloadFlag){
  auto tgfxsys = (TGFX_PLUGIN_LOAD_TYPE)ecsSYS->getSystem(TGFX_PLUGIN_NAME);
  if(!tgfxsys){
    printf("You're trying to load RenderGraph sys before loading TGFX, which is invalid!\n");
    return;
  }
}

ECSPLUGIN_EXIT(ecsSYS, reloadFlag){

}