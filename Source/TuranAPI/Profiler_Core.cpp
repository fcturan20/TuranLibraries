#include "Profiler_Core.h"
#include "Logger_Core.h"
#include <flatbuffers/flatbuffers.h>

namespace TuranAPI {

	//CODE ALL OF THESE!
	Profiled_Scope::Profiled_Scope() {}
	Profiled_Scope::Profiled_Scope(const char* name, bool StartImmediately, unsigned char timingindex) : NAME(name) {
		if (StartImmediately) {
			StartRecording(timingindex);
		}
	}
	bool Profiled_Scope::StartRecording(unsigned char timingindex) {
		switch (timingindex) {
		case 0:
			START_POINT = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		case 1:
			START_POINT = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		case 2:
			START_POINT = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		case 3:
			START_POINT = std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
			break;
		default:
			LOG_CRASHING_TAPI("You should specify timing between 0-3, not anything else!");
			return false;
		}
		THREAD_ID = std::hash<std::thread::id>{}(std::this_thread::get_id());
		Is_Recording = true;
		return true;
	}
	bool Profiled_Scope::StopRecording(long long& duration) {
		if (!Is_Recording) {
			return false;
		}
		END_POINT = std::chrono::time_point_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now()).time_since_epoch().count();
		duration = END_POINT - START_POINT;
		return true;
		/*
		std::cout << "A Profiled scope is saved!" << std::endl;
		std::cout << "Scope Name:" << NAME << std::endl;
		std::cout << "Scope Start Point: " << START_POINT << std::endl;
		std::cout << "Scope End Point: " << END_POINT << std::endl;
		std::cout << "Scope Thread ID: " << THREAD_ID << std::endl;*/
	}
	Profiled_Scope::~Profiled_Scope() {
		if (Is_Recording) {
			long long Duration;
			StopRecording(Duration);
			std::cout << NAME << " Scope Duration: " << Duration << " microseconds (1/1000 milliseconds)" << std::endl;
		}
	}

	Profiler_System* Profiler_System::SELF = nullptr;
	Profiler_System::Profiler_System() {
		SELF = this;

		START_POINT_PTR = new std::chrono::time_point<std::chrono::steady_clock>;
		*((std::chrono::time_point<std::chrono::steady_clock>*)START_POINT_PTR) = std::chrono::high_resolution_clock::now();

		PROFILED_SCOPEs_vector = new vector<Profiled_Scope>;
	}
	Profiler_System::~Profiler_System() {

	}

	void Profiler_System::Save_a_ProfiledScope_toSession(const Profiled_Scope& PROFILED_SCOPE) {

	}
}