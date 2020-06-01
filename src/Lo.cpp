#include "plugin.hpp"

namespace Lo {

struct LoModule : Module {
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
		LIGHT_ENABLED,
		NUM_LIGHTS
	};

    bool active = false;

	LoModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
	}
};


struct LoContainer : widget::Widget {
	LoModule* module;

	void draw(const DrawArgs& args) override {
		if (module && module->active) {
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
		Widget::draw(args);
	}

	void onHoverKey(const event::HoverKey& e) override {
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_X && (e.mods & RACK_MOD_MASK) == (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) {
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
				if (w != this) {
					w->visible = !module->active;
				}
			}
		}
		Widget::onHoverKey(e);
	}
};

struct LoWidget : ModuleWidget {
	LoContainer* loContainer;
	bool enabled = false;

	LoWidget(LoModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Lo.svg")));

		addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(15.f, 291.3f), module, LoModule::LIGHT_ENABLED));

		if (module) {enabled = registerSingleton("Lo", this);
			if (enabled) {
				loContainer = new LoContainer;
				loContainer->module = module;
				// This is where the magic happens: add a new widget on top-level to Rack
				APP->scene->rack->addChild(loContainer);
			}
		}
	}

	~LoWidget() {
		if (enabled && loContainer) {
			unregisterSingleton("Lo", this);
			APP->scene->rack->removeChild(loContainer);
			delete loContainer;
		}
	}

	void step() override {
		if (module) {
			module->lights[LoModule::LIGHT_ENABLED].setBrightness(enabled);
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		menu->addChild(new MenuSeparator());
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Hotkey Ctrl/Cmd+Alt+X"));
	}
};

} // namespace Lo

Model* modelLo = createModel<Lo::LoModule, Lo::LoWidget>("Lo");