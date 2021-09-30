#include "plugin.hpp"

namespace Mx {

struct MxModule : Module {
	enum ParamIds {
		PARAM_ACTIVE,
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_X,
		INPUT_Y,
		INPUT_LCLK,
		INPUT_MCLK,
		INPUT_RCLK,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_ACTIVE,
		NUM_LIGHTS
	};

	bool active = false;
	bool leftClickPress = false;
	bool leftClickRelease = false;
	bool middleClickPress = false;
	bool middleClickRelease = false;
	bool rightClickPress = false;
	bool rightClickRelease = false;

	dsp::BooleanTrigger leftClickPressTrigger;
	dsp::BooleanTrigger leftClickReleaseTrigger;
	dsp::BooleanTrigger middleClickPressTrigger;
	dsp::BooleanTrigger middleClickReleaseTrigger;
	dsp::BooleanTrigger rightClickPressTrigger;
	dsp::BooleanTrigger rightClickReleaseTrigger;

	MxModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PARAM_ACTIVE, 0.f, 1.f, 0.f, "Active");
		onReset();
		params[PARAM_ACTIVE].setValue(0.f);
	}

	void process(const ProcessArgs& args) override {
		bool lclk = inputs[INPUT_LCLK].getVoltage() > 0.f;
		if (leftClickPressTrigger.process(lclk)) {
			leftClickPress = true;
		}
		if (leftClickReleaseTrigger.process(!lclk)) {
			leftClickRelease = true;
		}
		bool mclk = inputs[INPUT_MCLK].getVoltage() > 0.f;
		if (middleClickPressTrigger.process(mclk)) {
			middleClickPress = true;
		}
		if (middleClickReleaseTrigger.process(!mclk)) {
			middleClickRelease = true;
		}
		bool rclk = inputs[INPUT_RCLK].getVoltage() > 0.f;
		if (rightClickPressTrigger.process(rclk)) {
			rightClickPress = true;
		}
		if (rightClickReleaseTrigger.process(!rclk)) {
			rightClickRelease = true;
		}
	}
};


struct ActiveButton : TL1105 {
	MxModule* module;
	void step() override {
		TL1105::step();
		if (module) {
			module->lights[MxModule::LIGHT_ACTIVE].setBrightness(module->active);
		}
	}
	void onButton(const event::Button& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			module->active ^= true;
		}
		TL1105::onButton(e);
	}
};

struct MxWidget : ModuleWidget {
	MxModule* module;
	static constexpr float inf = std::numeric_limits<float>::infinity();
	float lastX = inf;
	float lastY = inf;

	MxWidget(MxModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mx.svg")));
		this->module = module;

		addInput(createInputCentered<PJ301MPort>(Vec(15.f, 98.0f), module, MxModule::INPUT_LCLK));
		addInput(createInputCentered<PJ301MPort>(Vec(15.f, 137.0f), module, MxModule::INPUT_MCLK));
		addInput(createInputCentered<PJ301MPort>(Vec(15.f, 176.0f), module, MxModule::INPUT_RCLK));
		addInput(createInputCentered<PJ301MPort>(Vec(15.f, 214.9f), module, MxModule::INPUT_X));
		addInput(createInputCentered<PJ301MPort>(Vec(15.f, 253.8f), module, MxModule::INPUT_Y));

		addChild(createLightCentered<TinyLight<YellowLight>>(Vec(15.f, 291.3f), module, MxModule::LIGHT_ACTIVE));
		ActiveButton* activeButton = createParamCentered<ActiveButton>(Vec(15.f, 306.7f), module, MxModule::PARAM_ACTIVE);
		activeButton->module = module;
		addParam(activeButton);
	}

	void step() override {
		ModuleWidget::step();
		if (!module || !module->active) return;

		float x = inf;
		if (module->inputs[MxModule::INPUT_X].isConnected()) {
			x = module->inputs[MxModule::INPUT_X].getVoltage() / 10.f;
		}
		float y = inf;
		if (module->inputs[MxModule::INPUT_Y].isConnected()) {
			y = module->inputs[MxModule::INPUT_Y].getVoltage() / 10.f;
		}

		if ((x <= 1.f && x != lastX) || (y <= 1.f && y != lastY)) {
			int winWidth, winHeight;
			glfwGetWindowSize(APP->window->win, &winWidth, &winHeight);

			if (x != lastX) {
				lastX = x;
				x *= winWidth;
			}
			else {
				x = lastX * winWidth;
			}
			if (y != lastY) {
				lastY = y;
				y *= winHeight;
			}
			else {
				y = lastY * winHeight;
			}
			glfwSetCursorPos(APP->window->win, x, y);
		}

		if (module->leftClickPress) {
			module->leftClickPress = false;
			APP->event->handleButton(APP->scene->getMousePos(),  GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
		}
		if (module->leftClickRelease) {
			module->leftClickRelease = false;
			APP->event->handleButton(APP->scene->getMousePos(),  GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE, 0);
		}
		if (module->middleClickPress) {
			module->middleClickPress = false;
			APP->event->handleButton(APP->scene->getMousePos(),  GLFW_MOUSE_BUTTON_MIDDLE, GLFW_PRESS, 0);
		}
		if (module->middleClickRelease) {
			module->middleClickRelease = false;
			APP->event->handleButton(APP->scene->getMousePos(),  GLFW_MOUSE_BUTTON_MIDDLE, GLFW_RELEASE, 0);
		}
		if (module->rightClickPress) {
			module->rightClickPress = false;
			APP->event->handleButton(APP->scene->getMousePos(),  GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS, 0);
		}
		if (module->rightClickRelease) {
			module->rightClickRelease = false;
			APP->event->handleButton(APP->scene->getMousePos(),  GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE, 0);
		}
	}
};

} // namespace Mx

Model* modelMx = createModel<Mx::MxModule, Mx::MxWidget>("Mx");