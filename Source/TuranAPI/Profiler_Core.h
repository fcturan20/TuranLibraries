#pragma once
#include "TuranAPI/API_includes.h"


/*
	Create a Profiling system that starts profiling in Begin_Profiling, profiles functions with Profile_Function, profiles scopes with Profile_Scope(name) and ends all profiling systems with End_Profiling!


*/

namespace TuranAPI {
	class TURANAPI Profiled_Scope {
	public:
		bool Is_Recording;
		long long START_POINT, END_POINT, THREAD_ID;
		string NAME;
		//Use this constructor to fill the data later!
		Profiled_Scope();
		//Use this constructor to start profiling a scope!
		//Timing Indexes: 0->Nanoseconds, 1->Microseconds, 2->Milliseconds, 3->Seconds
		Profiled_Scope(const char* name, bool StartImmediately, unsigned char timingindex);
		bool StartRecording(unsigned char timingindex);
		bool StopRecording(long long& duration);
		~Profiled_Scope();
	};



	class TURANAPI Profiler_System {
		void* START_POINT_PTR;
		vector<Profiled_Scope>* PROFILED_SCOPEs_vector;
	public:
		static Profiler_System* SELF;
		Profiler_System();
		~Profiler_System();

		void Save_a_ProfiledScope_toSession(const Profiled_Scope& PROFILED_SCOPE);
	};
}

#define TURAN_STOP_PROFILING() TuranAPI::Stop_Recording_Session()
//Profile in microseconds
#define TURAN_PROFILE_SCOPE_NAS(name) TuranAPI::Profiled_Scope ProfilingScope##__LINE__(name, true, 0)
#define TURAN_PROFILE_SCOPE_MCS(name) TuranAPI::Profiled_Scope ProfilingScope##__LINE__(name, true, 1)
#define TURAN_PROFILE_SCOPE_MLS(name) TuranAPI::Profiled_Scope ProfilingScope##__LINE__(name, true, 2)
#define TURAN_PROFILE_SCOPE_S(name) TuranAPI::Profiled_Scope ProfilingScope##__LINE__(name, true, 3)
#define TURAN_PROFILE_FUNCTION_NAS() TURAN_PROFILE_SCOPE_MCS(__FUNCSIG__, true, 0)
#define TURAN_PROFILE_FUNCTION_MCS() TURAN_PROFILE_SCOPE_MCS(__FUNCSIG__, true, 1)
#define TURAN_PROFILE_FUNCTION_MLS() TURAN_PROFILE_SCOPE_MCS(__FUNCSIG__, true, 2)
#define TURAN_PROFILE_FUNCTION_S() TURAN_PROFILE_SCOPE_MCS(__FUNCSIG__, true, 3)
