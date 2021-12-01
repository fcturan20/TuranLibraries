#include "Profiler_Core.h"
#include "Logger_Core.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>



tapi_ProfiledScope LastProfiledScope = nullptr;

	struct TURANAPI Profiled_Scope {
	public:
		bool Is_Recording : 1;
		unsigned char TimingType : 2;
		unsigned long long START_POINT;
		unsigned long long* DURATION;
		std::string NAME;
	};
	/*
	class TURANAPI Profiler_System {
		void* START_POINT_PTR;
		std::vector<Profiled_Scope>* PROFILED_SCOPEs_vector;
	public:
		Profiler_System();
		~Profiler_System();

		void Save_a_ProfiledScope_toSession(const Profiled_Scope& PROFILED_SCOPE);
	};
	Profiler_System* PROFSYS = nullptr;

	void tapi_StartProfilingSystem)() {
		PROFSYS = new Profiler_System;
	}*/
	void tapi_Profile_Start(tapi_ProfiledScope* handle, const char* NAME, unsigned long long* duration, unsigned char TimingTypeIndex) {
		Profiled_Scope* profil = new Profiled_Scope;
		switch (TimingTypeIndex) {
		case 0:
			profil->START_POINT = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		case 1:
			profil->START_POINT = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		case 2:
			profil->START_POINT = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		case 3:
			profil->START_POINT = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		default:
			tapi_breakpoint("You should specify timing between 0-3, not anything else!");
			return;
		}
		profil->Is_Recording = true;
		profil->DURATION = duration;
		profil->TimingType = TimingTypeIndex;
		profil->NAME = NAME;
		*handle = (tapi_ProfiledScope)profil;
		LastProfiledScope = *handle;
	}
	void tapi_Profile_End(unsigned char ShouldPrint) {
		Profiled_Scope* profil = (Profiled_Scope*)LastProfiledScope;
		switch (profil->TimingType) {
		case 0:
			*profil->DURATION = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
			if (ShouldPrint) {tapi_LOG_STATUS((profil->NAME + " took: " + std::to_string(*profil->DURATION) + "nanoseconds!").c_str());}
			return;
		case 1:
			*profil->DURATION = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
			if (ShouldPrint) {tapi_LOG_STATUS((profil->NAME + " took: " + std::to_string(*profil->DURATION) + "microseconds!").c_str());}
			return;
		case 2:
			*profil->DURATION = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count() - profil->START_POINT;
			if (ShouldPrint) {tapi_LOG_STATUS((profil->NAME + " took: " + std::to_string(*profil->DURATION) + "milliseconds!").c_str());}
			return;
		case 3:
			*profil->DURATION = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			if (ShouldPrint) {tapi_LOG_STATUS((profil->NAME + " took: " + std::to_string(*profil->DURATION) + "seconds!").c_str());}
			return;
		default:
			tapi_breakpoint("You should specify timing between 0-3, not anything else!");
			return;
		}
		delete profil;
	}

