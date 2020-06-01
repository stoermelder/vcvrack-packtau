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

	/** [Stored to JSON] */
	float dim = 0.8f;

	LoModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
	}

	json_t* dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "dim", json_real(dim));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		dim = json_real_value(json_object_get(rootJ, "dim"));
	}
};


struct LoContainer : widget::Widget {
	LoModule* module;

	void draw(const DrawArgs& args) override {
		if (module && module->active) {
			// Dim layer
			box = parent->box.zeroPos();
			nvgBeginPath(args.vg);
			nvgRect(args.vg, 0, 0, box.size.x, box.size.y);
			nvgFillColor(args.vg, nvgRGBA(0x00, 0x00, 0x00, (char)(255.f * module->dim)));
			nvgFill(args.vg);

			// Draw lights
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

			// Draw cable plugs
			for (widget::Widget* w : APP->scene->rack->cableContainer->children) {
				CableWidget* cw = dynamic_cast<CableWidget*>(w);
				assert(cw);
				cw->drawPlugs(args);
			}
		}
		Widget::draw(args);
	}
	
	void onHoverKey(const event::HoverKey& e) override {
		if (e.action == GLFW_PRESS && e.key == GLFW_KEY_X && (e.mods & RACK_MOD_MASK) == (GLFW_MOD_CONTROL | GLFW_MOD_ALT)) {
			module->active ^= true;

			/*
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
			*/
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
		LoModule* module = dynamic_cast<LoModule*>(this->module);

		struct DimSlider : ui::Slider {
			struct DimQuantity : Quantity {
				LoModule* module;
				DimQuantity(LoModule* module) {
					this->module = module;
				}
				void setValue(float value) override {
					module->dim = math::clamp(value, 0.f, 1.f);
				}
				float getValue() override {
					return module->dim;
				}
				float getDefaultValue() override {
					return 0.8f;
				}
				float getDisplayValue() override {
					return getValue() * 100;
				}
				void setDisplayValue(float displayValue) override {
					setValue(displayValue / 100);
				}
				std::string getLabel() override {
					return "Dim";
				}
				std::string getUnit() override {
					return "%";
				}
				float getMaxValue() override {
					return 1.f;
				}
				float getMinValue() override {
					return 0.f;
				}
			};

			DimSlider(LoModule* module) {
				box.size.x = 180.0f;
				quantity = new DimQuantity(module);
			}
			~DimSlider() {
				delete quantity;
			}
		};

		menu->addChild(new MenuSeparator());
		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Hotkey Ctrl/Cmd+Alt+X"));
		menu->addChild(new DimSlider(module));
	}
};

} // namespace Lo

Model* modelLo = createModel<Lo::LoModule, Lo::LoWidget>("Lo");