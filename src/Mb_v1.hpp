#include "Mb.hpp"
#include <widget/OpaqueWidget.hpp>
#include <widget/TransparentWidget.hpp>
#include <widget/ZoomWidget.hpp>
#include <ui/ScrollWidget.hpp>
#include <ui/SequentialLayout.hpp>
#include <ui/MarginLayout.hpp>
#include <ui/Label.hpp>
#include <ui/TextField.hpp>
#include <ui/MenuOverlay.hpp>
#include <ui/List.hpp>
#include <ui/MenuItem.hpp>
#include <ui/Button.hpp>
#include <ui/RadioButton.hpp>
#include <ui/ChoiceButton.hpp>
#include <ui/Tooltip.hpp>
#include <app/ModuleWidget.hpp>
#include <app/Scene.hpp>
#include <plugin.hpp>
#include <app.hpp>
#include <plugin/Model.hpp>
#include <string.hpp>
#include <history.hpp>
#include <settings.hpp>
#include <tag.hpp>

#include <set>
#include <algorithm>

namespace Mb {
namespace v1 {

#define PREVIEW_MIN 0.2f
#define PREVIEW_MAX 1.6f


// Static functions

static float modelScore(plugin::Model* model, const std::string& search) {
	if (search.empty())
		return 1.f;
	std::string s;
	s += model->plugin->brand;
	s += " ";
	s += model->plugin->name;
	s += " ";
	s += model->name;
	s += " ";
	s += model->slug;
	for (int tagId : model->tags) {
		// Add all aliases of a tag
		for (const std::string& alias : rack::tag::tagAliases[tagId]) {
			s += " ";
			s += alias;
		}
	}
	float score = string::fuzzyScore(string::lowercase(s), string::lowercase(search));
	return score;
}

static bool isModelVisible(plugin::Model* model, const std::string& search, const std::string& brand, int tagId) {
	// Filter search query
	if (search != "") {
		float score = modelScore(model, search);
		if (score <= 0.f)
			return false;
	}

	// Filter brand
	if (brand != "") {
		if (model->plugin->brand != brand)
			return false;
	}

	// Filter tag
	if (tagId >= 0) {
		auto it = std::find(model->tags.begin(), model->tags.end(), tagId);
		if (it == model->tags.end())
			return false;
	}

	return true;
}

static ModuleWidget* chooseModel(plugin::Model* model) {
	// Create module
	ModuleWidget* moduleWidget = model->createModuleWidget();
	assert(moduleWidget);
	APP->scene->rack->addModuleAtMouse(moduleWidget);

	// Push ModuleAdd history action
	history::ModuleAdd* h = new history::ModuleAdd;
	h->name = "create module";
	h->setModule(moduleWidget);
	APP->history->push(h);

	// Hide Module Browser
	APP->scene->moduleBrowser->hide();

	return moduleWidget;
}

template <typename K, typename V>
V get_default(const std::map<K, V>& m, const K& key, const V& def) {
	auto it = m.find(key);
	if (it == m.end())
		return def;
	return it->second;
}


// Widgets


static float modelBoxZoom = 0.9f;


struct ModelBox : widget::OpaqueWidget {
	plugin::Model* model;
	widget::Widget* previewWidget;
	ui::Tooltip* tooltip = NULL;
	/** Lazily created */
	widget::FramebufferWidget* previewFb = NULL;
	widget::ZoomWidget* zoomWidget = NULL;
	float modelBoxZoom = -1.f;
	float modelBoxWidth = -1.f;

	void setModel(plugin::Model* model) {
		this->model = model;
		previewWidget = new widget::TransparentWidget;
		addChild(previewWidget);
	}

	void step() override {
		if (modelBoxZoom != v1::modelBoxZoom) {
			//deletePreview();
			modelBoxZoom = v1::modelBoxZoom;
			// Approximate size as 10HP before we know the actual size.
			// We need a nonzero size, otherwise the parent widget will consider it not in the draw bounds, so its preview will not be lazily created.
			box.size.x = (modelBoxWidth < 0 ? 10 * RACK_GRID_WIDTH : modelBoxWidth) * modelBoxZoom;
			box.size.y = RACK_GRID_HEIGHT * modelBoxZoom;
			box.size = box.size.ceil();

			previewWidget->box.size.y = std::ceil(RACK_GRID_HEIGHT * modelBoxZoom);

			if (previewFb) sizePreview();
		}
		widget::OpaqueWidget::step();
	}

