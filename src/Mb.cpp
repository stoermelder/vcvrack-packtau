#include "plugin.hpp"

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
				mbV06->visible = visible && true;
				mbV1->visible = visible && false;
				break;
			case MODE::V1:
				mbV06->visible = visible && false;
				mbV1->visible = visible && true;
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


namespace v06 {

static const float itemMargin = 2.0;

static std::set<Model*> sFavoriteModels;
static std::string sAuthorFilter;
static int sTagFilter = -1;



bool isMatch(std::string s, std::string search) {
	s = string::lowercase(s);
	search = string::lowercase(search);
	return (s.find(search) != std::string::npos);
}

static bool isModelMatch(Model *model, std::string search) {
	if (search.empty())
		return true;
	std::string s;
	s += model->plugin->slug;
	s += " ";
	s += model->plugin->brand;
	s += " ";
	s += model->name;
	s += " ";
	s += model->slug;
	for (auto tag : model->tags) {
		s += " ";
		s += tag::tagAliases[tag][0];
	}
	return isMatch(s, search);
}


struct FavoriteRadioButton : RadioButton {
	Model *model = NULL;

	struct FavoriteQuantity : Quantity {
		float value;
		void setValue(float v) override { value = v; }
		float getValue() override { return value; }
		std::string getLabel() override { return "★"; }
	};

	FavoriteRadioButton() {
		quantity = new FavoriteQuantity;
	}

	~FavoriteRadioButton() {
		delete quantity;
	}

	void onAction(const event::Action& e) override;
};


struct SeparatorItem : OpaqueWidget {
	SeparatorItem() {
		box.size.y = 2*BND_WIDGET_HEIGHT + 2*itemMargin;
	}

	void setText(std::string text) {
		clearChildren();
		Label *label = new Label;
		label->setPosition(Vec(0, 12 + itemMargin));
		label->text = text;
		label->fontSize = 20;
		label->color.a *= 0.5;
		addChild(label);
	}
};


struct BrowserListItem : OpaqueWidget {
	bool selected = false;

	BrowserListItem() {
		box.size.y = BND_WIDGET_HEIGHT + 2*itemMargin;
	}

	void draw(const DrawArgs& args) override {
		BNDwidgetState state = selected ? BND_HOVER : BND_DEFAULT;
		bndMenuItem(args.vg, 0.0, 0.0, box.size.x, box.size.y, state, -1, "");
		Widget::draw(args);
	}

	void onDragStart(const event::DragStart &e) override;

	void onDragDrop(const event::DragDrop &e) override {
		if (e.origin != this)
			return;
		doAction();
	}

	void doAction() {
		event::Action eAction;
		eAction.consume(this);
		onAction(eAction);
		if (eAction.isConsumed()) {
			// deletes `this`
			Mb::BrowserOverlay* overlay = getAncestorOfType<Mb::BrowserOverlay>();
			overlay->hide();
		}
	}
};


struct ModelItem : BrowserListItem {
	Model *model;
	Label *pluginLabel = NULL;

	void setModel(Model *model) {
		clearChildren();
		assert(model);
		this->model = model;

		FavoriteRadioButton *favoriteButton = new FavoriteRadioButton;
		favoriteButton->setPosition(Vec(8, itemMargin));
		favoriteButton->box.size.x = 20;
		//favoriteButton->quantity->label = "★";
		addChild(favoriteButton);

		// Set favorite button initial state
		auto it = sFavoriteModels.find(model);
		if (it != sFavoriteModels.end())
			favoriteButton->quantity->setValue(1);
		favoriteButton->model = model;

		Label *nameLabel = new Label;
		nameLabel->setPosition(favoriteButton->box.getTopRight());
		nameLabel->text = model->name;
		addChild(nameLabel);

		pluginLabel = new Label;
		pluginLabel->setPosition(Vec(0, itemMargin));
		pluginLabel->alignment = Label::RIGHT_ALIGNMENT;
		pluginLabel->text = model->plugin->slug + " " + model->plugin->version;
		pluginLabel->color.a = 0.5;
		addChild(pluginLabel);
	}

