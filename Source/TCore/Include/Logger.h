#pragma once
#include "TCore.h"

TCORE_BEGIN_C_LINKAGE
TCORE_PLUGIN_DEFINE(TCLog, "tcLog", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

/*
 * You should init the log system. This will make every log to be saved in the main log file
 * If you want to save specific type of logs to a specific text file, you can query it with save().
 * All logs are saved as UTF32.
 * You can use all UTFs by defining them with proper format argument: 8 -> %s, 16 -> %v, 32 -> %w
 * There is a limit of characters per log. It is defined with maxCharPerLog_tapi constant.
 */

typedef enum TCLogLevel
{
	TC_LOG_LEVEL_STATUS,
	TC_LOG_LEVEL_WARNING,
	TC_LOG_LEVEL_ERROR,
	TC_LOG_LEVEL_NOT_CODED,
	TC_LOG_LEVEL_CRASHING,
	TC_LOG_LEVEL_ALL
} TCLogLevel;

typedef struct ITCLog
{
	void (*Initialize)(const char* logFilePath);
	// You can set file paths different each time you write to file
	// Also you can set paths as NULL, if you don't want to change file path
	// Formats are the same as string api's
	void (*Save)(enum TCLogLevel logType, const char* filePath);
	// If time_point is 0, current time will be used. Otherwise, the time_point will be used to log the log's time.
	// If owner is NULL, the log's owner will be caller plugin's name.
	void (*Log)(enum TCLogLevel type, size_t time_point, const char* owner, const char* text, const char* details);
} ITCLog;

TCORE_END_C_LINKAGE

// C++ wrapper
#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
#include <TString.h>

namespace TCore
{

class td
{
};

class tl
{
public:
	tl(TCLogLevel level, const char* owner = TCORE_ACTIVE_PLUGIN_NAME) : Level(level)
	{
		TCORE_SOFT_CHECK(TCLog,
						 "Log system is not initialized. Please call "
						 "TCLog->Initialize() before using the log system.");
	}

	~tl()
	{
		if (!TCLog)
			return;
		TCLog->Log(Level, 0, NULL, Message.CStr(), Details.CStr());
	}

	void StartDetailing() { IsStartedDetailing = true; }

	void Append(const char* text)
	{
		if (IsStartedDetailing)
			Details.Append(text);
		else
			Message.Append(text);
	}

	TCLogLevel Level;
	TCore::String Message;
	TCore::String Details;
	bool IsStartedDetailing = false;

	tl& operator<<(const td& d)
	{
		StartDetailing();
		return *this;
	}

#if defined(_SSTREAM_) || defined(_GLIBCXX_SSTREAM) || defined(_LIBCPP_SSTREAM)
	tl& operator<<(const std::ostringstream& t)
	{
		Append(t.str().c_str());
		return *this;
	}
#endif

	tl& operator<<(const char* l)
	{
		Append(l);
		return *this;
	}
};

class tw : public tl
{
public:
	tw(const char* owner = TCORE_ACTIVE_PLUGIN_NAME) : tl(TC_LOG_LEVEL_WARNING, owner) {}
};

class ti : public tl
{
public:
	ti(const char* owner = TCORE_ACTIVE_PLUGIN_NAME) : tl(TC_LOG_LEVEL_STATUS, owner) {}
};

class te : public tl
{
public:
	te(const char* owner = TCORE_ACTIVE_PLUGIN_NAME) : tl(TC_LOG_LEVEL_ERROR, owner) {}
};

} // namespace TCore
#endif