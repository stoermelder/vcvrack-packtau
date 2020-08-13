#pragma once
#include "plugin.hpp"
#include <plugin.hpp>

namespace Mb {

static std::set<Model*> favoriteModels;
static std::set<Model*> hiddenModels;

enum class MODE {
	V06,
	V1
};

struct BrowserOverlay : widget::OpaqueWidget {
	Widget* mbWidgetBackup;
	Widget* mbV06;
	Widget* mbV1;

	MODE* mode;

	BrowserOverlay();
	~BrowserOverlay();

	void step() override {
		switch (*mode) {
			case MODE::V06:
				if (visible) mbV06->show(); else mbV06->hide();
				mbV1->hide();
				break;
			case MODE::V1:
				mbV06->hide();
				if (visible) mbV1->show(); else mbV1->hide();
				break;
		}

		box = parent->box.zeroPos();
		// Only step if visible, since there are potentially thousands of descendants that don't need to be stepped.
		if (visible) OpaqueWidget::step();
	}

	void onButton(const event::Button& e) override {
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;

		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			hide();
			e.consume(this);
		}
	}
};

json_t* moduleBrowserToJson() {
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

	return rootJ;
}

void moduleBrowserFromJson(json_t* rootJ) {
	json_t* favoritesJ = json_object_get(rootJ, "favorites");
	if (favoritesJ) {
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
}

} // namespace Mb