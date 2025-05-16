#include "plugin.hpp"
#include <patch.hpp>
#include <osdialog.h>


namespace Exit {

struct ExitModule : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		INPUT_LOAD,
		INPUT_LOADSAVE,
		INPUT_QUIT,
		NUM_INPUTS
	};
	enum OutputIds {
		NUM_OUTPUTS
	};
	enum LightIds {
		LIGHT_ACTIVE,
		NUM_LIGHTS
	};

	enum class WORK {
		NONE,
		LOAD,
		LOADSAVE,
		QUIT
	};

	std::string path = "";
	std::string workPath;
	WORK workToDo = WORK::NONE;

	dsp::SchmittTrigger loadTrigger;
	dsp::SchmittTrigger loadSaveTrigger;
	dsp::SchmittTrigger quitTrigger;

	ExitModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
	}

	void process(const ProcessArgs &args) override {
		if (inputs[INPUT_LOAD].isConnected() && loadTrigger.process(inputs[INPUT_LOAD].getVoltage())) {
			if (path != "") {
				trigger(WORK::LOAD, path);
			}
		}
		if (inputs[INPUT_LOADSAVE].isConnected() && loadSaveTrigger.process(inputs[INPUT_LOADSAVE].getVoltage())) {
			if (path != "") {
				trigger(WORK::LOADSAVE, path);
			}
		}
		if (inputs[INPUT_QUIT].isConnected() && quitTrigger.process(inputs[INPUT_QUIT].getVoltage())) {
			trigger(WORK::QUIT);
		}
	}

	void trigger(WORK workToDo, std::string workPath = "") {
		this->workPath = workPath;
		this->workToDo = workToDo;
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_t* pathJ = json_string(path.c_str());
		json_object_set_new(rootJ, "path", pathJ);
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t* pathJ = json_object_get(rootJ, "path");
		if (pathJ)
			path = json_string_value(pathJ);
		}
};


struct ExitDummyWidget : TransparentWidget {
	static ExitDummyWidget* instance;
	ExitModule* module = nullptr;

	void step() override {
		if (module != nullptr) {
			switch (module->workToDo) {
				case ExitModule::WORK::NONE:
					break;
				case ExitModule::WORK::LOAD:
					APP->patch->load(module->workPath);
					APP->patch->path = module->workPath;
					APP->history->setSaved();
					break;
				case ExitModule::WORK::LOADSAVE:
					APP->patch->save(APP->patch->path);
					APP->patch->load(module->workPath);
					APP->patch->path = module->workPath;
					APP->history->setSaved();
					break;
				case ExitModule::WORK::QUIT:
					APP->window->close();
					break;
			}
		}
		TransparentWidget::step();
	}
}; // struct ExitDummyWidget

ExitDummyWidget* ExitDummyWidget::instance = nullptr;


struct ExitWidget : ModuleWidget {
	const std::string PATCH_FILTERS = "VCV Rack patch (.vcv):vcv";
	ExitModule* module;
	bool active = false;

	ExitWidget(ExitModule* module) {
		setModule(module);
		this->module = module;
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Exit.svg")));

		addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(22.5f, 330.0f), module, ExitModule::LIGHT_ACTIVE));

		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 211.8f), module, ExitModule::INPUT_LOAD));
		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 255.4f), module, ExitModule::INPUT_LOADSAVE));
		addInput(createInputCentered<PJ301MPort>(Vec(22.5f, 298.9f), module, ExitModule::INPUT_QUIT));

		initExitDummyWidget();
		if (module) {
			active = registerSingleton("Exit", this);
			if (active) {
				ExitDummyWidget::instance->module = module;
			}
		}
	}

	~ExitWidget() {
		if (module && active) {
			unregisterSingleton("Exit", this);
			ExitDummyWidget::instance->module = nullptr;
		}
	}

	void step() override {
		if (module) {
			module->lights[ExitModule::LIGHT_ACTIVE].setBrightness(active);
		}
		ModuleWidget::step();
	}

	void selectFileDialog() {
		std::string dir;
		if (module->path.empty()) {
			dir = asset::user("patches");
			system::createDirectory(dir);
		}
		else {
			dir = system::getDirectory(module->path);
		}

		osdialog_filters* filters = osdialog_filters_parse(PATCH_FILTERS.c_str());
		DEFER({
			osdialog_filters_free(filters);
		});

		char* pathC = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, filters);
		if (!pathC) {
			// Fail silently
			return;
		}
		DEFER({
			std::free(pathC);
		});

		module->path = pathC;
	}

	void appendContextMenu(Menu* menu) override {
		menu->addChild(new MenuSeparator());

		struct SelectFileItem : MenuItem {
			ExitWidget* widget;
			void onAction(const event::Action &e) override {
                widget->selectFileDialog();
			}
		};

		menu->addChild(construct<SelectFileItem>(&MenuItem::text, "Select patch", &SelectFileItem::widget, this));

		if (module->path != "") {
			ui::MenuLabel* textLabel = new ui::MenuLabel;
			textLabel->text = "Currently selected...";
			menu->addChild(textLabel);

			ui::MenuLabel* modelLabel = new ui::MenuLabel;
			modelLabel->text = module->path;
			menu->addChild(modelLabel);
		}
	}

	void initExitDummyWidget() {
		if (ExitDummyWidget::instance == nullptr) {
			ui::SequentialLayout* layout = APP->scene->menuBar->getFirstDescendantOfType<ui::SequentialLayout>();
			ExitDummyWidget::instance = new ExitDummyWidget;
			layout->addChild(ExitDummyWidget::instance);
		}
	}
};

} // namespace Exit

Model* modelExit = createModel<Exit::ExitModule, Exit::ExitWidget>("Exit");