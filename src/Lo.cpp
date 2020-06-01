#include "plugin.hpp"

namespace Lo {

struct LoModule : Module {
	enum ParamIds {
		PARAM_ACTIVE,
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

    bool active = false;

	LoModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(PARAM_ACTIVE, 0.f, 1.f, 0.f, "Active");
		onReset();
		params[PARAM_ACTIVE].setValue(0.f);
	}
};


struct ActiveButton : TL1105 {
	LoModule* module;

	void onButton(const event::Button& e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			module->active ^= true;

			std::queue<Widget*> q;
			q.push(APP->scene->rack->moduleContainer);
			while (!q.empty()) {
				Widget* w = q.front();
				q.pop();

				ModuleWidget* sw = dynamic_cast<ModuleWidget*>(w);
				if (sw && sw->module != module) {
					sw->visible = !module->active;
				}

				for (Widget* w1 : w->children) {
					q.push(w1);
				}
			}

			for (Widget* w : APP->scene->rack->children) {
				if (w != APP->scene->rack->moduleContainer && w != APP->scene->rack->railFb) {
					w->visible = !module->active;
				}
			}
		}
		TL1105::onButton(e);
	}

	void draw(const DrawArgs& args) override {
		if (module->active) {
			std::queue<Widget*> q;
			q.push(APP->scene->rack->moduleContainer);
			while (!q.empty()) {
				Widget* w = q.front();
				q.pop();

				LightWidget* lw = dynamic_cast<LightWidget*>(w);
				if (lw) {
					nvgSave(args.vg);
					nvgResetScissor(args.vg);
					Vec p1 = lw->getRelativeOffset(Vec(), this);
					Vec p = getAbsoluteOffset(Vec()).neg();
					p = p.plus(p1);
					p = p.div(APP->scene->rackScroll->zoomWidget->zoom);
					nvgTranslate(args.vg, p.x, p.y);

					lw->draw(args);
					nvgRestore(args.vg);
				}

				for (Widget* w1 : w->children) {
					q.push(w1);
				}
			}
		}
		TL1105::draw(args);
	}
};

struct LoWidget : ModuleWidget {
	LoWidget(LoModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Lo.svg")));
		this->module = module;

		ActiveButton* activeButton = createParamCentered<ActiveButton>(Vec(15.f, 306.7f), module, LoModule::PARAM_ACTIVE);
		activeButton->module = module;
		addParam(activeButton);
	}
};

} // namespace Lo

Model* modelLo = createModel<Lo::LoModule, Lo::LoWidget>("Lo");