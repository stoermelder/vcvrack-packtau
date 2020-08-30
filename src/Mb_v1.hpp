#include "Mb.hpp"
#include <plugin.hpp>
#include <ui/MarginLayout.hpp>

namespace Mb {
namespace v1 {

#define PREVIEW_MIN 0.2f
#define PREVIEW_MAX 1.6f

extern float modelBoxZoom;


struct ModelZoomSlider : ui::Slider { 
	ModelZoomSlider();
	~ModelZoomSlider();
};


enum class ModuleBrowserSort {
	DEFAULT,
	NAME,
	LAST_USED,
	MOST_USED
};

struct BrowserSidebar : widget::Widget {
	ui::TextField* searchField;
	ui::Button* clearButton;
	ui::List* favoriteList;
	ui::Label* tagLabel;
	ui::List* tagList;
	ui::ScrollWidget* tagScroll;
	ui::Label* brandLabel;
	ui::List* brandList;
	ui::ScrollWidget* brandScroll;

	BrowserSidebar();
	void step() override;
};

struct ModuleBrowser : widget::OpaqueWidget {
	BrowserSidebar* sidebar;
	ui::ScrollWidget* modelScroll;
	ui::Label* modelLabel;
	ui::MenuItem* modelSortDefaultItem;
	ui::MenuItem* modelSortNameItem;
	ui::MenuItem* modelSortLastUsedItem;
	ui::MenuItem* modelSortMostUsedItem;
	ui::Slider* modelZoomSlider;
	ui::MarginLayout* modelMargin;
	ui::SequentialLayout* modelContainer;

	std::string search;
	bool favorites;
	std::string brand;
	std::set<int> tagId;
	bool hidden;
	ModuleBrowserSort sort = ModuleBrowserSort::DEFAULT;
	std::set<int> emptyTagId;

	ModuleBrowser();
	void step() override;
	void draw(const DrawArgs& args) override;
	void refresh(bool resetScroll);
	void clear(bool keepSearch);
	void onShow(const event::Show& e) override;
	void onHoverScroll(const event::HoverScroll& e) override;
};

} // namespace v1
} // namespace Mb