#include "filesys_tapi.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "ecs_tapi.h"
#include "string_tapi.h"

void failedToRead_tapi(stringReadArgument_tapi(path)) {
  switch (pathType) {
    case tlStringUTF8: printf("There is no such file: %s\n", ( const char* )pathData); break;
    case tlStringUTF16: wprintf(L"There is no such file %s\n", ( const wchar_t* )pathData); break;
    default: break;
  }
}
template <typename T>
void openFile_tapi(T& file, stringReadArgument_tapi(path), std::ios::openmode openMode = 1) {
  switch (pathType) {
    case tlStringUTF8: file.open(( const char* )pathData, openMode); break;
    case tlStringUTF16: file.open(( const wchar_t* )pathData, openMode); break;
    default: break;
  }
}
void* read_binaryfile(stringReadArgument_tapi(path), unsigned long* size) {
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

void overwrite_binaryfile(stringReadArgument_tapi(path), void* data, unsigned long datasize) {
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

void* read_textfile(stringReadArgument_tapi(path), string_type_tapi fileTextType) {
  switch (fileTextType) {
    case tlStringUTF8: {
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
    case tlStringUTF16: {
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
void write_textfile(stringReadArgument_tapi(text), stringReadArgument_tapi(path),
                    unsigned char writeToEnd) {
  std::ios::openmode openMode;
  if (writeToEnd) {
    openMode = std::ios::out | std::ios::app;
  } else {
    openMode = std::ios::out | std::ios::trunc;
  }
  switch (textType) {
    case tlStringUTF8: {
      std::ofstream outputFile;
      openFile_tapi(outputFile, pathType, pathData, openMode);
      outputFile << ( const char* )textData << std::endl;
      outputFile.close();
    } break;
    case tlStringUTF16: {
      std::wofstream outputFile;
      openFile_tapi(outputFile, pathType, pathData, openMode);
      outputFile << ( const wchar_t* )textData << std::endl;
      outputFile.close();
    } break;
  }
}
void delete_file(stringReadArgument_tapi(path)) {
  switch (pathType) {
    case tlStringUTF8: std::filesystem::remove(( const char* )pathData); break;
    case tlStringUTF16: std::filesystem::remove(( wchar_t* )pathData); break;
  }
}

typedef struct tlIOPriv {
  tlIO* type;
} filesys_tapi_d;

ECSPLUGIN_ENTRY(ecssys, reloadFlag) {
  tlIO*     type   = ( tlIO* )malloc(sizeof(tlIO));
  tlIOPriv* priv   = ( tlIOPriv* )malloc(sizeof(tlIOPriv));
  priv->type       = type;

  type->readBinary      = &read_binaryfile;
  type->overwriteBinary = &overwrite_binaryfile;
  type->overwriteBinary = &overwrite_binaryfile;
  type->readText        = &read_textfile;
  type->writeText       = &write_textfile;
  type->deleteFile      = &delete_file;
  type->d               = priv;

  ecssys->addSystem(FILESYS_TAPI_PLUGIN_NAME, FILESYS_TAPI_PLUGIN_VERSION, type);
}

ECSPLUGIN_EXIT(ecssys, reloadFlag) {}