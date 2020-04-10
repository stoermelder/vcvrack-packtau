#include "plugin.hpp"
#include "T7.hpp"
#include "osdialog.h"


namespace T7 {

template < typename MODULE >
struct MidiCcTwoMessageToggle : T7Driver {
	struct MidiPortDescriptor : PortDescriptor {
		int midiType;
		int midiCcValue;
		std::string cableColor = "";
		std::string comment = "";
	};

	MODULE* module;

	std::map<std::tuple<int, int>, MidiPortDescriptor> portMap;
	MidiPortDescriptor lastInput;
	MidiPortDescriptor lastOutput;
	int lastEventType; // 0 = Trigger, 1 = Add, 2 = Remove

	std::string getName() override {
		return "MidiCcTwoMessage";
	}

	void exampleJson(json_t* driverJ) override {
		json_t* eventsJ = json_array();

		// Output
		json_t* eventJ = json_object();
		json_object_set_new(eventJ, "type", json_string("cable"));

		json_t* midiJ = json_object();
		json_object_set_new(midiJ, "channel", json_integer(0));
		json_object_set_new(midiJ, "cc", json_integer(11));
		json_object_set_new(midiJ, "ccValue", json_integer(127));
		json_object_set_new(midiJ, "type", json_string("gate"));
		json_object_set_new(eventJ, "midi", midiJ);

		json_t* targetJ = json_object();
		json_object_set_new(targetJ, "moduleId", json_integer(4));
		json_object_set_new(targetJ, "portType", json_string("output"));
		json_object_set_new(targetJ, "portId", json_integer(5));
		json_object_set_new(eventJ, "target", targetJ);

		json_object_set_new(eventJ, "cableColor", json_string(color::toHexString(color::YELLOW).c_str()));
		json_object_set_new(eventJ, "comment", json_string("comment for module 4 port 5"));
		json_array_append_new(eventsJ, eventJ);

		// Output
		eventJ = json_object();
		json_object_set_new(eventJ, "type", json_string("cable"));

		midiJ = json_object();
		json_object_set_new(midiJ, "channel", json_integer(0));
		json_object_set_new(midiJ, "cc", json_integer(12));
		json_object_set_new(midiJ, "ccValue", json_integer(127));
		json_object_set_new(midiJ, "type", json_string("trigger"));
		json_object_set_new(eventJ, "midi", midiJ);

		targetJ = json_object();
		json_object_set_new(targetJ, "moduleId", json_integer(3));
		json_object_set_new(targetJ, "portType", json_string("output"));
		json_object_set_new(targetJ, "portId", json_integer(3));
		json_object_set_new(eventJ, "target", targetJ);

		json_object_set_new(eventJ, "cableColor", json_string(color::toHexString(color::BLUE).c_str()));
		json_object_set_new(eventJ, "comment", json_string("comment for module port 3"));
		json_array_append_new(eventsJ, eventJ);

		// Input
		eventJ = json_object();
		json_object_set_new(eventJ, "type", json_string("cable"));

		midiJ = json_object();
		json_object_set_new(midiJ, "channel", json_integer(1));
		json_object_set_new(midiJ, "cc", json_integer(13));
		json_object_set_new(midiJ, "ccValue", json_integer(127));
		json_object_set_new(midiJ, "type", json_string("gate"));
		json_object_set_new(eventJ, "midi", midiJ);

		targetJ = json_object();
		json_object_set_new(targetJ, "moduleId", json_integer(2));
		json_object_set_new(targetJ, "portType", json_string("input"));
		json_object_set_new(targetJ, "portId", json_integer(3));
		json_object_set_new(eventJ, "target", targetJ);

		json_object_set_new(eventJ, "comment", json_string("comment for module 2 port 3"));
		json_array_append_new(eventsJ, eventJ);

		// Input
		eventJ = json_object();
		json_object_set_new(eventJ, "type", json_string("cable"));

		midiJ = json_object();
		json_object_set_new(midiJ, "channel", json_integer(1));
		json_object_set_new(midiJ, "cc", json_integer(13));
		json_object_set_new(midiJ, "ccValue", json_integer(127));
		json_object_set_new(midiJ, "type", json_string("trigger"));
		json_object_set_new(eventJ, "midi", midiJ);

		targetJ = json_object();
		json_object_set_new(targetJ, "moduleId", json_integer(2));
		json_object_set_new(targetJ, "portType", json_string("input"));
		json_object_set_new(targetJ, "portId", json_integer(4));
		json_object_set_new(eventJ, "target", targetJ);

		json_object_set_new(eventJ, "comment", json_string("comment for module 2 port 4"));
		json_array_append_new(eventsJ, eventJ);

		json_object_set_new(driverJ, "events", eventsJ);
	}

