#define T_INCLUDE_PLATFORM_LIBS
#include "ecs_tapi.h"
#include "pecfManager/pecfManager.h"
#include "main.h"
#include "threadingsys_tapi.h"
#include "allocator_tapi.h"
#include <stdio.h>
ecs_tapi* editorECS = nullptr;
extern void initialize_pecfManager();
extern void load_systems();

struct baseCompManager {
  struct baseCOMPSTRUCT {
    unsigned int dir[3];
    float speed;
  };
  static baseCOMPSTRUCT* v_BASECOMPs;
  //Access variables using this
  struct baseCOMPTYPE {
    unsigned int (*getDirX)(componentHnd_ecstapi comp), (*getDirY)(componentHnd_ecstapi comp), (*getDirZ)(componentHnd_ecstapi comp);
    float (*getSpeed)(componentHnd_ecstapi comp);
  };
  static baseCOMPTYPE* type;
  static componentHnd_ecstapi createComp() {
    baseCOMPSTRUCT base_d;
    base_d.dir[0] = 1;
    base_d.dir[1] = 2;
    base_d.dir[2] = 3;
    base_d.speed = 231.f;
    allocatorSys->vector_manager->push_back(v_BASECOMPs, &base_d);
    return reinterpret_cast<componentHnd_ecstapi>(&v_BASECOMPs[allocatorSys->vector_manager->size(v_BASECOMPs) - 1]);
  }
  static void destroyComp(componentHnd_ecstapi) {

  }
  static compTypeHnd_ecstapi getCompType(const char* name) {
    return reinterpret_cast<compTypeHnd_ecstapi>(&type);
  }
  static componentManager_ecs getECSCompManager() {
    componentManager_ecs mngr;
    mngr.createComponent = createComp;
    mngr.destroyComponent = destroyComp;
    return mngr;
  }
  static unsigned int getDir(componentHnd_ecstapi comp) {
    return ((baseCOMPSTRUCT*)comp)->dir[2];
  };
  static float getSpeed(componentHnd_ecstapi comp) {
    return ((baseCOMPSTRUCT*)comp)->speed;
  };
  static void initialize() {
    supermemoryblock_tapi* baseCompMngrAlloc = allocatorSys->createSuperMemoryBlock(1 << 16, "BaseCompManager");
    v_BASECOMPs = TAPI_CREATEVECTOR(allocatorSys->vector_manager, baseCOMPSTRUCT, baseCompMngrAlloc, 100);

    type = new baseCOMPTYPE;
    type->getDirX = getDir; type->getDirY = getDir; type->getDirZ = getDir; type->getSpeed = getSpeed;
  }
};
baseCompManager::baseCOMPSTRUCT* baseCompManager::v_BASECOMPs = nullptr;
baseCompManager::baseCOMPTYPE* baseCompManager::type;

struct derivedCompManager {
  struct derivenCOMPSTRUCT {
    unsigned long long time;
    float speed;
  };
  static derivenCOMPSTRUCT* v_COMPs;
  //Access variables using this
  static baseCompManager::baseCOMPTYPE* baseType;
  struct derivenCOMPTYPE {
    unsigned int (*getSpeedXTime)(componentHnd_ecstapi comp);
  };
  static derivenCOMPTYPE* type;
  static componentHnd_ecstapi createComp() {
    derivenCOMPSTRUCT base_d;
    base_d.time = 365;
    base_d.speed = 5.f;
    allocatorSys->vector_manager->push_back(v_COMPs, &base_d);
    return reinterpret_cast<componentHnd_ecstapi>(&v_COMPs[allocatorSys->vector_manager->size(v_COMPs) - 1]);
  }
  static void destroyComp(componentHnd_ecstapi) {

  }
  static compTypeHnd_ecstapi getCompType(const char* name) {
    return reinterpret_cast<compTypeHnd_ecstapi>(&type);
  }
  static componentManager_ecs getECSCompManager() {
    componentManager_ecs mngr;
    mngr.createComponent = createComp;
    mngr.destroyComponent = destroyComp;
    return mngr;
  }
  static unsigned int getDir(componentHnd_ecstapi comp) {
    return ((derivenCOMPSTRUCT*)comp)->time * 2;
  };
  static float getSpeed(componentHnd_ecstapi comp) {
    return ((derivenCOMPSTRUCT*)comp)->speed;
  };
  static unsigned int getSpeedXTime(componentHnd_ecstapi comp) {
    derivenCOMPSTRUCT* data = ((derivenCOMPSTRUCT*)comp);
    return data->speed * data->time;
  }
  static void initialize() {
    supermemoryblock_tapi* baseCompMngrAlloc = allocatorSys->createSuperMemoryBlock(1 << 16, "BaseCompManager");
    v_COMPs = TAPI_CREATEVECTOR(allocatorSys->vector_manager, derivenCOMPSTRUCT, baseCompMngrAlloc, 100);

    baseType = new baseCompManager::baseCOMPTYPE;
    baseType->getDirX = getDir; baseType->getDirY = getDir; baseType->getDirZ = getDir; baseType->getSpeed = getSpeed;
  
    type = new derivenCOMPTYPE;
    type->getSpeedXTime = getSpeedXTime;
  }
};
derivedCompManager::derivenCOMPSTRUCT* derivedCompManager::v_COMPs = nullptr;
derivedCompManager::derivenCOMPTYPE* derivedCompManager::type;
baseCompManager::baseCOMPTYPE* derivedCompManager::baseType;

