#pragma once
#include "TCore.h"
TCORE_BEGIN_C_LINKAGE

TCORE_PLUGIN_DEFINE(TCStringSys, "tcString", TCORE_MAKE_PLUGIN_VERSION(0, 0, 0))

TCORE_DEFINE_HANDLE(TCString);

typedef struct ITCStringSys
{
	TCString (*Create)(const char* str);
	void (*Destroy)(TCString str);
	void (*Append)(TCString str, const char* str_to_append);
	void (*Clear)(TCString str);
	void (*Set)(TCString str, const char* new_str);
	const char* (*CStr)(TCString str);
	char* (*Data)(TCString str);
	void (*Resize)(TCString str, size_t new_capacity);
	TCString (*Substring)(TCString str, size_t start_index, size_t end_index);
} ITCStringSys;

TCORE_END_C_LINKAGE

#if defined(TCORE_CPP_20) & defined(TCORE_USE_CPP_WRAPPER)
// C++ wrapper
namespace TCore
{
class String
{
public:
	String(const char* str = nullptr) { Handle = TCStringSys->Create(str); }
	String(const String& other) { Handle = TCStringSys->Create(other.CStr()); }
	String& operator=(const String& other)
	{
		if (this != &other)
		{
			TCStringSys->Destroy(Handle);
			Handle = TCStringSys->Create(other.CStr());
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
			TCStringSys->Destroy(Handle);
			Handle = other.Handle;
			other.Handle = nullptr;
		}
		return *this;
	}
	String& operator+=(const char* str_to_append)
	{
		TCStringSys->Append(Handle, str_to_append);
		return *this;
	}
	String& operator+=(const String& other)
	{
		TCStringSys->Append(Handle, other.CStr());
		return *this;
	}
	String& operator=(const char* new_str)
	{
		TCStringSys->Set(Handle, new_str);
		return *this;
	}
	String(TCString handle) : Handle(handle) {}
	~String()
	{
		if (Handle)
			TCStringSys->Destroy(Handle);
	}
	String& Append(const char* str_to_append)
	{
		TCStringSys->Append(Handle, str_to_append);
		return *this;
	}
	String& Clear()
	{
		TCStringSys->Clear(Handle);
		return *this;
	}
	String& Set(const char* new_str)
	{
		TCStringSys->Set(Handle, new_str);
		return *this;
	}
	const char* CStr() const { return TCStringSys->CStr(Handle); }
	char* Data() const { return TCStringSys->Data(Handle); }
	void Resize(size_t new_capacity) { TCStringSys->Resize(Handle, new_capacity); }
	String Substring(size_t start_index, size_t end_index)
	{
		return TCStringSys->Substring(Handle, start_index, end_index);
	}

#if defined(_XSTRING_) || defined(_LIBCPP_STRING) || defined(_GLIBCXX_STRING)
	operator std::string() const { return std::string(CStr()); }
	String& operator=(const std::string& str)
	{
		TCStringSys->Set(Handle, str.c_str());
		return *this;
	}
	String& operator+=(const std::string& str)
	{
		TCStringSys->Append(Handle, str.c_str());
		return *this;
	}
	// Converting constructor from std::string to support:
	//   TCore::String textData = std::string("abd");
	String(const std::string& str) { Handle = TCStringSys->Create(str.c_str()); }
#endif

	operator TCString() const { return Handle; }

protected:
	TCString Handle;
};

} // namespace TCore
#endif