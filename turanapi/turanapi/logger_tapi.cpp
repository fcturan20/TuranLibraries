#include "logger_tapi.h"

#include <iostream>
#include <string>
#include <vector>

#include "array_of_strings_tapi.h"
#include "ecs_tapi.h"
#include "filesys_tapi.h"
#include "virtualmemorysys_tapi.h"

enum class LOG_TYPE : unsigned char {
  CRASHING_ERROR = 0,
  SOME_ERROR     = 1,
  WARNING        = 2,
  STATUS         = 3,
  NOT_CODEDPATH  = 4
};

struct LOG {
  std::string TEXT;
  LOG_TYPE    TYPE;
};
struct Logger {
  std::string      MainLogFile_Path;
  std::string      WarningLogFile_Path;
  std::string      ErrorLogFile_Path;
  std::string      NotCodedLogFile_Path;
  std::vector<LOG> LOGLIST;
};
static Logger*       LOGSYS  = nullptr;
static filesys_tapi* filesys = nullptr;

#define GET_printer(i) (*GET_LOGLISTTAPI())[i]
inline void breakpoint(const char* log) {
  std::cout << "Crashing ERROR: " << log << "\ny to continue, n to exit application" << std::endl;
  char breakpoint_choice = 0;
  while (breakpoint_choice != 'y' || breakpoint_choice != 'n') {
    std::cin >> &breakpoint_choice;
  }
  if (breakpoint_choice == 'n') {
    exit(-1);
  }
}

void WRITE_LOGs_toFILEs(const char* mainlogfile, const char* errorlogfile,
                        const char* warninglogfile, const char* notcodedfile) {
  if (!mainlogfile) {
    LOGSYS->MainLogFile_Path = mainlogfile;
  }
  if (!errorlogfile) {
    LOGSYS->ErrorLogFile_Path = errorlogfile;
  }
  if (!warninglogfile) {
    LOGSYS->WarningLogFile_Path = warninglogfile;
  }
  if (!notcodedfile) {
    LOGSYS->NotCodedLogFile_Path = notcodedfile;
  }
  if (LOGSYS->LOGLIST.size() == 0) {
    std::cout << "There is no log to write!\n";
    return;
  }
  std::string MainLogFile_Text, ErrorLogFile_Text, WarningLogFile_Text, NotCodedLogFile_Text;
  LOG*        log_data = nullptr;
  for (unsigned int i = 0; i < LOGSYS->LOGLIST.size(); i++) {
    log_data = &LOGSYS->LOGLIST[i];
    switch (log_data->TYPE) {
      case LOG_TYPE::CRASHING_ERROR:
      case LOG_TYPE::SOME_ERROR:
        MainLogFile_Text.append(log_data->TEXT);
        MainLogFile_Text.append("\n");
        ErrorLogFile_Text.append(log_data->TEXT);
        ErrorLogFile_Text.append("\n");
        break;
      case LOG_TYPE::WARNING:
        MainLogFile_Text.append(log_data->TEXT);
        MainLogFile_Text.append("\n");
        WarningLogFile_Text.append(log_data->TEXT);
        WarningLogFile_Text.append("\n");
        break;
      case LOG_TYPE::NOT_CODEDPATH:
        MainLogFile_Text.append(log_data->TEXT);
        MainLogFile_Text.append("\n");
        ErrorLogFile_Text.append(log_data->TEXT);
        ErrorLogFile_Text.append("\n");
        NotCodedLogFile_Text.append(log_data->TEXT);
        NotCodedLogFile_Text.append("\n");
        break;
      case LOG_TYPE::STATUS:
        MainLogFile_Text.append(log_data->TEXT);
        MainLogFile_Text.append("\n");
        break;
      default: break;
    }
  }
  filesys->write_textfile(MainLogFile_Text.c_str(), LOGSYS->MainLogFile_Path.c_str(), true);
  filesys->write_textfile(ErrorLogFile_Text.c_str(), LOGSYS->ErrorLogFile_Path.c_str(), true);
  filesys->write_textfile(WarningLogFile_Text.c_str(), LOGSYS->WarningLogFile_Path.c_str(), true);
  filesys->write_textfile(NotCodedLogFile_Text.c_str(), LOGSYS->NotCodedLogFile_Path.c_str(), true);

  LOGSYS->LOGLIST.clear();
}

