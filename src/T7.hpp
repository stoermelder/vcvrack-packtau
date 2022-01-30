#pragma once
#include "plugin.hpp"

namespace T7 {

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


enum class T7MessageType {
	MIDI = 1
};

struct T7Message {
	T7MessageType type;
};

struct T7MidiMessage : T7Message {
	midi::Message msg;
	int driverId;
	int deviceId;
	T7MidiMessage() { type = T7MessageType::MIDI; }
};


struct T7Driver {
	struct PortDescriptor {
		int64_t moduleId = -1;
		int portType = -1;
		int portId = -1;
	};

	virtual ~T7Driver() {}
	virtual std::string getName() { return ""; }
	virtual void toJson(json_t* driverJ) {}
	virtual void fromJson(json_t* driverJ) {}
	virtual void exampleJson(json_t* driverJ) {}
	virtual void reset() {}
	virtual bool processMessage(midi::Message msg) { return false; }
	virtual T7Event* getEvent() { return NULL; }
};


struct T7CableEvent : T7Event {
	T7Driver::PortDescriptor outPd;
	T7Driver::PortDescriptor inPd;
	std::string cableColor = "";
	bool replaceInputCable = false;
	T7CableEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd);
	void removeCable(CableWidget* cw);
	void addCable(CableWidget* cw);
	CableWidget* findCable(ModuleWidget* outputModule, ModuleWidget* inputModule);
};

struct T7CableToggleEvent : T7CableEvent {
	T7CableToggleEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd);
	void execute() override;
};

struct T7CableAddEvent : T7CableEvent {
	T7CableAddEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd);
	void execute() override;
};

struct T7CableRemoveEvent : T7CableEvent {
	T7CableRemoveEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd);
	void execute() override;
};

} // namespace T7