#include "plugin.hpp"
#include "T7.hpp"

namespace T7 {

struct T7AssistantModule : Module {
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

	T7AssistantModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
	}
};


struct DebugDisplay : LedDisplayTextField {
	T7AssistantModule* module;
	int lastModuleId;
	int lastPortType;
	int lastPortId;

	void step() override {
		LedDisplayTextField::step();
		if (!module) return;

		lastModuleId = -1;
		lastPortType = -1;
		lastPortId = -1;

		Widget* w = APP->event->getSelectedWidget();
		if (!w) return;
		ModuleWidget* mw = w->getAncestorOfType<ModuleWidget>();
		if (!mw) return;
		Module* m = mw->module;
		if (!m || m == module) return;
		lastModuleId = m->id;

		PortWidget* pw = dynamic_cast<PortWidget*>(w);
		if (pw) {
			lastPortType = pw->type;
			lastPortId = pw->portId;
		}

		std::string t = "";
		t += string::f("moduleId:\n%i\n", lastModuleId);
		t += "input ports:\n";
		for (PortWidget* p : mw->getInputs()) {
			t += string::f("portId: %i", p->portId);
			if (p->portId == lastPortId && p->type == lastPortType) t += " clicked";
			t += "\n";
		}
		t += "output ports:\n";
		for (PortWidget* p : mw->getOutputs()) {
			t += string::f("portId: %i", p->portId);
			if (p->portId == lastPortId && p->type == lastPortType) t += " clicked";
			t += "\n";
		}

		this->text = t;
	}
};

struct T7AssistantWidget : ModuleWidget {
	T7AssistantModule* module;
	T7AssistantWidget(T7AssistantModule* module) {
		setModule(module);
		this->module = module;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/T7Assistant.svg")));

		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewBlack>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		
		DebugDisplay* textField2 = createWidget<DebugDisplay>(Vec(14.4f, 46.5f));
		textField2->module = module;
		textField2->box.size = Vec(150.4f, 277.5f);
		textField2->multiline = true;
		addChild(textField2);
	}
};

} // namespace T7

Model* modelT7Assistant = createModel<T7::T7AssistantModule, T7::T7AssistantWidget>("T7Assistant");