int main(){
  auto ecs_tapi_dll = DLIB_LOAD_TAPI("tapi_ecs.dll");
  if (!ecs_tapi_dll) { printf("There is no tapi_ecs.dll, initialization failed!"); exit(-1); }
  load_ecstapi_func ecsloader = (load_ecstapi_func)DLIB_FUNC_LOAD_TAPI(ecs_tapi_dll, "load_ecstapi");
  if (!ecsloader) { printf("tapi_ecs.dll is loaded but ecsloader func isn't found!"); exit(-1); }
  editorECS = ecsloader();
  if (!editorECS) { printf("ECS initialization failed!"); exit(-1); }

  initialize_pecfManager();
  load_systems();

  baseCompManager::initialize();
  derivedCompManager::initialize();
  compTypeID_ecstapi baseCompTypeID = editorECS->addComponentType("BASE", reinterpret_cast<compType_ecstapi>(baseCompManager::type), baseCompManager::getECSCompManager(), nullptr, 0);
  ecs_compTypePair pair; pair.base = baseCompTypeID; pair.overriden = (compType_ecstapi)derivedCompManager::baseType;
  compTypeID_ecstapi derivedCompTypeID = editorECS->addComponentType("DERIVED", reinterpret_cast<compType_ecstapi>(derivedCompManager::type), derivedCompManager::getECSCompManager(), &pair, 1);
  
  // Create an entity with base type and get value of the component
  {
    compTypeID_ecstapi typeList[1] = { baseCompTypeID };
    entityTypeHnd_ecstapi baseEntityType = editorECS->addEntityType(typeList, 1);
    entityHnd_ecstapi baseEntity = editorECS->createEntity(baseEntityType);
    compTypeHnd_ecstapi overridenCompType;
    componentHnd_ecstapi baseComp = editorECS->get_component_byEntityHnd(baseEntity, baseCompTypeID, &overridenCompType);
    printf("Base Component DirZ: %d \n", reinterpret_cast<baseCompManager::baseCOMPTYPE*>(overridenCompType)->getDirZ(baseComp));
  }

  // Create an entity with deriven type and get value of the component
  {
    compTypeID_ecstapi typeList[1] = { derivedCompTypeID };
    entityTypeHnd_ecstapi derivenEntityType = editorECS->addEntityType(typeList, 1);
    entityHnd_ecstapi derivenEntity = editorECS->createEntity(derivenEntityType);
    compTypeHnd_ecstapi overridenCompType;
    componentHnd_ecstapi baseComp = editorECS->get_component_byEntityHnd(derivenEntity, baseCompTypeID, &overridenCompType);
    printf("Derived Component DirZ: %d \n", reinterpret_cast<baseCompManager::baseCOMPTYPE*>(overridenCompType)->getDirZ(baseComp));
    compTypeHnd_ecstapi derivedOverridenCompType;
    componentHnd_ecstapi derivedComp = editorECS->get_component_byEntityHnd(derivenEntity, derivedCompTypeID, &derivedOverridenCompType);
    printf("Derived Component SpeedXTime: %d \n", reinterpret_cast<derivedCompManager::derivenCOMPTYPE*>(derivedOverridenCompType)->getSpeedXTime(derivedComp));
  }

  return 1;
}