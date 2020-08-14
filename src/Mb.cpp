#include "plugin.hpp"
#include "Mb.hpp"
#include "Mb_v1.hpp"
#include "Mb_v06.hpp"
#include <osdialog.h>

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

		struct ExportItem : MenuItem {
			MbWidget* mw;
			void onAction(const event::Action& e) override {
				mw->exportSettingsDialog();
			}
		};

		struct ImportItem : MenuItem {
			MbWidget* mw;
			void onAction(const event::Action& e) override {
				mw->importSettingsDialog();
			}
		};

		menu->addChild(new MenuSeparator());
		menu->addChild(construct<ModeV06Item>(&MenuItem::text, "v0.6", &ModeV06Item::module, module));
		menu->addChild(construct<ModeV1Item>(&MenuItem::text, "v1 mod", &ModeV1Item::module, module));
		menu->addChild(new MenuSeparator());
		menu->addChild(construct<ExportItem>(&MenuItem::text, "Export favorites/hidden", &ExportItem::mw, this));
		menu->addChild(construct<ImportItem>(&MenuItem::text, "Import favorites/hidden", &ImportItem::mw, this));
	}

	void exportSettings(std::string filename) {
		INFO("Saving settings %s", filename.c_str());

		json_t* rootJ = moduleBrowserToJson();

		DEFER({
			json_decref(rootJ);
		});

		FILE* file = fopen(filename.c_str(), "w");
		if (!file) {
			std::string message = string::f("Could not write to patch file %s", filename.c_str());
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
		}
		DEFER({
			fclose(file);
		});

		json_dumpf(rootJ, file, JSON_INDENT(2) | JSON_REAL_PRECISION(9));
	}

	void exportSettingsDialog() {
		osdialog_filters* filters = osdialog_filters_parse(":json");
		DEFER({
			osdialog_filters_free(filters);
		});

		char* path = osdialog_file(OSDIALOG_SAVE, "", "stoermelder-mb.json", filters);
		if (!path) {
			// No path selected
			return;
		}
		DEFER({
			free(path);
		});

		std::string pathStr = path;
		std::string extension = string::filenameExtension(string::filename(pathStr));
		if (extension.empty()) {
			pathStr += ".json";
		}

		exportSettings(pathStr);
	}

	void importSettings(std::string filename) {
		INFO("Loading settings %s", filename.c_str());

		FILE* file = fopen(filename.c_str(), "r");
		if (!file) {
			std::string message = string::f("Could not load file %s", filename.c_str());
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
			return;
		}
		DEFER({
			fclose(file);
		});

		json_error_t error;
		json_t* rootJ = json_loadf(file, 0, &error);
		if (!rootJ) {
			std::string message = string::f("File is not a valid file. JSON parsing error at %s %d:%d %s", error.source, error.line, error.column, error.text);
			osdialog_message(OSDIALOG_WARNING, OSDIALOG_OK, message.c_str());
			return;
		}
		DEFER({
			json_decref(rootJ);
		});

		moduleBrowserFromJson(rootJ);
	}

	void importSettingsDialog() {
		osdialog_filters* filters = osdialog_filters_parse(":json");
		DEFER({
			osdialog_filters_free(filters);
		});

		char* path = osdialog_file(OSDIALOG_OPEN, "", NULL, filters);
		if (!path) {
			// No path selected
			return;
		}
		DEFER({
			free(path);
		});

		importSettings(path);
	}
};

} // namespace Mb

Model *modelMb = createModel<Mb::MbModule, Mb::MbWidget>("Mb");