	void step() override {
		BrowserListItem::step();
		if (pluginLabel)
			pluginLabel->box.size.x = box.size.x - BND_SCROLLBAR_WIDTH;
	}

	void onAction(const event::Action &e) override {
		ModuleWidget *moduleWidget = model->createModuleWidget();
		if (!moduleWidget)
			return;
		APP->scene->rack->addModuleAtMouse(moduleWidget);

		// Push ModuleAdd history action
		history::ModuleAdd* h = new history::ModuleAdd;
		h->name = "create module";
		h->setModule(moduleWidget);
		APP->history->push(h);

		// Move module nearest to the mouse position
		//moduleWidget->box.pos = APP->scene->rack->mousePos.minus(moduleWidget->box.size.div(2));
		//APP->scene->rack->requestModulePos(moduleWidget, moduleWidget->box.pos);
	}
};


struct AuthorItem : BrowserListItem {
	std::string author;

	void setAuthor(std::string author) {
		clearChildren();
		this->author = author;
		Label *authorLabel = new Label;
		authorLabel->setPosition(Vec(0, 0 + itemMargin));
		if (author.empty())
			authorLabel->text = "Show all modules";
		else
			authorLabel->text = author;
		addChild(authorLabel);
	}

	void onAction(const event::Action &e) override;
};


struct TagItem : BrowserListItem {
	int tag;

	void setTag(int tag) {
		clearChildren();
		this->tag = tag;
		Label *tagLabel = new Label;
		tagLabel->setPosition(Vec(0, 0 + itemMargin));
		if (tag == -1)
			tagLabel->text = "Show all tags";
		else
			tagLabel->text = tag::tagAliases[tag][0];
		addChild(tagLabel);
	}

	void onAction(const event::Action &e) override;
};


struct ClearFilterItem : BrowserListItem {
	ClearFilterItem() {
		Label *label = new Label;
		label->setPosition(Vec(0, 0 + itemMargin));
		label->text = "Back";
		addChild(label);
	}

	void onAction(const event::Action &e) override;
};


struct BrowserList : List {
	int selected = 0;

	void step() override {
		incrementSelection(0);
		// Find and select item
		int i = 0;
		for (Widget *child : children) {
			BrowserListItem *item = dynamic_cast<BrowserListItem*>(child);
			if (item) {
				item->selected = (i == selected);
				i++;
			}
		}
		List::step();
	}

	void incrementSelection(int delta) {
		selected += delta;
		selected = clamp(selected, 0, countItems() - 1);
	}

	int countItems() {
		int n = 0;
		for (Widget *child : children) {
			BrowserListItem *item = dynamic_cast<BrowserListItem*>(child);
			if (item) {
				n++;
			}
		}
		return n;
	}

	void selectItem(Widget *w) {
		int i = 0;
		for (Widget *child : children) {
			BrowserListItem *item = dynamic_cast<BrowserListItem*>(child);
			if (item) {
				if (child == w) {
					selected = i;
					break;
				}
				i++;
			}
		}
	}

	BrowserListItem *getSelectedItem() {
		int i = 0;
		for (Widget *child : children) {
			BrowserListItem *item = dynamic_cast<BrowserListItem*>(child);
			if (item) {
				if (i == selected) {
					return item;
				}
				i++;
			}
		}
		return NULL;
	}

	void scrollSelected() {
		BrowserListItem *item = getSelectedItem();
		if (item) {
			ScrollWidget *parentScroll = dynamic_cast<ScrollWidget*>(parent->parent);
			if (parentScroll)
				parentScroll->scrollTo(item->box);
		}
	}
};


struct ModuleBrowser;

struct SearchModuleField : TextField {
	ModuleBrowser *moduleBrowser;
	void onChange(const event::Change& e) override;
	void onSelectKey(const event::SelectKey &e) override;
};


struct ModuleBrowser : OpaqueWidget {
	SearchModuleField *searchField;
	ScrollWidget *moduleScroll;
	BrowserList *moduleList;
	std::set<std::string, string::CaseInsensitiveCompare> availableAuthors;
	std::set<int> availableTags;

