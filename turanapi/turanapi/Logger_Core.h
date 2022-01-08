#pragma once
#include "API_includes.h"





#ifdef __cplusplus
extern "C" {
#endif

	void Start_LoggingSystem(const char* LogFolder);

	void tapi_LOG_CRASHING(const char* log);
	void tapi_LOG_ERROR(const char* log);
	void tapi_LOG_WARNING(const char* log);
	void tapi_LOG_STATUS(const char* log);
	void tapi_LOG_NOTCODED(const char* log, unsigned char stop_running);
	void tapi_WRITE_LOGs_toFILEs();


#ifdef __cplusplus
}
#endif


