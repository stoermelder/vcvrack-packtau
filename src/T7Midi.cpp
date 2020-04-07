#include "plugin.hpp"
#include "T7.hpp"

namespace T7 {

struct T7MidiModule : Module {
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

	midi::InputQueue midiInput;

	std::vector<T7MidiMessage> consumerMessage;
	std::vector<T7MidiMessage> producerMessage;

	T7MidiModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
		leftExpander.consumerMessage = &consumerMessage;
		leftExpander.producerMessage = &producerMessage;
	}

	void onReset() override {
		midiInput.reset();
	}

	void process(const ProcessArgs& args) override {
		std::vector<T7MidiMessage>* queue = reinterpret_cast<std::vector<T7MidiMessage>*>(leftExpander.producerMessage);

		midi::Message msg;
		while (midiInput.shift(&msg)) {
			T7MidiMessage m;
			m.driverId = midiInput.driverId;
			m.deviceId = midiInput.deviceId;
			m.msg = msg;
			queue->push_back(m);
		}

		Module* mr = rightExpander.module;
		if (mr && mr->model->plugin->slug == "Stoermelder-PackTau" && mr->model->slug == "T7Midi") {
			std::vector<T7MidiMessage>* queue1 = reinterpret_cast<std::vector<T7MidiMessage>*>(mr->leftExpander.consumerMessage);
			for (T7MidiMessage m : *queue1) queue->push_back(m);
			queue1->clear();
		}

		if (queue->size() > 0) {
			leftExpander.messageFlipRequested = true;
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "midi", midiInput.toJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* midiJ = json_object_get(rootJ, "midi");
		if (midiJ) midiInput.fromJson(midiJ);
	}
};


struct T7MidiWidget : ModuleWidget {
	T7MidiWidget(T7MidiModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/T7Midi.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		MidiWidget* midiWidget = createWidget<MidiWidget>(Vec(14.0f, 46.5f));
		midiWidget->box.size = Vec(122.0f, 83.0f);
		midiWidget->setMidiPort(module ? &module->midiInput : NULL);
		addChild(midiWidget);
	}
};

} // namespace T7

Model* modelT7Midi = createModel<T7::T7MidiModule, T7::T7MidiWidget>("T7Midi");