	ModuleBrowser() {
		box.size.x = 450;
		sAuthorFilter = "";
		sTagFilter = -1;

		// Search
		searchField	= new SearchModuleField();
		searchField->box.size.x = box.size.x;
		searchField->moduleBrowser = this;
		addChild(searchField);

		moduleList = new BrowserList();
		moduleList->box.size = Vec(box.size.x, 0.0);

		// Module Scroll
		moduleScroll = new ScrollWidget();
		moduleScroll->box.pos.y = searchField->box.size.y;
		moduleScroll->box.size.x = box.size.x;
		moduleScroll->container->addChild(moduleList);
		addChild(moduleScroll);

		// Collect authors
		for (Plugin *plugin : rack::plugin::plugins) {
			for (Model *model : plugin->models) {
				// Insert author
				if (!model->plugin->brand.empty())
					availableAuthors.insert(model->plugin->brand);
				// Insert tag
				for (auto tag : model->tags) {
					if (tag != -1)
						availableTags.insert(tag);
				}
			}
		}

		// Trigger search update
		clearSearch();
		refreshSearch();
	}

	void draw(const DrawArgs& args) override {
		bndMenuBackground(args.vg, 0.0, 0.0, box.size.x, box.size.y, BND_CORNER_NONE);
		Widget::draw(args);
	}

	void clearSearch() {
		searchField->setText("");
	}

	bool isModelFiltered(Model *model) {
		if (!sAuthorFilter.empty() && model->plugin->brand != sAuthorFilter)
			return false;
		if (sTagFilter != -1) {
			auto it = std::find(model->tags.begin(), model->tags.end(), sTagFilter);
			if (it == model->tags.end())
				return false;
		}
		return true;
	}

	void refreshSearch() {
		std::string search = searchField->text;
		moduleList->clearChildren();
		moduleList->selected = 0;
		bool filterPage = !(sAuthorFilter.empty() && sTagFilter == -1);

		if (!filterPage) {
			// Favorites
			if (!sFavoriteModels.empty()) {
				SeparatorItem *item = new SeparatorItem();
				item->setText("Favorites");
				moduleList->addChild(item);
			}
			for (Model *model : sFavoriteModels) {
				if (isModelFiltered(model) && isModelMatch(model, search)) {
					ModelItem *item = new ModelItem();
					item->setModel(model);
					moduleList->addChild(item);
				}
			}
			// Author items
			{
				SeparatorItem *item = new SeparatorItem();
				item->setText("Authors");
				moduleList->addChild(item);
			}
			for (std::string author : availableAuthors) {
				if (isMatch(author, search)) {
					AuthorItem *item = new AuthorItem();
					item->setAuthor(author);
					moduleList->addChild(item);
				}
			}
			// Tag items
			{
				SeparatorItem *item = new SeparatorItem();
				item->setText("Tags");
				moduleList->addChild(item);
			}
			for (int tag : availableTags) {
				if (isMatch(tag::tagAliases[tag][0], search)) {
					TagItem *item = new TagItem();
					item->setTag(tag);
					moduleList->addChild(item);
				}
			}
		}
		else {
			// Clear filter
			ClearFilterItem *item = new ClearFilterItem();
			moduleList->addChild(item);
		}

		if (filterPage || !search.empty()) {
			if (!search.empty()) {
				SeparatorItem *item = new SeparatorItem();
				item->setText("Modules");
				moduleList->addChild(item);
			}
			else if (filterPage) {
				SeparatorItem *item = new SeparatorItem();
				if (!sAuthorFilter.empty())
					item->setText(sAuthorFilter);
				else if (sTagFilter != -1)
					item->setText("Tag: " + tag::tagAliases[sTagFilter][0]);
				moduleList->addChild(item);
			}
			// Modules
			for (Plugin *plugin : rack::plugin::plugins) {
				for (Model *model : plugin->models) {
					if (isModelFiltered(model) && isModelMatch(model, search)) {
						ModelItem *item = new ModelItem();
						item->setModel(model);
						moduleList->addChild(item);
					}
				}
			}
		}
	}

