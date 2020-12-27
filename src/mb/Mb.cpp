#include "plugin.hpp"
#include "Mb.hpp"
#include "Mb_v1.hpp"
#include "Mb_v06.hpp"
#include <osdialog.h>
#include <chrono>

namespace Mb {

std::set<Model*> favoriteModels;
std::set<Model*> hiddenModels;
std::map<Model*, ModelUsage*> modelUsage;

// JSON storage

json_t* moduleBrowserToJson(bool includeUsageData) {
	json_t* rootJ = json_object();

	json_t* favoritesJ = json_array();
	for (Model* model : favoriteModels) {
		json_t* slugJ = json_object();
		json_object_set_new(slugJ, "plugin", json_string(model->plugin->slug.c_str()));
		json_object_set_new(slugJ, "model", json_string(model->slug.c_str()));
		json_array_append_new(favoritesJ, slugJ);
	}
	json_object_set_new(rootJ, "favorites", favoritesJ);

	json_t* hiddenJ = json_array();
	for (Model* model : hiddenModels) {
		json_t* slugJ = json_object();
		json_object_set_new(slugJ, "plugin", json_string(model->plugin->slug.c_str()));
		json_object_set_new(slugJ, "model", json_string(model->slug.c_str()));
		json_array_append_new(hiddenJ, slugJ);
	}
	json_object_set_new(rootJ, "hidden", hiddenJ);

	if (includeUsageData) {
		json_t* usageJ = json_array();
		for (auto t : modelUsage) {
			json_t* slugJ = json_object();
			json_object_set_new(slugJ, "plugin", json_string(t.first->plugin->slug.c_str()));
			json_object_set_new(slugJ, "model", json_string(t.first->slug.c_str()));
			json_object_set_new(slugJ, "usedCount", json_integer(t.second->usedCount));
			json_object_set_new(slugJ, "usedTimestamp", json_integer(t.second->usedTimestamp));
			json_array_append_new(usageJ, slugJ);
		}
		json_object_set_new(rootJ, "usage", usageJ);
	}

	return rootJ;
}

void moduleBrowserFromJson(json_t* rootJ) {
	json_t* favoritesJ = json_object_get(rootJ, "favorites");
	if (favoritesJ) {
		favoriteModels.clear();
		size_t i;
		json_t* slugJ;
		json_array_foreach(favoritesJ, i, slugJ) {
			json_t* pluginJ = json_object_get(slugJ, "plugin");
			json_t* modelJ = json_object_get(slugJ, "model");
			if (!pluginJ || !modelJ)
				continue;
			std::string pluginSlug = json_string_value(pluginJ);
			std::string modelSlug = json_string_value(modelJ);
			Model* model = plugin::getModel(pluginSlug, modelSlug);
			if (!model)
				continue;
			favoriteModels.insert(model);
		}
	}

	json_t* hiddenJ = json_object_get(rootJ, "hidden");
	if (hiddenJ) {
		hiddenModels.clear();
		size_t i;
		json_t* slugJ;
		json_array_foreach(hiddenJ, i, slugJ) {
			json_t* pluginJ = json_object_get(slugJ, "plugin");
			json_t* modelJ = json_object_get(slugJ, "model");
			if (!pluginJ || !modelJ)
				continue;
			std::string pluginSlug = json_string_value(pluginJ);
			std::string modelSlug = json_string_value(modelJ);
			Model* model = plugin::getModel(pluginSlug, modelSlug);
			if (!model)
				continue;
			hiddenModels.insert(model);
		}
	}

	json_t* usageJ = json_object_get(rootJ, "usage");
	if (usageJ) {
		for (auto t : modelUsage) {
			delete t.second;
		}
		modelUsage.clear();
		size_t i;
		json_t* slugJ;
		json_array_foreach(usageJ, i, slugJ) {
			json_t* pluginJ = json_object_get(slugJ, "plugin");
			json_t* modelJ = json_object_get(slugJ, "model");
			if (!pluginJ || !modelJ)
				continue;
			std::string pluginSlug = json_string_value(pluginJ);
			std::string modelSlug = json_string_value(modelJ);
			Model* model = plugin::getModel(pluginSlug, modelSlug);
			if (!model)
				continue;

			ModelUsage* m = new ModelUsage;
			m->usedCount = json_integer_value(json_object_get(slugJ, "usedCount"));
			m->usedTimestamp = json_integer_value(json_object_get(slugJ, "usedTimestamp"));
			modelUsage[model] = m;
		}
	}
}


// Usage data

void modelUsageTouch(Model* model) {
	ModelUsage* mu = modelUsage[model];
	if (!mu) {
		mu = new ModelUsage;
		modelUsage[model] = mu;
	}
	mu->usedCount++;
	mu->usedTimestamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void modelUsageReset() {
	for (auto t : modelUsage) {
		delete t.second;
	}
	modelUsage.clear();
}


// Export / Import

void exportSettings(std::string filename) {
	INFO("Saving settings %s", filename.c_str());

	json_t* rootJ = moduleBrowserToJson(false);

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


// Browser overlay

BrowserOverlay::BrowserOverlay() {
	v1::modelBoxZoom = pluginSettings.mbV1zoom;
	v1::modelBoxSort = pluginSettings.mbV1sort;
	v1::hideBrands = pluginSettings.mbV1hideBrands;
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

	if (parent) {
		APP->scene->removeChild(this);
	}
	
	pluginSettings.mbV1zoom = v1::modelBoxZoom;
	pluginSettings.mbV1sort = v1::modelBoxSort;
	pluginSettings.mbV1hideBrands = v1::hideBrands;
	json_decref(pluginSettings.mbModelsJ);
	pluginSettings.mbModelsJ = moduleBrowserToJson();
	
	pluginSettings.saveToJson();
}

void BrowserOverlay::step() {
	switch ((MODE)pluginSettings.mbMode) {
		case MODE::V06:
			if (visible) mbV06->show(); else mbV06->hide();
			mbV1->hide();
			break;
		case MODE::V1:
			mbV06->hide();
			if (visible) mbV1->show(); else mbV1->hide();
			break;
		default:
			break;
	}

	box = parent->box.zeroPos();
	// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
	if (visible) OpaqueWidget::step();
}

void BrowserOverlay::draw(const DrawArgs& args) {
	nvgBeginPath(args.vg);
	nvgRect(args.vg, RECT_ARGS(parent->box.zeroPos()));
	nvgFillColor(args.vg, nvgRGBA(0x0, 0x0, 0x0, 0xB0));
	nvgFill(args.vg);
	OpaqueWidget::draw(args);
}

void BrowserOverlay::onButton(const event::Button& e) {
	OpaqueWidget::onButton(e);
	if (e.getTarget() != this)
		return;

	if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
		hide();
		e.consume(this);
	}
}

BrowserOverlay* browserOverlay = NULL;

void init() {
	switch ((MODE)pluginSettings.mbMode) {
		case MODE::V06:
		case MODE::V1:
			if (!browserOverlay) {
				browserOverlay = new BrowserOverlay;
				browserOverlay->hide();
			}
			break;
		default:
			if (browserOverlay) {
				delete browserOverlay;
				browserOverlay = NULL;
			}
			break;
	}
}

} // namespace Mb