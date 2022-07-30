#define PECF_MANAGER_PLUGIN_NAME "PECF Manager"
#define PECF_MANAGER_PLUGIN_VERSION MAKE_PLUGIN_VERSION_TAPI(0,0,0)
#define PECF_MANAGER_PLUGIN_TYPE pecf_manager*


// PECF is the whole purpose behind TuranEditor, writing applications
// UI code can access PECF to decide how to visualize ECS types/values and do visual scripting
// EDITOR Init:
//  1) Application load ECS, then register PECFManager to ECS.
//  2) Then application looks at the workspace directory for a config file to load plugins.
// GAME Init:
//  1) Game loads ECS, then loads all plugins from a load_plugins.cpp file.
// 
// EDITOR - GAME:
//  1) When an editor plugin wants to register to game permanently, it should use registerPlugin_toGame
//  2) Visual scripting nodes are functions from plugins with known header locations etc.
//   So editor will write a .cpp file to convert nodes to cpp code. Then will compile them into a plugin.
//  3) Because game will run on a loop, one of the registered plugins should manage it
// 
// HOW TO WRITE PLUGINS:
//  1) Each plugin is a ECSTAPI plugin in its core, so entry point should be ECSPLUGIN_ENTRY
//  2) Plugin can access PECF Manager with given ECS, get_system(PECF_MANAGER_PLUGIN_NAME)
//  3) If PECF Manager isn't found, that means game loaded the plugin (so only do game stuff)
//  4) PECF Manager functions is just to visualize, nothing functional
//  5) To add functionality to Editor: you should add it to ECS then interact with other systems etc.
//  Note: It's better not to store editor functionality with game functionality in the same plugin
//   So you should store game code of your plugins in a different plugin that'll be only loaded by game.


// PECF's extensible resources;
//  1) Component Types: Stores: ECS component manager, component type-wide callbacks and info about its ECS component struct (its primitiveDataTypes etc). 
//      A PECF component is a handle to a struct of: pointer to ECS component, vector of component specific callbacks
//  2) Entities: A vector that holds componentHnds.
//      In ECS terminology, an PECF_Entity holds both an entity and an entityType
//  3) Primitive Data Types: Stores info about how a C struct is defined
//      So you can define primitive data types as a collection of other ones (Structs of structs)
//      Default Primitive Data Types: Bool, UINT32, Vec3, String, PTR, ComponentHnd.
//  4) Nodes: Stores a func pointer and how to access it in a .cpp file



