#include "plugin.hpp"
#include "T7.hpp"

namespace T7 {

// T7CableEvent

T7CableEvent::T7CableEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd) {
	this->outPd = outPd;
	this->inPd = inPd;
}

void T7CableEvent::removeCable(CableWidget* cw) {
	// history::CableRemove
	history::CableRemove* h = new history::CableRemove;
	h->setCable(cw);
	APP->history->push(h);
	APP->scene->rack->removeCable(cw);
	delete cw;
}

void T7CableEvent::addCable(CableWidget* cw) {
	APP->scene->rack->addCable(cw);
	// history::CableAdd
	history::CableAdd* h = new history::CableAdd;
	h->setCable(cw);
	APP->history->push(h);
}

CableWidget* T7CableEvent::findCable(ModuleWidget* outputModule, ModuleWidget* inputModule) {
	for (PortWidget* outPort : outputModule->getOutputs()) {
		if (outPort->portId == outPd.portId) {
			for (CableWidget* cw : APP->scene->rack->getCablesOnPort(outPort)) {
				if (cw->inputPort->portId == inPd.portId) {
					// Cable found
					return cw;
				}
			}
			break;
		}
	}
	return NULL;
}


// T7CableToggleEvent

T7CableToggleEvent::T7CableToggleEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd)
	: T7CableEvent(outPd, inPd) {
}

void T7CableToggleEvent::execute() {
	ModuleWidget* outputModule = APP->scene->rack->getModule(outPd.moduleId);
	ModuleWidget* inputModule = APP->scene->rack->getModule(inPd.moduleId);
	if (!outputModule || !inputModule) return;

	// NB: unstable API from here on...
	// ---
	// Check if cable already exists
	CableWidget* cable = findCable(outputModule, inputModule);
	if (cable) {
		// Remove existing cable
		removeCable(cable);
		log("cable removed");
	}
	else {
		// Add new cable
		engine::Cable* c = new engine::Cable;
		c->outputId = outPd.portId;
		c->outputModule = outputModule->module;
		c->inputId = inPd.portId;
		c->inputModule = inputModule->module;

		for (PortWidget* inPort : inputModule->getInputs()) {
			if (inPort->portId == inPd.portId) {
				// Input port found
				if (APP->scene->rack->getCablesOnPort(inPort).size() > 0) {
					// Input has a cable
					if (replaceInputCable) {
						CableWidget* cw1 = APP->scene->rack->getCablesOnPort(inPort).front();
						removeCable(cw1);
					}
					else {
						log("input port occupied");
						break;
					}
				}
				break;
			}
		}

		APP->engine->addCable(c);

		CableWidget* cw = new CableWidget;
		cw->setCable(c);
		if (cableColor != "") {
			cw->color = color::fromHexString(cableColor);
		}
		addCable(cw);
		log("cable patched");

		// TODO: incomplete cables?
		// log("cable incomplete");
	}
	// ---
}


// T7CableAddEvent

T7CableAddEvent::T7CableAddEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd)
	: T7CableEvent(outPd, inPd) {
}

void T7CableAddEvent::execute() {
	ModuleWidget* outputModule = APP->scene->rack->getModule(outPd.moduleId);
	ModuleWidget* inputModule = APP->scene->rack->getModule(inPd.moduleId);
	if (!outputModule || !inputModule) return;

	// NB: unstable API from here on...
	// ---
	// Check if cable already exists
	CableWidget* cable = findCable(outputModule, inputModule);
	if (cable) {
		log("cable already patched");
		return;
	}
	else {
		// Add new cable
		engine::Cable* c = new engine::Cable;
		c->outputId = outPd.portId;
		c->outputModule = outputModule->module;
		c->inputId = inPd.portId;
		c->inputModule = inputModule->module;

		for (PortWidget* inPort : inputModule->getInputs()) {
			if (inPort->portId == inPd.portId) {
				// Input port found
				if (APP->scene->rack->getCablesOnPort(inPort).size() > 0) {
					// Input has a cable
					if (replaceInputCable) {
						CableWidget* cw1 = APP->scene->rack->getCablesOnPort(inPort).front();
						removeCable(cw1);
					}
					else {
						log("input port occupied");
						break;
					}
				}
				break;
			}
		}

		APP->engine->addCable(c);

		CableWidget* cw = new CableWidget;
		cw->setCable(c);
		if (cableColor != "") {
			cw->color = color::fromHexString(cableColor);
		}
		addCable(cw);
		log("cable patched");

		// TODO: incomplete cables?
		// log("cable incomplete");
	}
	// ---
}


// T7CableRemoveEvent

T7CableRemoveEvent::T7CableRemoveEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd)
	: T7CableEvent(outPd, inPd) {
}

void T7CableRemoveEvent::execute() {
	ModuleWidget* outputModule = APP->scene->rack->getModule(outPd.moduleId);
	ModuleWidget* inputModule = APP->scene->rack->getModule(inPd.moduleId);
	if (!outputModule || !inputModule) return;

	// NB: unstable API from here on...
	// ---
	// Check if cable already exists
	CableWidget* cable = findCable(outputModule, inputModule);
	if (cable) {
		// Remove existing cable
		removeCable(cable);
		log("cable removed");
	}
	else {
		log("no cable to remove");
	}
	// ---
}

} // namespace T7