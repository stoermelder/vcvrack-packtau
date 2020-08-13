#include "plugin.hpp"
#include "Mb.hpp"
#include "Mb_v1.hpp"
#include "Mb_v06.hpp"

namespace Mb {


BrowserOverlay::BrowserOverlay() {
	v1::modelBoxZoom = pluginSettings.mbV1zoom;
	moduleBrowserFromJson(pluginSettings.mbModelsJ);

	mbWidgetBackup = APP->scene->moduleBrowser;
	mbWidgetBackup->hide();
	APP->scene->removeChild(mbWidgetBackup);

	mbV06 = new v06::ModuleBrowser;
	addChild(mbV06);

	mbV1 = new v1::ModuleBrowser;
	addChild(mbV1);

	APP->scene->moduleBrowser = this;
	APP->scene->addChild(this);
}

BrowserOverlay::~BrowserOverlay() {
	APP->scene->moduleBrowser = mbWidgetBackup;
	APP->scene->addChild(mbWidgetBackup);

	APP->scene->removeChild(this);

	pluginSettings.mbV1zoom = v1::modelBoxZoom;
	json_decref(pluginSettings.mbModelsJ);
	pluginSettings.mbModelsJ = moduleBrowserToJson();
	
	pluginSettings.saveToJson();
}



struct MbModule : Module {
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
		LIGHT_ACTIVE,
		NUM_LIGHTS
	};

	MbModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		onReset();
	}

	MODE mode = MODE::V1;

	json_t* dataToJson() override {
		json_t *rootJ = json_object();
		json_object_set_new(rootJ, "mode", json_integer((int)mode));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		mode = (MODE)json_integer_value(json_object_get(rootJ, "mode"));
	}
};


struct MbWidget : ModuleWidget {
	BrowserOverlay* browserOverlay;
	bool active = false;

	MbWidget(MbModule* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Mb.svg")));

		addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(15.f, 330.0f), module, MbModule::LIGHT_ACTIVE));

		if (module) {
			active = registerSingleton("Mb", this);
			if (active) {
				browserOverlay = new BrowserOverlay;
				browserOverlay->mode = &module->mode;
				browserOverlay->hide();
			}
		}
	}

	~MbWidget() {
		if (module && active) {
			unregisterSingleton("Mb", this);
			delete browserOverlay;
		}
	}

	void step() override {
		if (module) {
			module->lights[MbModule::LIGHT_ACTIVE].setBrightness(active);
		}
		ModuleWidget::step();
	}

	void appendContextMenu(Menu* menu) override {
		MbModule* module = dynamic_cast<MbModule*>(this->module);

		struct ModeV06Item : MenuItem {
			MbModule* module;
			void onAction(const event::Action& e) override {
				module->mode = MODE::V06;
			}
			void step() override {
				rightText = module->mode == MODE::V06 ? "✔" : "";
				MenuItem::step();
			}
		};

		struct ModeV1Item : MenuItem {
			MbModule* module;
			void onAction(const event::Action& e) override {
				module->mode = MODE::V1;
			}
			void step() override {
				rightText = module->mode == MODE::V1 ? "✔" : "";
				MenuItem::step();
			}

			Menu* createChildMenu() override {
				Menu* menu = new Menu;
				menu->addChild(new v1::ModelZoomSlider);
				return menu;
			}
		};

		menu->addChild(new MenuSeparator());
		menu->addChild(construct<ModeV06Item>(&MenuItem::text, "v0.6", &ModeV06Item::module, module));
		menu->addChild(construct<ModeV1Item>(&MenuItem::text, "v1 mod", &ModeV1Item::module, module));
	}
};

} // namespace Mb

Model *modelMb = createModel<Mb::MbModule, Mb::MbWidget>("Mb");