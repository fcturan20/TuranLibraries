#include "filesys_tapi.h"
#include "registrysys_tapi.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

void* read_binaryfile(const char* path, unsigned long* size){
	std::ifstream Binary_File;
	Binary_File.open(path, std::ios::binary | std::ios::in | std::ios::ate);
	if (!(Binary_File.is_open())) {
		std::cout << "There is no such file: " << path << std::endl;
		return nullptr;
	}

	Binary_File.seekg(0, std::ios::end);
	unsigned long long length = Binary_File.tellg();
	Binary_File.seekg(0, std::ios::beg);
	char* read_data = new char[length];
	Binary_File.read(read_data, length);
	Binary_File.close();
	*size = length;
	return read_data;
}

void overwrite_binaryfile(const char* path, void* data, unsigned long datasize){
	//ios::trunc is used to clear the file before outputting the data!
	std::ofstream Output_File(path, std::ios::binary | std::ios::out | std::ios::trunc);
	if (!Output_File.is_open()) {
		std::cout << "Error: " << path << " couldn't be outputted!\n";
		return;
	}
	if (data == nullptr) {
		std::cout << "data is nullptr!\n";
	}
	if (datasize == 0) {
			std::cout << "data size is 0!\n";
		}
		//Write to a file and finish all of the operation!
		Output_File.write((const char*)data, datasize);
		Output_File.close();
		std::cout << path << " is outputted successfully!\n";
}

char* read_textfile(const char* path){
	char* finaltext = nullptr;
	std::ifstream textfile;
	textfile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	try
	{
		textfile.open(path);
		std::stringstream string_data;
		string_data << textfile.rdbuf();
		textfile.close();
		finaltext = new char[string_data.str().length() + 1]{ '\n' };
		unsigned int i = 0;
		while (string_data.str()[i] != '\0') {
			finaltext[i] = string_data.str()[i];
			i++;
		}
		return finaltext;
	}
	catch (std::ifstream::failure error)
	{
		std::cout << "Error: Text file couldn't read: " << path << std::endl;
		return finaltext;
	}
}
void write_textfile(const char* text, const char* path, unsigned char write_to_end){
	std::ofstream Output_File;
	if (write_to_end) {
		Output_File.open(path, std::ios::out | std::ios::app);
		Output_File << text << std::endl;
		Output_File.close();
	}
	else {
		Output_File.open(path, std::ios::out | std::ios::trunc);
		Output_File << text << std::endl;
		Output_File.close();
	}
}
void delete_file(const char* path){
		std::remove(path);
}


typedef struct filesys_tapi_d{
    filesys_tapi_type* type;
} filesys_tapi_d;

extern "C" FUNC_DLIB_EXPORT void* load_plugin(registrysys_tapi* regsys, unsigned char reload){
    filesys_tapi_type* type = (filesys_tapi_type*)malloc(sizeof(filesys_tapi_type));
    type->data = (filesys_tapi_d*)malloc(sizeof(filesys_tapi_d));
    type->funcs = (filesys_tapi*)malloc(sizeof(filesys_tapi));
    type->data->type = type;

    type->funcs->read_binaryfile = &read_binaryfile;
    type->funcs->overwrite_binaryfile = &overwrite_binaryfile;
    type->funcs->write_binaryfile = &overwrite_binaryfile;
    type->funcs->read_textfile = &read_textfile;
    type->funcs->write_textfile = &write_textfile;
    type->funcs->delete_file = &delete_file;

    regsys->add(FILESYS_TAPI_PLUGIN_NAME, FILESYS_TAPI_PLUGIN_VERSION, type);
    return type;
}

extern "C" FUNC_DLIB_EXPORT void unload_plugin(registrysys_tapi* regsys, unsigned char reload){

}