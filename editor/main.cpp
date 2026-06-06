#define T_INCLUDE_PLATFORM_LIBS
#include "main.h"
#include <cstdlib>
#include <stdio.h>

#include "allocator_tapi.h"
#include "ecs_tapi.h"
#include "pecfManager/pecfManager.h"
#include "threadingsys_tapi.h"
const tlECS*   editorECS = nullptr;
extern void initialize_pecfManager();
extern void load_systems();

int main() {
  auto ecs_tapi_dll = DLIB_LOAD_TAPI("tapi_ecs" DLIB_EXTENSION_TAPI);
  if (!ecs_tapi_dll) {
    printf("There is no tapi_ecs" DLIB_EXTENSION_TAPI ", initialization failed!");
    exit(-1);
  }
  tlECSloadFnc ecsloader =
    ( tlECSloadFnc )DLIB_FUNC_LOAD_TAPI(ecs_tapi_dll, "load_ecstapi");
  if (!ecsloader) {
    printf("tapi_ecs" DLIB_EXTENSION_TAPI " is loaded but ecsloader func isn't found!");
    exit(-1);
  }
  editorECS = ecsloader();
  if (!editorECS) {
    printf("ECS initialization failed!");
    exit(-1);
  }

  // initialize_pecfManager();
  load_systems();

  return 1;
}