	void toJson(json_t* driverJ) override {
		json_t* eventsJ = json_array();
		for (auto p : portMap) {
			json_t* eventJ = json_object();
			json_object_set_new(eventJ, "type", json_string("cable"));

			json_t* midiJ = json_object();
			json_object_set_new(midiJ, "channel", json_integer(std::get<0>(p.first) + 1));
			json_object_set_new(midiJ, "cc", json_integer(std::get<1>(p.first)));
			json_object_set_new(midiJ, "ccValue", json_integer(p.second.midiCcValue));
			json_object_set_new(midiJ, "type", json_string(p.second.midiType == 0 ? "gate" : "trigger"));
			json_object_set_new(eventJ, "midi", midiJ);

			json_t* targetJ = json_object();
			json_object_set_new(targetJ, "moduleId", json_integer(p.second.moduleId));
			json_object_set_new(targetJ, "portType", json_string(p.second.portType == 0 ? "output" : "input"));
			json_object_set_new(targetJ, "portId", json_integer(p.second.portId));
			json_object_set_new(eventJ, "target", targetJ);

			if (p.second.portType == 0) json_object_set_new(eventJ, "cableColor", json_string(p.second.cableColor.c_str()));
			json_object_set_new(eventJ, "comment", json_string(p.second.comment.c_str()));
			json_array_append_new(eventsJ, eventJ);
		}
		json_object_set_new(driverJ, "events", eventsJ);
	}

	void fromJson(json_t* driverJ) override {
		reset();
		json_t* eventsJ = json_object_get(driverJ, "events");
		if (eventsJ) {
			json_t* eventJ;
			size_t eventIdx;
			json_array_foreach(eventsJ, eventIdx, eventJ) {
				json_t* typeJ = json_object_get(eventJ, "type");
				if (std::string(json_string_value(typeJ)) != "cable") continue;

				json_t* midiJ = json_object_get(eventJ, "midi");
				if (!midiJ) continue;
				json_t* midiChannelJ = json_object_get(midiJ, "channel");
				json_t* midiCcJ = json_object_get(midiJ, "cc");
				json_t* midiCcValueJ = json_object_get(midiJ, "ccValue");
				json_t* midiTypeJ = json_object_get(midiJ, "type");

				json_t* targetJ = json_object_get(eventJ, "target");
				if (!targetJ) continue;
				json_t* moduleIdJ = json_object_get(targetJ, "moduleId");
				json_t* portTypeJ = json_object_get(targetJ, "portType");
				json_t* portIdJ = json_object_get(targetJ, "portId");

				if (!midiChannelJ || !midiCcJ || !moduleIdJ || !midiTypeJ || !portTypeJ || !portIdJ) continue;
				json_t* cableColorJ = json_object_get(eventJ, "cableColor");
				json_t* commentJ = json_object_get(eventJ, "comment");

				int midiChannel = json_integer_value(midiChannelJ) - 1;
				int midiCc = json_integer_value(midiCcJ);

				MidiPortDescriptor pd;
				pd.midiType = std::string(json_string_value(midiTypeJ)) == "gate" ? 0 : 1;
				pd.midiCcValue = json_integer_value(midiCcValueJ);
				pd.moduleId = json_integer_value(moduleIdJ);
				pd.portType = std::string(json_string_value(portTypeJ)) == "output" ? 0 : 1;
				pd.portId = json_integer_value(portIdJ);
				pd.cableColor = cableColorJ ? json_string_value(cableColorJ) : "";
				pd.comment = commentJ ? json_string_value(commentJ) : "";

				auto t = std::make_tuple(midiChannel, midiCc);
				portMap[t] = pd;
			}
		}
	}

	void reset() override {
		portMap.clear();
	}

	bool processMessage(midi::Message msg) override {
		uint8_t ch = msg.getChannel();
		uint8_t cc = msg.getNote();
		int8_t value = msg.bytes[2];

		auto it = portMap.find(std::tuple<int, int>(ch, cc));
		if (it == portMap.end()) {
			it = portMap.find(std::tuple<int, int>(-1, cc));
		}

		auto h = [&](MidiPortDescriptor &pd, int lastEventType) {
			if (pd.portType == 0) this->lastEventType = lastEventType;
			if (pd.portType == 0) this->lastOutput = pd; // output
			if (pd.portType == 1) this->lastInput = pd; // input
		};

		if (it != portMap.end()) {
			MidiPortDescriptor pd;
			std::tie(std::ignore, pd) = *it;

			if (pd.midiType == 0) { // Gate
				if (value == 0) h(pd, 2); // Remove
				if (value >= pd.midiCcValue) h(pd, 1); // Add
			}

			if (pd.midiType == 1) { // Trigger
				if (value >= pd.midiCcValue) h(pd, 0); // Toggle
			}
		}

		return lastInput.moduleId >= 0 && lastOutput.moduleId >= 0;
	}

