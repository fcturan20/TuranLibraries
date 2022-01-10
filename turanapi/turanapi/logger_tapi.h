#include "predefinitions.h"
#define LOGGER_TAPI_PLUGIN_NAME "tapi_logger"
#define LOGGER_TAPI_PLUGIN_VERSION  MAKE_PLUGIN_VERSION_TAPI(0,0,0)
#define LOGGER_TAPI_PLUGIN_LOAD_TYPE logger_tapi_type*

typedef struct logger_tapi{
	void (*log_crashing)(const char* log);
	void (*log_error)(const char* log);
	void (*log_warning)(const char* log);
	void (*log_status)(const char* log);
	void (*log_notcoded)(const char* log, unsigned char stop_running);
    //You can set file paths different each time you write to file
    //Also you can set paths NULL if you don't want to change file path
	void (*writelogs_tofile)(const char* mainlogfile, const char* errorlogfile, const char* warninglogfile, const char* notcodedfile);
} logger_tapi;

typedef struct logger_tapi_d logger_tapi_d;
typedef struct logger_tapi_type{
    logger_tapi_d* data;
    logger_tapi* funcs;
} logger_tapi_type;


