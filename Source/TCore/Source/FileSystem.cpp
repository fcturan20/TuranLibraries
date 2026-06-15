#include "FileSystem.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

TCORE_PLUGIN_INIT(TS, TC)
TCORE_PLUGIN_INIT(TSFS, TCFileSystem)

void* ReadBinaryFile(const char* path, unsigned long* size) {
  std::ifstream binaryFile;
  binaryFile.open(path, std::ios::binary | std::ios::in | std::ios::ate);
  if (!binaryFile.is_open()) {
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

void OverwriteBinaryFile(const char* path, void* data, unsigned long datasize) {
  // ios::trunc is used to clear the file before outputting the data!
  std::ofstream outputFile;
  outputFile.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!outputFile.is_open()) {
    printf("Failed to open file: %s\n", path);
    return;
  }
  if (data == nullptr) {
    std::cout << "data is nullptr!\n";
  }
  if (datasize == 0) {
    std::cout << "data size is 0!\n";
  }
  // Write to a file and finish all of the operation!
  outputFile.write(( const char* )data, datasize);
  outputFile.close();
  printf("File output is successful\n");
}

void* ReadTextFile(const char* path, unsigned long* size) {
  std::ifstream cTextFile;
  cTextFile.open(path);
  if (!cTextFile.is_open()) {
    printf("There is no such file: %s\n", path);
    return nullptr;
  }

  std::stringstream stringData;
  stringData << cTextFile.rdbuf();
  cTextFile.close();
  
  // Allocate memory for the text data and copy it there
  char*        finaltext = new char[stringData.str().length() + 1]{'\n'};
  unsigned int i         = 0;
  while (stringData.str()[i] != '\0') {
    finaltext[i] = stringData.str()[i];
    i++;
  }
  finaltext[i] = '\0';
  *size = i;
  return finaltext;
}

void WriteTextFile(const char* text, const char* path, TBool writeToEnd) {
  std::ios::openmode openMode = std::ios::out;
  if (writeToEnd) 
     openMode |= std::ios::app;
  else
    openMode |= std::ios::trunc;
  
  std::ofstream outputFile;
  outputFile.open(path, openMode);
  if (!outputFile.is_open()) {
    printf("Failed to open file: %s\n", path);
    return;
  }
  outputFile << text << std::endl;
  outputFile.close();
}

void DeleteFile(const char* path) {
  std::filesystem::remove(path);
}

void BindPluginFunctions(TCPluginFunctions* funcs){
  funcs->Initialize = [](const void** outPluginAPI) -> TCResult {
    TCFileSystem* api = new TCFileSystem;
    api->ReadBinaryFile = ReadBinaryFile;
    api->WriteBinaryFile = OverwriteBinaryFile;
    api->OverwriteBinaryFile = OverwriteBinaryFile;
    api->ReadTextFile = ReadTextFile;
    api->WriteTextFile = WriteTextFile;
    api->DeleteFile = DeleteFile;
    *outPluginAPI = api;
    return TC_RESULT_SUCCESS;
  };
  funcs->OnPreShutdown = []() -> TCResult {
    return TC_RESULT_SUCCESS;
  };
  funcs->Shutdown = []() -> TCResult {
    delete TSFS;
    return TC_RESULT_SUCCESS;
  };
}
TCORE_PLUGIN_ENTRY_POINT_START(TSFS)
TCORE_PLUGIN_ENTRY_POINT_END()