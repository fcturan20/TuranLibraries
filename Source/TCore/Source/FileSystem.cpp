// External
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#define TCORE_USE_CPP_WRAPPER
#include "FileSystem.h"

#include <UnitTestSystem.h>
#include <Allocator.h>

TCORE_PLUGIN_INIT(TC)
TCORE_PLUGIN_INIT(TCFileSystem)
TCORE_PLUGIN_INIT(TCUnitTest)
TCORE_PLUGIN_INIT(TCAllocator)
TCORE_PLUGIN_MEMORY_BLOCK_INIT()

TCORE_PLUGIN_BOUNDED_ENTRY_POINT_START(TCFileSystem)
TCORE_PLUGIN_HARD_DEPENDENCY(TCAllocator, TCAllocator_PLUGIN_VERSION)
TCORE_PLUGIN_RESERVE_ADDRESS_SPACE(1 << 30);
TCORE_PLUGIN_ENTRY_POINT_END()

namespace TCore
{
namespace FileSystem
{
struct TCFileSystemContext* GContext = nullptr;
struct TCFileSystemContext
{
	static void Initialize() { GContext = new TCFileSystemContext; }
	static void* ReadBinaryFile(const char* path, TU8* size)
	{
		std::ifstream binaryFile;
		binaryFile.open(path, std::ios::binary | std::ios::in | std::ios::ate);
		if (!binaryFile.is_open())
		{
			printf("There is no such file: %s\n", path);
			return nullptr;
		}

		binaryFile.seekg(0, std::ios::end);
		unsigned long long length = binaryFile.tellg();
		binaryFile.seekg(0, std::ios::beg);
		char* read_data = new char[length];
		binaryFile.read(read_data, length);
		binaryFile.close();
		*size = length;
		return read_data;
	}

	static void OverwriteBinaryFile(const char* path, void* data, TU8 datasize)
	{
		// ios::trunc is used to clear the file before outputting the data!
		std::ofstream outputFile;
		outputFile.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
		if (!outputFile.is_open())
		{
			printf("Failed to open file: %s\n", path);
			return;
		}
		if (data == nullptr)
			std::cout << "data is nullptr!\n";
		if (datasize == 0)
			std::cout << "data size is 0!\n";
		// Write to a file and finish all of the operation!
		outputFile.write((const char*)data, datasize);
		outputFile.close();
		printf("File output is successful\n");
	}

	static void* ReadTextFile(const char* path, TU8* size)
	{
		std::ifstream cTextFile;
		cTextFile.open(path);
		if (!cTextFile.is_open())
		{
			printf("There is no such file: %s\n", path);
			return nullptr;
		}

		std::stringstream stringData;
		stringData << cTextFile.rdbuf();
		cTextFile.close();

		// Allocate memory for the text data and copy it there
		char* finaltext = new char[stringData.str().length() + 1]{'\n'};
		unsigned int i = 0;
		while (stringData.str()[i] != '\0')
		{
			finaltext[i] = stringData.str()[i];
			i++;
		}
		finaltext[i] = '\0';
		if (size)
			*size = i;
		return finaltext;
	}

	static void WriteTextFile(const char* text, const char* path, TBool writeToEnd)
	{
		std::ios::openmode openMode = std::ios::out;
		if (writeToEnd)
			openMode |= std::ios::app;
		else
			openMode |= std::ios::trunc;

		std::ofstream outputFile;
		outputFile.open(path, openMode);
		if (!outputFile.is_open())
		{
			printf("Failed to open file: %s\n", path);
			return;
		}
		outputFile << text << std::endl;
		outputFile.close();
	}

	static void DeleteFile(const char* path) { std::filesystem::remove(path); }
};

void RegisterUnitTests()
{
	{
		TCUnitTestDescription desc = {};
		desc.Name = "ReadBinaryFile";
		desc.GlobalCategoryName = "FileSystem";
		desc.Test = [](TCReadBuffer) -> TCResult {
			TU8 size = 0;
			auto currentPath = std::filesystem::current_path();
			void* data = TCFileSystemContext::ReadBinaryFile("test.bin", &size);
			if (!data)
			{
				return {TC_RESULTSTATE_FAILURE, 0};
			}
			printf("ReadBinaryFile test passed, size: %llu\n", size);
			delete[] static_cast<char*>(data);
			return {TC_RESULTSTATE_SUCCESS, 0};
		};
		TCUnitTest->RegisterTest(&desc);
	}
}

} // namespace FileSystem
} // namespace TCore

TCResult TCFileSystem_Initialize(const void** outPluginAPI)
{
	auto services = new ITCFileSystem;
	services->ReadBinaryFile = TCore::FileSystem::TCFileSystemContext::ReadBinaryFile;
	services->OverwriteBinaryFile = TCore::FileSystem::TCFileSystemContext::OverwriteBinaryFile;
	services->ReadTextFile = TCore::FileSystem::TCFileSystemContext::ReadTextFile;
	services->WriteTextFile = TCore::FileSystem::TCFileSystemContext::WriteTextFile;
	services->DeleteFile = TCore::FileSystem::TCFileSystemContext::DeleteFile;

	const TCPluginInfo* unitTestInfo{};
	if (TC->GetPlugin(TCUnitTest_PLUGIN_NAME, TCUnitTest_PLUGIN_VERSION, &unitTestInfo, (const void**)&TCUnitTest) ==
		TC_RESULTSTATE_SUCCESS)
		TCore::FileSystem::RegisterUnitTests();

	TCFileSystem = services;
	*outPluginAPI = TCFileSystem;
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCFileSystem_OnPreShutdown()
{
	return {TC_RESULTSTATE_SUCCESS, 0};
}

TCResult TCFileSystem_Shutdown()
{
	if (TCFileSystem)
	{
		delete TCFileSystem;
		TCFileSystem = nullptr;
	}
	return {TC_RESULTSTATE_SUCCESS, 0};
}

void TCFileSystem_OnPluginLoadStateChange(const TCPluginInfo* pluginInfo, TBool isLoaded)
{
	// This plugin doesn't react to other plugins being loaded or unloaded, so this function is empty.
}