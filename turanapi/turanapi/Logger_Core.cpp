#include "Logger_Core.h"
#include <iostream>
#include <vector>
#include <string>
#include "FileSystem_Core.h"



enum class LOG_TYPE : char {
	CRASHING_ERROR = 0, ERROR = 1, WARNING = 2, STATUS = 3, NOT_CODEDPATH = 4
};

struct LOG {
	std::string TEXT;
	LOG_TYPE TYPE;
};
struct Logger {
	std::string MainLogFile_Path;
	std::string WarningLogFile_Path;
	std::string ErrorLogFile_Path;
	std::string NotCodedLogFile_Path;
	std::vector<LOG> LOGLIST;
};
Logger* LOGSYS = nullptr;



#define GET_LOG(i) (*GET_LOGLISTTAPI())[i]

void Start_LoggingSystem(const char* LogFolder) {
	LOGSYS = new Logger;
	LOGSYS->ErrorLogFile_Path = (LogFolder + std::string("/errors.txt")).c_str();
	LOGSYS->MainLogFile_Path = (LogFolder + std::string("/logs.txt")).c_str();
	LOGSYS->NotCodedLogFile_Path = (LogFolder + std::string("/notcodedpaths.txt")).c_str();
	LOGSYS->WarningLogFile_Path = (LogFolder + std::string("/warnings.txt")).c_str();

	tapi_Write_TextFile("TuranAPI: Logging Started!", LOGSYS->MainLogFile_Path.c_str(), false);
	tapi_Write_TextFile("TuranAPI: Logging Started!", LOGSYS->WarningLogFile_Path.c_str(), false);
	tapi_Write_TextFile("TuranAPI: Logging Started!", LOGSYS->ErrorLogFile_Path.c_str(), false);
	tapi_Write_TextFile("TuranAPI: Logging Started!", LOGSYS->NotCodedLogFile_Path.c_str(), false);
}



void tapi_WRITE_LOGs_toFILEs() {
	if (LOGSYS->LOGLIST.size() == 0) {
		std::cout << "There is no log to write!\n";
		return;
	}
	std::string MainLogFile_Text, ErrorLogFile_Text, WarningLogFile_Text, NotCodedLogFile_Text;
	LOG* log_data = nullptr;
	for (unsigned int i = 0; i < LOGSYS->LOGLIST.size(); i++) {
		log_data = &LOGSYS->LOGLIST[i];
		switch (log_data->TYPE)
		{
		case LOG_TYPE::CRASHING_ERROR:
		case LOG_TYPE::ERROR:
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
		default:
			break;
		}
	}
	tapi_Write_TextFile(MainLogFile_Text.c_str(), LOGSYS->MainLogFile_Path.c_str(), true);
	tapi_Write_TextFile(ErrorLogFile_Text.c_str(), LOGSYS->ErrorLogFile_Path.c_str(), true);
	tapi_Write_TextFile(WarningLogFile_Text.c_str(), LOGSYS->WarningLogFile_Path.c_str(), true);
	tapi_Write_TextFile(NotCodedLogFile_Text.c_str(), LOGSYS->NotCodedLogFile_Path.c_str(), true);

	LOGSYS->LOGLIST.clear();
}

void tapi_LOG_CRASHING(const char* log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::CRASHING_ERROR;
	tapi_WRITE_LOGs_toFILEs();
	tapi_breakpoint(log);
}

void tapi_LOG_ERROR(const char* log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::ERROR;


	std::cout << "Error: " << log_data->TEXT << std::endl;
}

void tapi_LOG_WARNING(const char* log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::WARNING;

	std::cout << "Warning: " << log_data->TEXT << std::endl;
}

void tapi_LOG_STATUS(const char* log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::STATUS;
	std::cout << "Status: " << log_data->TEXT << std::endl;
}

void tapi_LOG_NOTCODED(const char* log, bool stop_running) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::NOT_CODEDPATH;

	std::cout << "Not Coded Path: " << log_data->TEXT << std::endl;

	if (stop_running) {
		tapi_WRITE_LOGs_toFILEs();
		tapi_breakpoint(nullptr);
	}
}



//String
void tapi_LOG_CRASHING(const std::string& log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];


	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::CRASHING_ERROR;
	tapi_WRITE_LOGs_toFILEs();
	tapi_breakpoint(log.c_str());
}
void tapi_LOG_ERROR(const std::string& log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::ERROR;


	std::cout << "Error: " << log_data->TEXT << std::endl;
}
void tapi_LOG_WARNING(const std::string& log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::WARNING;

	std::cout << "Warning: " << log_data->TEXT << std::endl;
}
void tapi_LOG_STATUS(const std::string& log) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::STATUS;
	std::cout << "Status: " << log_data->TEXT << std::endl;
}
void tapi_LOG_NOTCODED(const std::string& log, bool stop_running) {
	LOG* log_data = nullptr;
	LOGSYS->LOGLIST.push_back(LOG());
	log_data = &LOGSYS->LOGLIST[LOGSYS->LOGLIST.size() - 1];
	log_data->TEXT = log;
	log_data->TYPE = LOG_TYPE::NOT_CODEDPATH;

	std::cout << "Not Coded Path: " << log_data->TEXT << std::endl;

	if (stop_running) {
		tapi_WRITE_LOGs_toFILEs();
		tapi_breakpoint(nullptr);
	}
}