	T7Event* getEvent() override {
		if (lastInput.moduleId >= 0 && lastOutput.moduleId >= 0) {
			T7CableEvent* e = NULL;
			if (lastEventType == 0) e = new T7CableToggleEvent {lastOutput, lastInput};
			if (lastEventType == 1) e = new T7CableAddEvent {lastOutput, lastInput};
			if (lastEventType == 2) e = new T7CableRemoveEvent {lastOutput, lastInput};
			e->cableColor = lastOutput.cableColor;
			e->replaceInputCable = module->replaceInputCable;
			lastInput.moduleId = lastOutput.moduleId = -1;
			return e;
		}
		return NULL;
	}
};


struct T7CtrlModule : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	bool replaceInputCable;

	std::vector<T7Driver*> driver;

	dsp::RingBuffer<std::string, 256> debugMessagesEngine;
	dsp::RingBuffer<T7Event*, 16> events;

	struct EventLogger : T7EventLogger {
		dsp::RingBuffer<std::string, 256> debugMessagesGui;
		void log(std::string s) override {
			if (!debugMessagesGui.full()) debugMessagesGui.push(s);
		}
	};

	EventLogger eventLogger;

	T7CtrlModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();

		auto d1 = new MidiCcTwoMessageToggle<T7CtrlModule>();
		d1->module = this;
		driver.push_back(d1);
	}

	~T7CtrlModule() {
		for (T7Driver* d : driver) {
			delete d;
		}
	}

	void onReset() override {
		for (T7Driver* d : driver) {
			d->reset();
		}
		replaceInputCable = true;
		Module::onReset();
	}

	void process(const ProcessArgs& args) override {
		Module* mr = rightExpander.module;
		if (mr && mr->model->plugin->slug == "Stoermelder-PackTau" && mr->model->slug == "T7Midi") {
			std::vector<T7MidiMessage>* queue = reinterpret_cast<std::vector<T7MidiMessage>*>(mr->leftExpander.consumerMessage);
			for (T7MidiMessage m : *queue) {
				switch (m.msg.getStatus()) {
					// cc
					case 0xb:
						processCC(m.driverId, m.deviceId, m.msg);
						break;
					default:
						break;
				}
			}
			queue->clear();
		}
	}

	void processCC(int driverId, int deviceId, midi::Message msg) {
		uint8_t ch = msg.getChannel();
		uint8_t cc = msg.getNote();
		int8_t value = msg.bytes[2];

		std::string s = string::f("drv %i dev %i ch %i cc %i val %i", driverId, deviceId, ch + 1, cc, value);
		if (!debugMessagesEngine.full()) {
			debugMessagesEngine.push(s);
		}
		for (T7Driver* d : driver) {
			if (d->processMessage(msg)) {
				T7Event* e = d->getEvent();
				e->logger = &eventLogger;
				if (!events.full()) events.push(e);
			}
		}
	}

	json_t* driverMappingExampleJson() {
		json_t* driversJ = json_array();
		for (T7Driver* d : driver) {
			json_t* driverJ = json_object();
			std::string driverName = "";
			d->exampleJson(driverJ);
			driverName = d->getName();
			json_object_set_new(driverJ, "driverName", json_string(driverName.c_str()));
			json_array_append_new(driversJ, driverJ);
		}
		return driversJ;
	}

	json_t* driverMappingToJson() {
		json_t* driversJ = json_array();
		for (T7Driver* d : driver) {
			json_t* driverJ = json_object();
			std::string driverName = "";
			d->toJson(driverJ);
			driverName = d->getName();
			json_object_set_new(driverJ, "driverName", json_string(driverName.c_str()));
			json_array_append_new(driversJ, driverJ);
		}
		return driversJ;
	}

	void driverMappingFromJson(json_t* driversJ) {
		json_t* driverJ;
		size_t driverIdx;
		json_array_foreach(driversJ, driverIdx, driverJ) {
			json_t* driverNameJ = json_object_get(driverJ, "driverName");
			if (!driverNameJ) continue;
			std::string driverName = json_string_value(driverNameJ);
			for (T7Driver* d : driver) {
				if (d->getName() == driverName) {
					d->fromJson(driverJ);
				}
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "replaceInputCable", json_boolean(replaceInputCable));

		json_t* driversJ = driverMappingToJson();
		json_object_set_new(rootJ, "driver", driversJ);
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		replaceInputCable = json_boolean_value(json_object_get(rootJ, "replaceInputCable"));

		json_t* driverJ = json_object_get(rootJ, "driver");
		if (driverJ) driverMappingFromJson(driverJ);
	}
};


