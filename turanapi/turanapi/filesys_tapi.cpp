#include "filesys_tapi.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ecs_tapi.h"
#include "string_tapi.h"

void failedToRead_tapi(stringArgument_tapi(path)) {
  switch (pathType) {
    case string_type_tapi_UTF8:
      printf("There is no such file: %s\n", ( const char* )pathData);
      break;
    case string_type_tapi_UTF16:
      wprintf(L"There is no such file %s\n", ( const wchar_t* )pathData);
      break;
    default: break;
  }
}
template <typename T>
void openFile_tapi(T& file, stringArgument_tapi(path), std::ios::openmode openMode = 1) {
  switch (pathType) {
    case string_type_tapi_UTF8: file.open(( const char* )pathData, openMode); break;
    case string_type_tapi_UTF16: file.open(( const wchar_t* )pathData, openMode); break;
    default: break;
  }
}
void* read_binaryfile(stringArgument_tapi(path), unsigned long* size) {
  std::ifstream binaryFile;
  openFile_tapi(binaryFile, pathType, pathData, std::ios::binary | std::ios::in | std::ios::ate);
  if (!(binaryFile.is_open())) {
    failedToRead_tapi(pathType, pathData);
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

void overwrite_binaryfile(stringArgument_tapi(path), void* data, unsigned long datasize) {
  // ios::trunc is used to clear the file before outputting the data!
  std::ofstream outputFile;
  openFile_tapi(outputFile, pathType, pathData, std::ios::binary | std::ios::out | std::ios::trunc);
  if (!outputFile.is_open()) {
    failedToRead_tapi(pathType, pathData);
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

void* read_textfile(stringArgument_tapi(path), string_type_tapi fileTextType) {
  switch (fileTextType) {
    case string_type_tapi_UTF8: {
      std::ifstream cTextFile;
      openFile_tapi(cTextFile, pathType, pathData);
      std::stringstream stringData;
      stringData << cTextFile.rdbuf();
      cTextFile.close();
      char*        finaltext = new char[stringData.str().length() + 1]{'\n'};
      unsigned int i         = 0;
      while (stringData.str()[i] != '\0') {
        finaltext[i] = stringData.str()[i];
        i++;
      }
      return finaltext;
    } break;
    case string_type_tapi_UTF16: {
      std::ifstream wTextFile;
      openFile_tapi(wTextFile, pathType, pathData);
      std::wstringstream stringData;
      stringData << wTextFile.rdbuf();
      wTextFile.close();
      wchar_t*     finaltext = new wchar_t[stringData.str().length() + 1]{'\n'};
      unsigned int i         = 0;
      while (stringData.str()[i] != '\0') {
        finaltext[i] = stringData.str()[i];
        i++;
      }
      return finaltext;
    } break;
  }
}
void write_textfile(stringArgument_tapi(text), stringArgument_tapi(path),
                    unsigned char writeToEnd) {
  std::ios::openmode openMode;
  if (writeToEnd) {
    openMode = std::ios::out | std::ios::app;
  } else {
    openMode = std::ios::out | std::ios::trunc;
  }
  switch (textType) {
    case string_type_tapi_UTF8: {
      std::ofstream outputFile;
      openFile_tapi(outputFile, pathType, pathData, openMode);
      outputFile << ( const char* )textData << std::endl;
      outputFile.close();
    } break;
    case string_type_tapi_UTF16: {
      std::wofstream outputFile;
      openFile_tapi(outputFile, pathType, pathData, openMode);
      outputFile << ( const wchar_t* )textData << std::endl;
      outputFile.close();
    } break;
  }
}
void delete_file(stringArgument_tapi(path)) {
  switch (pathType) {
    case string_type_tapi_UTF8: std::filesystem::remove(( const char* )pathData); break;
    case string_type_tapi_UTF16: std::filesystem::remove(( wchar_t* )pathData); break;
  }
}

typedef struct filesys_tapi_d {
  filesys_tapi_type* type;
} filesys_tapi_d;

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  filesys_tapi_type* type = ( filesys_tapi_type* )malloc(sizeof(filesys_tapi_type));
  type->data              = ( filesys_tapi_d* )malloc(sizeof(filesys_tapi_d));
  type->funcs             = ( filesys_tapi* )malloc(sizeof(filesys_tapi));
  type->data->type        = type;

  type->funcs->read_binaryfile      = &read_binaryfile;
  type->funcs->overwrite_binaryfile = &overwrite_binaryfile;
  type->funcs->write_binaryfile     = &overwrite_binaryfile;
  type->funcs->read_textfile        = &read_textfile;
  type->funcs->write_textfile       = &write_textfile;
  type->funcs->delete_file          = &delete_file;

  ecssys->addSystem(FILESYS_TAPI_PLUGIN_NAME, FILESYS_TAPI_PLUGIN_VERSION, type);
}

ECSPLUGIN_EXIT(ecssys, reloadFlag) {}