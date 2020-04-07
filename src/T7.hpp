#pragma once
#include "plugin.hpp"

namespace T7 {

struct T7MidiMessage {
	midi::Message msg;
	int driverId;
	int deviceId;
};

struct T7EventLogger {
	virtual ~T7EventLogger() {}
	virtual void log(std::string s) {}
};

struct T7Event {
	T7EventLogger* logger = NULL; // not owned
	virtual ~T7Event() {}
	virtual void execute() {}
	void log(std::string s) { if (logger) logger->log(s); }
};

struct T7Driver {
	struct PortDescriptor {
		int moduleId = -1;
		int portType = -1;
		int portId = -1;
	};

	virtual ~T7Driver() {}
	virtual std::string getName() { return ""; }
	virtual void toJson(json_t* driverJ) {}
	virtual void fromJson(json_t* driverJ) {}
	virtual void exampleJson(json_t* driverJ) {}
	virtual bool processMessage(midi::Message msg) { return false; }
	virtual T7Event* getEvent() { return NULL; }
};


struct T7CableToggleEvent : T7Event {
	T7Driver::PortDescriptor outPd;
	T7Driver::PortDescriptor inPd;
	bool replaceInputCable = false;

	T7CableToggleEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd);
	void execute() override;
};

} // namespace T7