	void createPreview() {
		previewFb = new widget::FramebufferWidget;
		if (math::isNear(APP->window->pixelRatio, 1.0)) {
			// Small details draw poorly at low DPI, so oversample when drawing to the framebuffer
			previewFb->oversample = 2.0;
		}
		previewWidget->addChild(previewFb);

		zoomWidget = new widget::ZoomWidget;
		previewFb->addChild(zoomWidget);

		ModuleWidget* moduleWidget = model->createModuleWidgetNull();
		zoomWidget->addChild(moduleWidget);
		// Save the width, used for correct width of blank before rendered
		modelBoxWidth = moduleWidget->box.size.x;

		sizePreview();
	}

	void sizePreview() {
		zoomWidget->setZoom(modelBoxZoom);

		Widget* moduleWidget = zoomWidget->children.front();
		zoomWidget->box.size.x = moduleWidget->box.size.x * modelBoxZoom;
		zoomWidget->box.size.y = RACK_GRID_HEIGHT * modelBoxZoom;
		previewWidget->box.size.x = std::ceil(zoomWidget->box.size.x);

		box.size.x = previewWidget->box.size.x;

		// Not sure how to do this corretly but works for now
		previewFb->fbBox = previewWidget->box;
		previewFb->dirty = true;
	}

	void deletePreview() {
		if (!previewFb) return;
		previewWidget->removeChild(previewFb);
		delete previewFb;
		previewFb = NULL;
	}

	void draw(const DrawArgs& args) override {
		// Lazily create preview when drawn
		if (!previewFb) {
			createPreview();
		}

		// Draw shadow
		nvgBeginPath(args.vg);
		float r = 10; // Blur radius
		float c = 10; // Corner radius
		nvgRect(args.vg, -r, -r, box.size.x + 2 * r, box.size.y + 2 * r);
		NVGcolor shadowColor = nvgRGBAf(0, 0, 0, 0.5);
		NVGcolor transparentColor = nvgRGBAf(0, 0, 0, 0);
		nvgFillPaint(args.vg, nvgBoxGradient(args.vg, 0, 0, box.size.x, box.size.y, c, r, shadowColor, transparentColor));
		nvgFill(args.vg);

		OpaqueWidget::draw(args);
	}

	void setTooltip(ui::Tooltip* tooltip) {
		if (this->tooltip) {
			this->tooltip->requestDelete();
			this->tooltip = NULL;
		}

		if (tooltip) {
			APP->scene->addChild(tooltip);
			this->tooltip = tooltip;
		}
	}

	void onButton(const event::Button& e) override {
		OpaqueWidget::onButton(e);
		if (e.getTarget() != this)
			return;

		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			ModuleWidget* mw = chooseModel(model);

			// Pretend the moduleWidget was clicked so it can be dragged in the RackWidget
			e.consume(mw);
		}
	}

	void onEnter(const event::Enter& e) override {
		std::string text;
		text = model->plugin->brand;
		text += " " + model->name;
		// Tags
		text += "\nTags: ";
		for (size_t i = 0; i < model->tags.size(); i++) {
			if (i > 0)
				text += ", ";
			int tagId = model->tags[i];
			text += rack::tag::tagAliases[tagId][0];
		}
		// Description
		if (model->description != "") {
			text += "\n" + model->description;
		}
		ui::Tooltip* tooltip = new ui::Tooltip;
		tooltip->text = text;
		setTooltip(tooltip);
	}

	void onLeave(const event::Leave& e) override {
		setTooltip(NULL);
	}

	void onHide(const event::Hide& e) override {
		// Hide tooltip
		setTooltip(NULL);
		OpaqueWidget::onHide(e);
	}
};


struct ModelZoomSlider : ui::Slider {
	struct ModelZoomQuantity : Quantity {
		void setValue(float value) override {
			v1::modelBoxZoom = math::clamp(value, PREVIEW_MIN, PREVIEW_MAX);
		}
		float getValue() override {
			return v1::modelBoxZoom;
		}
		float getDefaultValue() override {
			return 0.9f;
		}
		float getDisplayValue() override {
			return getValue() * 100;
		}
		void setDisplayValue(float displayValue) override {
			setValue(displayValue / 100);
		}
		std::string getLabel() override {
			return "Preview";
		}
		std::string getUnit() override {
			return "";
		}
		int getDisplayPrecision() override {
			return 3;
		}
		float getMaxValue() override {
			return 1.4f;
		}
		float getMinValue() override {
			return 0.2f;
		}
	};