void LOG_CRASHING(const char* log) {
  LOG* log_data = nullptr;
  LOGSYS->LOGLIST.push_back(LOG());
  log_data       = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
  log_data->TEXT = log;
  log_data->TYPE = LOG_TYPE::CRASHING_ERROR;
  WRITE_LOGs_toFILEs(nullptr, nullptr, nullptr, nullptr);
  breakpoint(log);
}

void LOG_ERROR(const char* log) {
  LOG* log_data = nullptr;
  LOGSYS->LOGLIST.push_back(LOG());
  log_data       = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
  log_data->TEXT = log;
  log_data->TYPE = LOG_TYPE::SOME_ERROR;

  std::cout << "Error: " << log_data->TEXT << std::endl;
}

void LOG_WARNING(const char* log) {
  LOG* log_data = nullptr;
  LOGSYS->LOGLIST.push_back(LOG());
  log_data       = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
  log_data->TEXT = log;
  log_data->TYPE = LOG_TYPE::WARNING;

  std::cout << "Warning: " << log_data->TEXT << std::endl;
}

void LOG_STATUS(const char* log) {
  LOG* log_data = nullptr;
  LOGSYS->LOGLIST.push_back(LOG());
  log_data       = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
  log_data->TEXT = log;
  log_data->TYPE = LOG_TYPE::STATUS;
  std::cout << "Status: " << log_data->TEXT << std::endl;
}

void LOG_NOTCODED(const char* log, unsigned char stop_running) {
  LOG* log_data = nullptr;
  LOGSYS->LOGLIST.push_back(LOG());
  log_data       = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
  log_data->TEXT = log;
  log_data->TYPE = LOG_TYPE::NOT_CODEDPATH;

  std::cout << "Not Coded Path: " << log_data->TEXT << std::endl;

  if (stop_running) {
    WRITE_LOGs_toFILEs(nullptr, nullptr, nullptr, nullptr);
    breakpoint(log);
  }
}

typedef struct logger_tapi_d {
  logger_tapi_type* type;
  Logger*           log_sys;
} logger_tapi_d;
ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  VIRTUALMEMORY_TAPI_PLUGIN_TYPE virmemsys_type =
    ( VIRTUALMEMORY_TAPI_PLUGIN_TYPE )ecssys->getSystem(VIRTUALMEMORY_TAPI_PLUGIN_NAME);
  ARRAY_OF_STRINGS_TAPI_LOAD_TYPE array_of_strings_sys_type =
    ( ARRAY_OF_STRINGS_TAPI_LOAD_TYPE )ecssys->getSystem(ARRAY_OF_STRINGS_TAPI_PLUGIN_NAME);
  FILESYS_TAPI_PLUGIN_LOAD_TYPE filesys_type =
    ( FILESYS_TAPI_PLUGIN_LOAD_TYPE )ecssys->getSystem(FILESYS_TAPI_PLUGIN_NAME);

  if (!virmemsys_type || !array_of_strings_sys_type || !filesys_type) {
    printf(
      "Logger system needs virtual memory, array of strings and file systems loaded. So loading "
      "failed!");
    return;
  }

  LOGGER_TAPI_PLUGIN_LOAD_TYPE type =
    ( LOGGER_TAPI_PLUGIN_LOAD_TYPE )malloc(sizeof(logger_tapi_type));
  type->data       = ( logger_tapi_d* )malloc(sizeof(logger_tapi_d));
  type->funcs      = ( logger_tapi* )malloc(sizeof(logger_tapi));
  type->data->type = type;

  type->funcs->log_crashing     = &LOG_CRASHING;
  type->funcs->log_error        = &LOG_ERROR;
  type->funcs->log_notcoded     = &LOG_NOTCODED;
  type->funcs->log_status       = &LOG_STATUS;
  type->funcs->log_warning      = &LOG_WARNING;
  type->funcs->writelogs_tofile = &WRITE_LOGs_toFILEs;

  ecssys->addSystem(LOGGER_TAPI_PLUGIN_NAME, LOGGER_TAPI_PLUGIN_VERSION, type);

  LOGSYS              = new Logger;
  type->data->log_sys = LOGSYS;
}
ECSPLUGIN_EXIT(ecssys, reloadFlag) { printf("Not coded!"); }