	void step() override {
		if (!visible) return;
		box.pos = parent->box.size.minus(box.size).div(2).round();
		box.pos.y = 60;
		box.size.y = parent->box.size.y - 2 * box.pos.y;
		moduleScroll->box.size.y = std::min(box.size.y - moduleScroll->box.pos.y, moduleList->box.size.y);
		box.size.y = std::min(box.size.y, moduleScroll->box.getBottomRight().y);

		APP->event->setSelected(searchField);
		Widget::step();
	}
};


// Implementations of inline methods above

void AuthorItem::onAction(const event::Action &e) {
	ModuleBrowser *moduleBrowser = getAncestorOfType<ModuleBrowser>();
	sAuthorFilter = author;
	moduleBrowser->clearSearch();
	moduleBrowser->refreshSearch();
	//e.isConsumed = false;
}

void TagItem::onAction(const event::Action &e) {
	ModuleBrowser *moduleBrowser = getAncestorOfType<ModuleBrowser>();
	sTagFilter = tag;
	moduleBrowser->clearSearch();
	moduleBrowser->refreshSearch();
	//e.isConsumed = false;
}

void ClearFilterItem::onAction(const event::Action &e) {
	ModuleBrowser *moduleBrowser = getAncestorOfType<ModuleBrowser>();
	sAuthorFilter = "";
	sTagFilter = -1;
	moduleBrowser->refreshSearch();
	//e.isConsumed = false;
}

void FavoriteRadioButton::onAction(const event::Action &e) {
	if (!model)
		return;
	if (quantity->getValue() > 0.f) {
		sFavoriteModels.insert(model);
	}
	else {
		auto it = sFavoriteModels.find(model);
		if (it != sFavoriteModels.end())
			sFavoriteModels.erase(it);
	}

	ModuleBrowser *moduleBrowser = getAncestorOfType<ModuleBrowser>();
	if (moduleBrowser)
		moduleBrowser->refreshSearch();
}

void BrowserListItem::onDragStart(const event::DragStart &e) {
	BrowserList *list = dynamic_cast<BrowserList*>(parent);
	if (list) {
		list->selectItem(this);
	}
}

void SearchModuleField::onChange(const event::Change& e) {
	moduleBrowser->refreshSearch();
}

void SearchModuleField::onSelectKey(const event::SelectKey &e) {
	switch (e.key) {
		case GLFW_KEY_ESCAPE: {
			BrowserOverlay* overlay = getAncestorOfType<BrowserOverlay>();
			overlay->hide();
			e.consume(this);
			return;
		} break;
		case GLFW_KEY_UP: {
			moduleBrowser->moduleList->incrementSelection(-1);
			moduleBrowser->moduleList->scrollSelected();
			e.consume(this);
		} break;
		case GLFW_KEY_DOWN: {
			moduleBrowser->moduleList->incrementSelection(1);
			moduleBrowser->moduleList->scrollSelected();
			e.consume(this);
		} break;
		case GLFW_KEY_PAGE_UP: {
			moduleBrowser->moduleList->incrementSelection(-5);
			moduleBrowser->moduleList->scrollSelected();
			e.consume(this);
		} break;
		case GLFW_KEY_PAGE_DOWN: {
			moduleBrowser->moduleList->incrementSelection(5);
			moduleBrowser->moduleList->scrollSelected();
			e.consume(this);
		} break;
		case GLFW_KEY_ENTER: {
			BrowserListItem *item = moduleBrowser->moduleList->getSelectedItem();
			if (item) {
				item->doAction();
				e.consume(this);
				return;
			}
		} break;
	}

	if (!e.isConsumed()) {
		TextField::onSelectKey(e);
	}
}

// Global functions


json_t *appModuleBrowserToJson() {
	json_t *rootJ = json_object();

	json_t *favoritesJ = json_array();
	for (Model *model : sFavoriteModels) {
		json_t *modelJ = json_object();
		json_object_set_new(modelJ, "plugin", json_string(model->plugin->slug.c_str()));
		json_object_set_new(modelJ, "model", json_string(model->slug.c_str()));
		json_array_append_new(favoritesJ, modelJ);
	}
	json_object_set_new(rootJ, "favorites", favoritesJ);

	return rootJ;
}

void appModuleBrowserFromJson(json_t *rootJ) {
	json_t *favoritesJ = json_object_get(rootJ, "favorites");
	if (favoritesJ) {
		size_t i;
		json_t *favoriteJ;
		json_array_foreach(favoritesJ, i, favoriteJ) {
			json_t *pluginJ = json_object_get(favoriteJ, "plugin");
			json_t *modelJ = json_object_get(favoriteJ, "model");
			if (!pluginJ || !modelJ)
				continue;
			std::string pluginSlug = json_string_value(pluginJ);
			std::string modelSlug = json_string_value(modelJ);
			Model *model = plugin::getModel(pluginSlug, modelSlug);
			if (!model)
				continue;
			sFavoriteModels.insert(model);
		}
	}
}

} // namespace v06


