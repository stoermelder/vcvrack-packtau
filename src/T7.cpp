#include "plugin.hpp"
#include "T7.hpp"

namespace T7 {

T7CableToggleEvent::T7CableToggleEvent(T7Driver::PortDescriptor outPd, T7Driver::PortDescriptor inPd) {
	this->outPd = outPd;
	this->inPd = inPd;
}

void T7CableToggleEvent::execute() {
	ModuleWidget* outputModule = APP->scene->rack->getModule(outPd.moduleId);
	ModuleWidget* inputModule = APP->scene->rack->getModule(inPd.moduleId);
	if (!outputModule || !inputModule) return;

	// NB: unstable API from here on...
	// ---
	// Check if cable already exists
	CableWidget* cable = NULL;
	for (PortWidget* outPort : outputModule->outputs) {
		if (outPort->portId == outPd.portId) {
			for (CableWidget* cw : APP->scene->rack->getCablesOnPort(outPort)) {
				if (cw->inputPort->portId == inPd.portId) {
					// Cable found
					cable = cw;
					break;
				}
			}
			break;
		}
	}

	if (cable) {
		// Remove existing cable
		// history::CableRemove
		history::CableRemove* h = new history::CableRemove;
		h->setCable(cable);
		APP->history->push(h);

		APP->scene->rack->removeCable(cable);
		log("cable removed");
	}
	else {
		// Add new cable
		CableWidget* cw = new CableWidget;
		for (PortWidget* outPort : outputModule->outputs) {
			if (outPort->portId == outPd.portId) {
				// Output port found
				cw->setOutput(outPort);
				break;
			}
		}
		for (PortWidget* inPort : inputModule->inputs) {
			if (inPort->portId == inPd.portId) {
				// Input port found
				if (APP->scene->rack->getCablesOnPort(inPort).size() > 0) {
					// Input has a cable
					if (replaceInputCable) {
						CableWidget* cw1 = APP->scene->rack->getCablesOnPort(inPort).front();

						// history::CableRemove
						history::CableRemove* h = new history::CableRemove;
						h->setCable(cw1);
						APP->history->push(h);

						APP->scene->rack->removeCable(cw1);
					}
					else {
						log("input port occupied");
						break;
					}
				}
				cw->setInput(inPort);
				break;
			}
		}
		if (cw->isComplete()) {
			APP->scene->rack->addCable(cw);
			log("cable patched");

			// history::CableAdd
			history::CableAdd* h = new history::CableAdd;
			h->setCable(cw);
			APP->history->push(h);
		}
		else {
			delete cw;
			log("cable incomplete");
		}
	}
	// ---
}

} // namespace T7