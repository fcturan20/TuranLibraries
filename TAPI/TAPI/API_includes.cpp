#include "API_includes.h"
#include <iostream>
#include <string>
#include <vector>



void tapi_breakpoint(const char* Breakpoint_Reason) {
	if (Breakpoint_Reason != nullptr) {
		std::cout << "Crashing Reason: " << Breakpoint_Reason << std::endl;
	}
	char input;
	std::cout << "Application hit a breakpoint, would you like to continue? (Note: Be careful, application may crash!) \nTo continue: enter y, to close: enter n \nInput: ";
	std::cin >> input;
	if (input == 'y') {
		return;
	}
	else if (input == 'n') {
		exit(EXIT_FAILURE);
	}
	else {
		std::cout << "Wrong input:\n";
		tapi_breakpoint(nullptr);
	}
}