namespace v1 {

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
	float modelBoxZoom = -1.f;

	void setModel(plugin::Model* model) {
		this->model = model;
		previewWidget = new widget::TransparentWidget;
		addChild(previewWidget);
	}

	void step() override {
		if (modelBoxZoom != v1::modelBoxZoom) {
			deletePreview();
			modelBoxZoom = v1::modelBoxZoom;
			// Approximate size as 10HP before we know the actual size.
			// We need a nonzero size, otherwise the parent widget will consider it not in the draw bounds, so its preview will not be lazily created.
			box.size.x = 10 * RACK_GRID_WIDTH * modelBoxZoom;
			box.size.y = RACK_GRID_HEIGHT * modelBoxZoom;
			box.size = box.size.ceil();

			previewWidget->box.size.y = std::ceil(RACK_GRID_HEIGHT * modelBoxZoom);
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

		widget::ZoomWidget* zoomWidget = new widget::ZoomWidget;
		zoomWidget->setZoom(modelBoxZoom);
		previewFb->addChild(zoomWidget);

		ModuleWidget* moduleWidget = model->createModuleWidgetNull();
		zoomWidget->addChild(moduleWidget);

		zoomWidget->box.size.x = moduleWidget->box.size.x * modelBoxZoom;
		zoomWidget->box.size.y = RACK_GRID_HEIGHT * modelBoxZoom;
		previewWidget->box.size.x = std::ceil(zoomWidget->box.size.x);

		box.size.x = previewWidget->box.size.x;
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



BrowserOverlay::BrowserOverlay() {
	v1::modelBoxZoom = pluginSettings.mbV1zoom;
	mbWidgetBackup = APP->scene->moduleBrowser;
	mbWidgetBackup->hide();
	APP->scene->removeChild(mbWidgetBackup);

	v06::appModuleBrowserFromJson(pluginSettings.mbV06favouritesJ);
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
	json_decref(pluginSettings.mbV06favouritesJ);
	pluginSettings.mbV06favouritesJ = v06::appModuleBrowserToJson();
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

		addChild(createLightCentered<TinyLight<WhiteLight>>(Vec(15.f, 291.3f), module, MbModule::LIGHT_ACTIVE));

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

				struct ZoomSlider : ui::Slider {
					struct ZoomQuantity : Quantity {
						void setValue(float value) override {
							v1::modelBoxZoom = math::clamp(value, 0.2f, 1.4f);
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

					ZoomSlider() {
						box.size.x = 180.0f;
						quantity = new ZoomQuantity();
					}
					~ZoomSlider() {
						delete quantity;
					}
				};

				menu->addChild(new ZoomSlider);
				return menu;
			}
		};

		menu->addChild(new MenuSeparator());
		menu->addChild(construct<ModeV06Item>(&MenuItem::text, "v0.6", &ModeV06Item::module, module));
		menu->addChild(construct<ModeV1Item>(&MenuItem::text, "v1", &ModeV1Item::module, module));
	}
};

} // namespace Mb

Model *modelMb = createModel<Mb::MbModule, Mb::MbWidget>("Mb");