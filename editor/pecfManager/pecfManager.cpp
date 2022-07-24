#include "pecfManager.h"
#include "stdio.h"
#include "predefinitions_tapi.h"
#include "ecs_tapi.h"
#include "main.h"


//  Because both engine and TuranLibraries don't know about the editor
//   primitiveTypes of their systems should be added by default here
//  Adding a pecfType should be in a pecfManagerPlugin, but there is no need to add a plugin for them
//   because editor is already built upon TuranEngine. 

static pecf_manager* mngr;

compTypeHnd_pecf add_componentType(componentManagerInfo_pecf mngr) {
  return 0;
}
entityHnd_pecf add_entity(const char* entityTypeName) {
  return 0;
}
unsigned char registerPlugin_toGame(const char* path) {
  return 0;
}
unsigned char add_node(nodeProperties props, primitiveVariable* inputArgs, unsigned int inputArgsCount, primitiveVariable* outputArgs, unsigned int outputArgsCount) {
  return 0;
}
const ecs_tapi* get_gameECS() {
  return nullptr;
}


void setFuncPtrs() {
  mngr->add_componentType = add_componentType;
  mngr->add_entity = add_entity;
  mngr->add_node = add_node;
  mngr->get_gameECS = get_gameECS;
  mngr->regPlugin_toGame = registerPlugin_toGame;
}
void initialize_pecfManager(){
  mngr = new pecf_manager;
  setFuncPtrs();
  editorECS->addSystem(PECF_MANAGER_PLUGIN_NAME, PECF_MANAGER_PLUGIN_VERSION, mngr);  //Now other plugins can access PECF Manager using editor ECS
}