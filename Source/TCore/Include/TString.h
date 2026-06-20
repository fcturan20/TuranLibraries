#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCString, "tcString", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

TCORE_DEFINE_HANDLE(TCString);
typedef struct ITCString
{
	TCStringHandle (*Create)(const char* str);
	void (*Destroy)(TCStringHandle str);
	void (*Append)(TCStringHandle str, const char* str_to_append);
	void (*Clear)(TCStringHandle str);
	void (*Set)(TCStringHandle str, const char* new_str);
	const char* (*CStr)(TCStringHandle str);
	void (*Resize)(TCStringHandle str, size_t new_capacity);
	TCStringHandle (*Substring)(TCStringHandle str, size_t start_index, size_t end_index);
} ITCString;

TCORE_END_C_LINKAGE

#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
// C++ wrapper
namespace TCore
{
class String
{
public:
	String(const char* str = nullptr) { Handle = TCString->Create(str); }
	String(const String& other) { Handle = TCString->Create(other.CStr()); }
	String& operator=(const String& other)
	{
		if (this != &other)
		{
			TCString->Destroy(Handle);
			Handle = TCString->Create(other.CStr());
		}
		return *this;
	}
	String(String&& other) noexcept
	{
		Handle = other.Handle;
		other.Handle = nullptr;
	}
	String& operator=(String&& other) noexcept
	{
		if (this != &other)
		{
			TCString->Destroy(Handle);
			Handle = other.Handle;
			other.Handle = nullptr;
		}
		return *this;
	}
	String& operator+=(const char* str_to_append)
	{
		TCString->Append(Handle, str_to_append);
		return *this;
	}
	String& operator+=(const String& other)
	{
		TCString->Append(Handle, other.CStr());
		return *this;
	}
	String& operator=(const char* new_str)
	{
		TCString->Set(Handle, new_str);
		return *this;
	}
	String(TCStringHandle handle) : Handle(handle) {}
	~String()
	{
		if (Handle)
		{
			TCString->Destroy(Handle);
		}
	}
	String& Append(const char* str_to_append)
	{
		TCString->Append(Handle, str_to_append);
		return *this;
	}
	String& Clear()
	{
		TCString->Clear(Handle);
		return *this;
	}
	String& Set(const char* new_str)
	{
		TCString->Set(Handle, new_str);
		return *this;
	}
	const char* CStr() const { return TCString->CStr(Handle); }
	void Resize(size_t new_capacity) { TCString->Resize(Handle, new_capacity); }
	String Substring(size_t start_index, size_t end_index)
	{
		return TCString->Substring(Handle, start_index, end_index);
	}

#if defined(_XSTRING_) || defined(_LIBCPP_STRING) || defined(_GLIBCXX_STRING)
	operator std::string() const { return std::string(CStr()); }
	String& operator=(const std::string& str)
	{
		TCString->Set(Handle, str.c_str());
		return *this;
	}
	String& operator+=(const std::string& str)
	{
		TCString->Append(Handle, str.c_str());
		return *this;
	}
	// Converting constructor from std::string to support:
	//   TCore::String textData = std::string("abd");
	String(const std::string& str) { Handle = TCString->Create(str.c_str()); }
#endif
private:
	TCStringHandle Handle;
};
} // namespace TCore
#endif