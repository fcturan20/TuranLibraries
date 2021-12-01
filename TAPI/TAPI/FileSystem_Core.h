#pragma once
#include "API_includes.h"



#ifdef __cplusplus
extern "C" {
#endif

	/*
		1) This class is to add data types to Engine!
		2) An added data type should be type of .gamecont or .enginecont

		Handles basic IO operations and provides an interface for writing a File_System in a DLL
		Because there is a lack of Data Model, every filesystem should handle its own ID generation
		So that means, there may some collisions between assets because of FileSystems' ID generation systems.
	*/
	void* tapi_Read_BinaryFile(const char* path, unsigned int* size);
	void tapi_Write_BinaryFile(const char* path, void* data, unsigned int size);
	void tapi_Overwrite_BinaryFile(const char* path, void* data, unsigned int size);
	void tapi_Delete_File(const char* path);


	char* tapi_Read_TextFile(const char* path);
	void tapi_Write_TextFile(const char* text, const char* path, unsigned char write_to_end);
	void tapi_StartFileSystem();
	void tapi_EndFileSystem();



#ifdef __cplusplus
}
#endif



#define TAPIFILESYSTEM FileSystem