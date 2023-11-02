#pragma once

#ifdef __cplusplus
extern "C" {
#endif
// Forward Declarations
/////////////////////////////////

typedef struct tlECS            ecs_tapi;
typedef struct primitiveData_pecf* primDataHnd_pecf;
typedef struct primitiveType_pecf* primTypeHnd_pecf;
typedef struct funcType_pecf*      funcTypeHnd_pecf;

typedef struct editor_gameLinker_callStack* callStackHnd_gameLinker_editor;

// Compile-time constants
///////////////////

static constexpr unsigned int GAMELINKER_MAXVARNAME = 80;

// This struct is to capture the value of a primitive data
// Before starting a function end after ending a function,
typedef struct editor_gameLinker_activeVar {
  char             name[GAMELINKER_MAXVARNAME];
  primTypeHnd_pecf type; //
  primDataHnd_pecf data; // This is a pointer to memory allocated by linker
} activeVar_gameLinker_editor;
typedef void (*editor_gameLinker_callStackDestroyFunc)(editor_gameLinker_activeVar* varList);

typedef struct editor_gameLinker {
  // Register a plugin to game
  // load_plugins.cpp file (Game) will be modified to load this plugin while initializing
  // @return 0 if fails, 1 if suceeds
  unsigned char (*regPlugin_toGame)(const char* path);

  // If a game child process is running, this will return its ECS
  ecs_tapi* (*getGame_ECS)();

  // Just concepts
  ///////////////////////

  // You can delete log by calling stringSys->delete()
  unsigned char (*compileGame)(const char** log);
  unsigned char (*runGame)();

  void (*addToCallStack)(funcTypeHnd_pecf func, editor_gameLinker_activeVar* captureList,
                         editor_gameLinker_callStackDestroyFunc destroyFunc);
  unsigned char (*getCallStack)(callStackHnd_gameLinker_editor* callstack);
  unsigned char (*setCallStackSize)(unsigned int callCount);
} gameLinker_editor;

#ifdef __cplusplus
}
#endif