struct MidiDisplay : LedDisplayTextField {
	T7CtrlModule* module;
	const int MAX = 600;

	void step() override {
		LedDisplayTextField::step();
		if (!module) return;

		while (!module->debugMessagesEngine.empty()) {
			std::string s = module->debugMessagesEngine.shift();
			text = s + "\n" + text.substr(0, MAX);
		}
		while (!module->eventLogger.debugMessagesGui.empty()) {
			std::string s = module->eventLogger.debugMessagesGui.shift();
			text = "# " + s + " #\n" + text.substr(0, MAX);
		}
	}
};


struct T7CtrlWidget : ModuleWidget {
	T7CtrlModule* module;
	T7CtrlWidget(T7CtrlModule* module) {
		setModule(module);
		this->module = module;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/T7Ctrl.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		MidiDisplay* textField1 = createWidget<MidiDisplay>(Vec(14.4f, 46.5f));
		textField1->module = module;
		textField1->box.size = Vec(211.1f, 277.5f);
		textField1->multiline = true;
		addChild(textField1);
	}

	void appendContextMenu(Menu* menu) override {
		struct ExampleMappingItem : MenuItem {
			T7CtrlWidget* mw;
			void onAction(const event::Action& e) override {
				mw->exampleMapping();
			}
		};
		struct CopyMappingItem : MenuItem {
			T7CtrlWidget* mw;
			void onAction(const event::Action& e) override {
				mw->copyMapping();
			}
		};
		struct PasteMappingItem : MenuItem {
			T7CtrlWidget* mw;
			void onAction(const event::Action& e) override {
				mw->pasteMapping();
			}
		};

		struct ReplaceCableItem : MenuItem {
			T7CtrlModule* module;
			void onAction(const event::Action& e) override {
				module->replaceInputCable ^= true;
			}
			void step() override {
				rightText = module->replaceInputCable ? "✔" : "";
				MenuItem::step();
			}
		};

		menu->addChild(new MenuSeparator());
		menu->addChild(construct<ExampleMappingItem>(&MenuItem::text, "Example JSON mapping", &ExampleMappingItem::mw, this));
		menu->addChild(construct<CopyMappingItem>(&MenuItem::text, "Copy JSON mapping", &CopyMappingItem::mw, this));
		menu->addChild(construct<PasteMappingItem>(&MenuItem::text, "Paste JSON mapping", &PasteMappingItem::mw, this));
		menu->addChild(new MenuSeparator());
		menu->addChild(construct<ReplaceCableItem>(&MenuItem::text, "Replace input cables", &ReplaceCableItem::module, module));
	}
	
	void exampleMapping() {
		json_t* driverJ = module->driverMappingExampleJson();
		DEFER({
			json_decref(driverJ);
		});
		char* mappingJson = json_dumps(driverJ, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		DEFER({
			free(mappingJson);
		});
		glfwSetClipboardString(APP->window->win, mappingJson);
	}

	void copyMapping() {
		json_t* driverJ = module->driverMappingToJson();
		DEFER({
			json_decref(driverJ);
		});
		char* mappingJson = json_dumps(driverJ, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
		DEFER({
			free(mappingJson);
		});
		glfwSetClipboardString(APP->window->win, mappingJson);
	}

	void pasteMapping() {
		const char* clipboard = glfwGetClipboardString(APP->window->win);
		if (!clipboard) {
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, "Could not get text from clipboard.");
			return;
		}

		json_error_t error;
		json_t* driverJ = json_loads(clipboard, 0, &error);
		if (!driverJ) {
			std::string message = string::f("JSON parsing error at %s %d:%d %s", error.source, error.line, error.column, error.text);
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
			return;
		}
		DEFER({
			json_decref(driverJ);
		});

		module->driverMappingFromJson(driverJ);
	}

	void step() override {
		ModuleWidget::step();
		if (!module) return;

		while (module->events.size() > 0) {
			T7Event* e = module->events.shift();
			e->execute();
			delete e;
		}
	}
};

} // namespace T7

Model* modelT7Ctrl = createModel<T7::T7CtrlModule, T7::T7CtrlWidget>("T7Ctrl");