#ifdef __cplusplus
extern "C" {
#endif


// Handles
////////////////////////////////////

typedef struct entity_pecf* entityHnd_pecf;
typedef struct component_pecf* compHnd_pecf;
typedef struct componentType_pecf* compTypeHnd_pecf;
typedef struct primitiveData_pecf* primDataHnd_pecf;
typedef struct primitiveType_pecf* primTypeHnd_pecf;


typedef struct ecstapi_idOnlyPointer* entityTypeHnd_ecstapi;
typedef struct ecstapi_compType* compTypeHnd_ecstapi;
typedef struct ecstapi_component* componentHnd_ecstapi;
typedef struct ecstapi_entity* entityHnd_ecstapi;
typedef struct ecstapi_plugin* pluginHnd_ecstapi;
typedef struct ecs_tapi ecs_tapi;
typedef struct ecs_componentManager componentManager_ecs;


// Callbacks
////////////////////////////////////

typedef void entity_onChangedFunc(entityHnd_pecf entity);
typedef void comp_onChangedFunc(compHnd_pecf comp);

// Structs
////////////////////////////////////

//Primitive types'll store primitive variables
typedef struct primitiveVariable{
  const char* varName;
  primTypeHnd_pecf primType;
} primitiveVariable;



// Visualize/Edit the component's values in editor
typedef struct componentEditor{
    // Do operations while initializing editor
    void (*editorInitialization)();
    // View components values in entity inspector view
    void (*editorVisualizationTab)(const componentHnd_ecstapi* handle);
} componentEditor;


typedef enum{
    nodeType_funcNode,
    nodeType_debugNode,
    nodeType_convertNode    //There shouldn't be more than 2 arguments
    //Overloaded nodes isn't supported because it'll affect nodeName system belove
    //Just give another name, don't automate that much!
} nodeType;


typedef struct nodeProperties{
    /* TUTORIAL:
    nodeName is used to visualize node in editor
    funcHeaderPath & castingTypeName is used to find the typedef of the function
    accessWay is used to access function in game
    So game code'll be like:
    #include "#funcHeaderPath"
    static castingTypeName nodeName;

    void load_plugin(ecs* gameECS){
        nodeName = gameECS->get("#pluginName")->accessWay;
    }
    */
    const char* pluginName, *funcHeaderPath, *nodeName, *castingTypeName, *accessWay;
    nodeType nodeType;
} nodeProperties;

typedef struct componentManagerInfo_pecf {
  const char* compName;
  //ECS Manager will handle 
  const componentManager_ecs* ecsManager;
};

typedef struct pecf_manager{
    const ecs_tapi* (*get_gameECS)();
    // Expose a function to Editor as a node
    // You should also expose function typedefs in a header in the plugin
    // @return 0 if registeration fails, 1 if succeeds
    unsigned char (*add_node)(nodeProperties props, primitiveVariable* inputArgs, unsigned int inputArgsCount, primitiveVariable* outputArgs, unsigned int outputArgsCount);



    primTypeHnd_pecf(*add_primitiveType)(const char* primName, unsigned int primDataSize, const primitiveVariable* varList, unsigned int varCount);
    // To display a component's variables, a window needs these informations
    // But this doesn't help in programming side, because primitive variable should be created in proper C struct
    void(*get_primitiveTypeInfo)(primTypeHnd_pecf hnd, const char** primName, unsigned int* primDataSize, primitiveVariable** varList, unsigned int* varCount);



    // Register a component type to Editor UI and ECS
    // @return NULL if registeration fails, otherwise valid handle
    compTypeHnd_pecf(*add_componentType)(componentManagerInfo_pecf mngr);
    // Both of get/set is lockful, so a system will either old or new version
    // Ordering which one is gotten (old or new) is a high-level problem
    void(*get_componentPrimInfo)(compHnd_pecf comp, unsigned int index, primitiveVariable* primInfo);
    // memcpy() primitive data to "dst". dst should be a pointer to a buffer of primitive data size
    void(*get_componentPrimData)(compHnd_pecf comp, unsigned int index, void* dst);
    // memcpy() primitive data from "src". src should be a pointer to a buffer of primitive data size
    // Also calls comp's registered onChanged callbacks.
    void(*set_componentPrimData)(compHnd_pecf comp, unsigned int index, const void* src);




    entityHnd_pecf (*add_entity)(const char* entityTypeName);
    // @return 1 if succeeds, 0 if fails
    unsigned char (*add_componentToEntity)(entityHnd_pecf entity, compTypeHnd_pecf compType);


    // Register a plugin to game
    // load_plugins.cpp file (Game) will be modified to load this plugin while initializing
    // @return 0 if fails, 1 if suceeds
    unsigned char (*regPlugin_toGame)(const char* path);

    // Callback Registrations
    // @param reg_orUnreg: 1 if you're registering, 0 if you're unregistering

    unsigned char (*regOnChanged_entity)(entity_onChangedFunc callback, entityHnd_ecstapi entity, unsigned char reg_orUnreg);
    unsigned char (*regOnChanged_comp)(comp_onChangedFunc callback, compHnd_pecf comp, unsigned char reg_orUnreg);
} pecf_manager;


#ifdef __cplusplus
}
#endif

#ifdef ECS_PLUGIN_NAME_HASHER  //This is used to find the plugin name to do name -> hash key conversion
#define ECS_PLUGIN_NAME PECF_MANAGER_PLUGIN_NAME 
#endif