	ModelZoomSlider() {
		box.size.x = 180.0f;
		quantity = new ModelZoomQuantity();
	}
	~ModelZoomSlider() {
		delete quantity;
	}
};


struct BrandItem : ui::MenuItem {
	void onAction(const event::Action& e) override;
	void step() override;
};


struct TagItem : ui::MenuItem {
	int tagId;
	void onAction(const event::Action& e) override;
	void step() override;
};


struct BrowserSearchField : ui::TextField {
	void step() override {
		// Steal focus when step is called
		APP->event->setSelected(this);
		TextField::step();
	}

	void onSelectKey(const event::SelectKey& e) override;
	void onChange(const event::Change& e) override;
	void onAction(const event::Action& e) override;

	void onHide(const event::Hide& e) override {
		APP->event->setSelected(NULL);
		ui::TextField::onHide(e);
	}

	void onShow(const event::Show& e) override {
		selectAll();
		TextField::onShow(e);
	}
};


struct ClearButton : ui::Button {
	void onAction(const event::Action& e) override;
};


struct BrowserSidebar : widget::Widget {
	BrowserSearchField* searchField;
	ClearButton* clearButton;
	ui::Label* tagLabel;
	ui::List* tagList;
	ui::ScrollWidget* tagScroll;
	ui::Label* brandLabel;
	ui::List* brandList;
	ui::ScrollWidget* brandScroll;

	BrowserSidebar() {
		// Search
		searchField = new BrowserSearchField;
		addChild(searchField);

		// Clear filters
		clearButton = new ClearButton;
		clearButton->text = "Reset filters";
		addChild(clearButton);

		// Tag label
		tagLabel = new ui::Label;
		// tagLabel->fontSize = 16;
		tagLabel->color = nvgRGB(0x80, 0x80, 0x80);
		tagLabel->text = "Tags";
		addChild(tagLabel);

		// Tag list
		tagScroll = new ui::ScrollWidget;
		addChild(tagScroll);

		tagList = new ui::List;
		tagScroll->container->addChild(tagList);

		for (int tagId = 0; tagId < (int) tag::tagAliases.size(); tagId++) {
			TagItem* item = new TagItem;
			item->text = tag::tagAliases[tagId][0];
			item->tagId = tagId;
			tagList->addChild(item);
		}

		// Brand label
		brandLabel = new ui::Label;
		// brandLabel->fontSize = 16;
		brandLabel->color = nvgRGB(0x80, 0x80, 0x80);
		brandLabel->text = "Brands";
		addChild(brandLabel);

		// Brand list
		brandScroll = new ui::ScrollWidget;
		addChild(brandScroll);

		brandList = new ui::List;
		brandScroll->container->addChild(brandList);

		// Collect brands from all plugins
		std::set<std::string, string::CaseInsensitiveCompare> brands;
		for (plugin::Plugin* plugin : rack::plugin::plugins) {
			brands.insert(plugin->brand);
		}

		for (const std::string& brand : brands) {
			BrandItem* item = new BrandItem;
			item->text = brand;
			brandList->addChild(item);
		}
	}

	void step() override {
		searchField->box.size.x = box.size.x;
		clearButton->box.pos = searchField->box.getBottomLeft();
		clearButton->box.size.x = box.size.x;

		float listHeight = (box.size.y - clearButton->box.getBottom()) / 2;
		listHeight = std::floor(listHeight);

		tagLabel->box.pos = clearButton->box.getBottomLeft();
		tagLabel->box.size.x = box.size.x;
		tagScroll->box.pos = tagLabel->box.getBottomLeft();
		tagScroll->box.size.y = listHeight - tagLabel->box.size.y;
		tagScroll->box.size.x = box.size.x;
		tagList->box.size.x = tagScroll->box.size.x;

		brandLabel->box.pos = tagScroll->box.getBottomLeft();
		brandLabel->box.size.x = box.size.x;
		brandScroll->box.pos = brandLabel->box.getBottomLeft();
		brandScroll->box.size.y = listHeight - brandLabel->box.size.y;
		brandScroll->box.size.x = box.size.x;
		brandList->box.size.x = brandScroll->box.size.x;

		Widget::step();
	}
};


struct ModuleBrowser : widget::OpaqueWidget {
	BrowserSidebar* sidebar;
	ui::ScrollWidget* modelScroll;
	ui::Label* modelLabel;
	ModelZoomSlider* modelZoomSlider;
	ui::MarginLayout* modelMargin;
	ui::SequentialLayout* modelContainer;

