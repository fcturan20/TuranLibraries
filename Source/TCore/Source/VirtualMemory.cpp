#include "VirtualMemory.h"

#include <stdio.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCVirtualMemory)
TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCVirtualMemory)
TCORE_PLUGIN_ENTRY_POINT_END()

#if defined(T_ENVMACOS) || defined(T_ENVLINUX)
#include <sys/mman.h>
#include <unistd.h>
#elif defined(T_ENVWINDOWS)
#include <windows.h>
#endif

namespace TCore
{
namespace VirtualMemory
{

struct Context
{

#if defined(T_ENVMACOS) || defined(T_ENVLINUX)
	// Reserve address space from virtual memory
	static void* Reserve(unsigned long long size)
	{
		return mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	}

	// Initialize the reserved memory with zeros.
	static void Commit(void* ptr, unsigned long long commitsize)
	{
		if (commitsize && mprotect(ptr, commitsize, PROT_READ | PROT_WRITE) == -1)
		{
			perror("mprotect failed");
		}
	}

	// Return back the committed memory to reserved state
	// This will help if you want to catch some bugs that points to memory you just freed.
	static void Decommit(void* ptr, unsigned long long size)
	{
		if (mprotect(ptr, size, PROT_NONE) == -1)
		{
			perror("mprotect failed");
		}
	}

	// Free the pages allocated
	static void Free(void* ptr, unsigned long long size) { munmap(ptr, size); }

	static unsigned int GetPageSize() { return (unsigned int)sysconf(_SC_PAGESIZE); }
#elif defined(T_ENVWINDOWS)
	// Reserve address space from virtual memory
	static void* Reserve(unsigned long long size) { return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE); }

	// Initialize the reserved memory with zeros.
	static void Commit(void* ptr, unsigned long long commitsize)
	{
		if (VirtualAlloc(ptr, commitsize, MEM_COMMIT, PAGE_READWRITE) == NULL && commitsize)
		{
			LPVOID msgBuf;
			DWORD dw = GetLastError();

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						  NULL,
						  dw,
						  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						  (LPTSTR)&msgBuf,
						  0,
						  NULL);

			SYSTEM_INFO si;
			GetSystemInfo(&si);

			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery(ptr, &mbi, sizeof(mbi));

			static constexpr char empty[] = "";
			static constexpr char notPageAlignedError[] = "Allocation is not page-aligned";
			printf("Virtual Alloc failed: %s\tOS Error: %d, %s",
				   (uintptr_t)ptr % si.dwPageSize != 0 ? notPageAlignedError : empty,
				   dw,
				   (const char*)msgBuf);

			printf("Base=%p\n", mbi.BaseAddress);
			printf("State=%lx\n", mbi.State);
			printf("Protect=%lx\n", mbi.Protect);
			printf("RegionSize=%zu\n", mbi.RegionSize);
		}
	}

	template <int flag>
	static void Free(void* ptr, unsigned long long size)
	{
		if (!VirtualFree(ptr, size, flag))
		{
			// Handle error
			LPVOID msgBuf;
			DWORD dw = GetLastError();

			FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						  NULL,
						  dw,
						  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						  (LPTSTR)&msgBuf,
						  0,
						  NULL);

			printf("Virtual Free failed: OS Error: %d, %s", dw, (const char*)msgBuf);
		}
	}

	// Return back the committed memory to reserved state
	// This will help if you want to catch some bugs that points to memory you just freed.
	static void Decommit(void* ptr, unsigned long long size) { Free<MEM_DECOMMIT>(ptr, size); }

	// Free the pages allocated
	static void Free(void* ptr, unsigned long long size) { Free<MEM_RELEASE>(ptr, size); }

	static TU4 GetPageSize()
	{
		SYSTEM_INFO si;
		GetSystemInfo(&si);
		return si.dwPageSize;
	}
#endif
};
} // namespace VirtualMemory
} // namespace TCore

TCResult TCVirtualMemory_Initialize(const void** outPluginAPI)
{
	auto services = new ITCVirtualMemory;
	services->Reserve = TCore::VirtualMemory::Context::Reserve;
	services->Commit = TCore::VirtualMemory::Context::Commit;
	services->Decommit = TCore::VirtualMemory::Context::Decommit;
	services->Free = TCore::VirtualMemory::Context::Free;
	services->GetPageSize = TCore::VirtualMemory::Context::GetPageSize;

	TCVirtualMemory = services;
	*outPluginAPI = TCVirtualMemory;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCVirtualMemory_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCVirtualMemory_Shutdown()
{
	if (TCVirtualMemory)
	{
		delete TCVirtualMemory;
		TCVirtualMemory = nullptr;
	}
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TCVirtualMemory_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}