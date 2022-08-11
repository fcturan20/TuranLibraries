#include "scene.h"
#include "ecs_tapi.h"
#include "pecfManager/pecfManager.h"

componentManager_ecs ecs_mngr;
struct sceneSettingsComponentManager{
  static componentHnd_ecstapi createComponent() {
    return nullptr;
  }
  static compType_ecstapi getComponent(const char* componentName) {
    return nullptr;
  }
  static void destroyComponent(componentHnd_ecstapi) {

  }
  static componentManagerInfo_pecf getManager() {
    ecs_mngr.createComponent = sceneSettingsComponentManager::createComponent;
    ecs_mngr.destroyComponent = sceneSettingsComponentManager::destroyComponent;

    componentManagerInfo_pecf c;
    c.compName = SCENE_SETTINGS_COMP_NAME;
    c.ecsManager = &ecs_mngr;
    return c;
  }
  static compType_ecstapi COMPTYPE;
};
compType_ecstapi sceneSettingsComponentManager::COMPTYPE = nullptr;

void ThrowFor_PECF_Register(void* result, const char* msg) { if (!result) { throw(msg); } }
const char* compRegFailedText = "Registering a component failed, but it's not designed to do so!";


ECSPLUGIN_ENTRY(ecssys, reloadflag) {
  PECF_MANAGER_PLUGIN_TYPE pecfmngr = (PECF_MANAGER_PLUGIN_TYPE)ecssys->getSystem(PECF_MANAGER_PLUGIN_NAME);
  //Plugin is loaded from Editor
  if (pecfmngr) {
    //pecfmngr->add_componentType("Scene Settings Component", sceneSettingsComponentManager::getManager());
  }
  //Plugin is loaded from Game
  else {
    
  }

  
  ThrowFor_PECF_Register
  (
    pecfmngr->add_componentType(sceneSettingsComponentManager::getManager()), compRegFailedText
  );
}