	std::string search;
	std::string brand;
	int tagId = -1;

	ModuleBrowser() {
		sidebar = new BrowserSidebar;
		sidebar->box.size.x = 200;
		addChild(sidebar);

		modelLabel = new ui::Label;
		// modelLabel->fontSize = 16;
		// modelLabel->box.size.x = 400;
		addChild(modelLabel);

		modelZoomSlider = new ModelZoomSlider;
		addChild(modelZoomSlider);

		modelScroll = new ui::ScrollWidget;
		addChild(modelScroll);

		modelMargin = new rack::ui::MarginLayout;
		modelMargin->margin = math::Vec(10, 10);
		modelScroll->container->addChild(modelMargin);

		modelContainer = new ui::SequentialLayout;
		modelContainer->spacing = math::Vec(10, 10);
		modelMargin->addChild(modelContainer);

		// Add ModelBoxes for each Model
		for (plugin::Plugin* plugin : rack::plugin::plugins) {
			for (plugin::Model* model : plugin->models) {
				ModelBox* moduleBox = new ModelBox;
				moduleBox->setModel(model);
				modelContainer->addChild(moduleBox);
			}
		}

		clear();
	}

	void step() override {
		if (!visible) return;
		box = parent->box.zeroPos().grow(math::Vec(-70, -70));

		sidebar->box.size.y = box.size.y;

		modelLabel->box.pos = sidebar->box.getTopRight().plus(math::Vec(5, 5));

		modelZoomSlider->box.pos = Vec(box.size.x - modelZoomSlider->box.size.x - 5, 5);

		modelScroll->box.pos = sidebar->box.getTopRight().plus(math::Vec(0, 30));
		modelScroll->box.size = box.size.minus(modelScroll->box.pos);
		modelMargin->box.size.x = modelScroll->box.size.x;
		modelMargin->box.size.y = modelContainer->getChildrenBoundingBox().size.y + 2 * modelMargin->margin.y;

		OpaqueWidget::step();
	}

	void draw(const DrawArgs& args) override {
		bndMenuBackground(args.vg, 0.0, 0.0, box.size.x, box.size.y, 0);
		Widget::draw(args);
	}

