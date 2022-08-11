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


int main(){
  auto ecs_tapi_dll = DLIB_LOAD_TAPI("tapi_ecs.dll");
  if (!ecs_tapi_dll) { printf("There is no tapi_ecs.dll, initialization failed!"); exit(-1); }
  load_ecstapi_func ecsloader = (load_ecstapi_func)DLIB_FUNC_LOAD_TAPI(ecs_tapi_dll, "load_ecstapi");
  if (!ecsloader) { printf("tapi_ecs.dll is loaded but ecsloader func isn't found!"); exit(-1); }
  editorECS = ecsloader();
  if (!editorECS) { printf("ECS initialization failed!"); exit(-1); }

  initialize_pecfManager();
  load_systems();

  return 1;
}