	void refresh() {
		// Reset scroll position
		modelScroll->offset = math::Vec();

		// Filter ModelBoxes
		for (Widget* w : modelContainer->children) {
			ModelBox* m = dynamic_cast<ModelBox*>(w);
			assert(m);
			m->visible = isModelVisible(m->model, search, brand, tagId);
		}

		// Sort ModelBoxes
		modelContainer->children.sort([&](Widget * w1, Widget * w2) {
			ModelBox* m1 = dynamic_cast<ModelBox*>(w1);
			ModelBox* m2 = dynamic_cast<ModelBox*>(w2);
			// Sort by (modifiedTimestamp descending, plugin brand)
			auto t1 = std::make_tuple(-m1->model->plugin->modifiedTimestamp, m1->model->plugin->brand);
			auto t2 = std::make_tuple(-m2->model->plugin->modifiedTimestamp, m2->model->plugin->brand);
			return t1 < t2;
		});

		if (search.empty()) {
			// We've already sorted above
		}
		else {
			std::map<Widget*, float> scores;
			// Compute scores
			for (Widget* w : modelContainer->children) {
				ModelBox* m = dynamic_cast<ModelBox*>(w);
				assert(m);
				if (!m->visible)
					continue;
				scores[m] = modelScore(m->model, search);
			}
			// // Sort by score
			// modelContainer->children.sort([&](Widget *w1, Widget *w2) {
			// 	// If score was not computed, scores[w] returns 0, but this doesn't matter because those widgets aren't visible.
			// 	return get_default(scores, w1, 0.f) > get_default(scores, w2, 0.f);
			// });
		}

		// Filter the brand and tag lists

		// Get modules that would be filtered by just the search query
		std::vector<plugin::Model*> filteredModels;
		for (Widget* w : modelContainer->children) {
			ModelBox* m = dynamic_cast<ModelBox*>(w);
			assert(m);
			if (isModelVisible(m->model, search, "", -1))
				filteredModels.push_back(m->model);
		}

		auto hasModel = [&](const std::string & brand, int tagId) -> bool {
			for (plugin::Model* model : filteredModels) {
				if (isModelVisible(model, "", brand, tagId))
					return true;
			}
			return false;
		};

		// Enable brand and tag items that are available in visible ModelBoxes
		int brandsLen = 0;
		for (Widget* w : sidebar->brandList->children) {
			BrandItem* item = dynamic_cast<BrandItem*>(w);
			assert(item);
			item->disabled = !hasModel(item->text, tagId);
			if (!item->disabled)
				brandsLen++;
		}
		sidebar->brandLabel->text = string::f("Brands (%d)", brandsLen);

		int tagsLen = 0;
		for (Widget* w : sidebar->tagList->children) {
			TagItem* item = dynamic_cast<TagItem*>(w);
			assert(item);
			item->disabled = !hasModel(brand, item->tagId);
			if (!item->disabled)
				tagsLen++;
		}
		sidebar->tagLabel->text = string::f("Tags (%d)", tagsLen);

		// Count models
		int modelsLen = 0;
		for (Widget* w : modelContainer->children) {
			if (w->visible)
				modelsLen++;
		}
		modelLabel->text = string::f("Modules (%d) Click and drag a module to place it in the rack.", modelsLen);
	}

	void clear() {
		search = "";
		sidebar->searchField->setText("");
		brand = "";
		tagId = -1;
		refresh();
	}

	void onShow(const event::Show& e) override {
		refresh();
		OpaqueWidget::onShow(e);
	}

	void onHoverScroll(const event::HoverScroll& e) override {
		if ((APP->window->getMods() & RACK_MOD_MASK) == RACK_MOD_CTRL) {
			// Increase zoom
			float zoomDelta = e.scrollDelta.y / 50.f / 12.f;
			v1::modelBoxZoom = math::clamp(v1::modelBoxZoom + zoomDelta, PREVIEW_MIN, PREVIEW_MAX);
			e.consume(this);
			return;
		}

		OpaqueWidget::onHoverScroll(e);
	}
};


// Implementations to resolve dependencies


inline void BrandItem::onAction(const event::Action& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	if (browser->brand == text)
		browser->brand = "";
	else
		browser->brand = text;
	browser->refresh();
}

inline void BrandItem::step() {
	MenuItem::step();
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	active = (browser->brand == text);
}

inline void TagItem::onAction(const event::Action& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	if (browser->tagId == tagId)
		browser->tagId = -1;
	else
		browser->tagId = tagId;
	browser->refresh();
}

inline void TagItem::step() {
	MenuItem::step();
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	active = (browser->tagId == tagId);
}

inline void BrowserSearchField::onSelectKey(const event::SelectKey& e) {
	if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
		switch (e.key) {
			case GLFW_KEY_ESCAPE: {
				Mb::BrowserOverlay* overlay = getAncestorOfType<Mb::BrowserOverlay>();
				overlay->hide();
				e.consume(this);
			} break;
			case GLFW_KEY_BACKSPACE: {
				if (text == "") {
					ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
					browser->clear();
					e.consume(this);
				}
			} break;
		}
	}

	if (!e.getTarget())
		ui::TextField::onSelectKey(e);
}

inline void BrowserSearchField::onChange(const event::Change& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	browser->search = string::trim(text);
	browser->refresh();
}

inline void BrowserSearchField::onAction(const event::Action& e) {
	// Get first ModelBox
	ModelBox* mb = NULL;
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	for (Widget* w : browser->modelContainer->children) {
		if (w->visible) {
			mb = dynamic_cast<ModelBox*>(w);
			break;
		}
	}

	if (mb) {
		chooseModel(mb->model);
	}
}

inline void ClearButton::onAction(const event::Action& e) {
	ModuleBrowser* browser = getAncestorOfType<ModuleBrowser>();
	browser->clear();
}

} // namespace v1
} // namespace Mb