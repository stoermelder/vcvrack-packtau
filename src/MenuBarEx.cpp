#include "MenuBarEx.hpp"
#include "UiSync.hpp"
#include "mb/Mb.hpp"
#include "mb/Mb_v1.hpp"
#include <thread>

namespace MenuBarEx {


struct MbModeItem : MenuItem {
	int mode;
	void onAction(const event::Action& e) override {
		pluginSettings.mbMode = mode;
		Mb::init();
	}
	void step() override {
		rightText = CHECKMARK(pluginSettings.mbMode == mode);
		MenuItem::step();
	}
};

struct MbHideBrandsItem : MenuItem {
	void onAction(const event::Action& e) override {
		Mb::v1::hideBrands ^= true;
	}
	void step() override {
		rightText = Mb::v1::hideBrands ? "✔" : "";
		MenuItem::step();
	}
};

struct MbSearchDescriptionsItem : MenuItem {
	void onAction(const event::Action& e) override {
		Mb::v1::searchDescriptions ^= true;
	}
	void step() override {
		rightText = Mb::v1::searchDescriptions ? "✔" : "";
		MenuItem::step();
	}
};

struct MbExportItem : MenuItem {
	void onAction(const event::Action& e) override {
		Mb::exportSettingsDialog();
	}
};

struct MbImportItem : MenuItem {
	void onAction(const event::Action& e) override {
		Mb::importSettingsDialog();
	}
};

struct MbResetUsageDataItem : MenuItem {
	void onAction(const event::Action& e) override {
		Mb::modelUsageReset();
	}
};


struct MenuButton : ui::Button {
	void step() override {
		box.size.x = bndLabelWidth(APP->window->vg, -1, text.c_str()) + 1.0;
		Widget::step();
	}
	void draw(const DrawArgs& args) override {
		BNDwidgetState state = BND_DEFAULT;
		if (APP->event->hoveredWidget == this)
			state = BND_HOVER;
		if (APP->event->draggedWidget == this)
			state = BND_ACTIVE;
		bndMenuItem(args.vg, 0.0, 0.0, box.size.x, box.size.y, state, -1, text.c_str());
		Widget::draw(args);
	}
}; // struct MenuButton

struct MenuBarExButton : MenuButton {
	void onAction(const event::Action& e) override {
		ui::Menu* menu = createMenu();
		menu->box.pos = getAbsoluteOffset(math::Vec(0, box.size.y));
		menu->box.size.x = box.size.x;

		menu->addChild(construct<MenuLabel>(&MenuLabel::text, "Module browser"));
		menu->addChild(construct<MbModeItem>(&MenuItem::text, "Default", &MbModeItem::mode, -1));
		menu->addChild(construct<MbModeItem>(&MenuItem::text, "Mode \"v0.6\"", &MbModeItem::mode, (int)Mb::MODE::V06));
		menu->addChild(construct<MbModeItem>(&MenuItem::text, "Mode \"v1 mod\"", &MbModeItem::mode, (int)Mb::MODE::V1));
		menu->addChild(construct<MbHideBrandsItem>(&MenuItem::text, "\"v1 mod\": Hide brand list"));
		menu->addChild(construct<MbSearchDescriptionsItem>(&MenuItem::text, "\"v1 mod\": Search descriptions"));
		menu->addChild(construct<MbExportItem>(&MenuItem::text, "Export favorites & hidden"));
		menu->addChild(construct<MbImportItem>(&MenuItem::text, "Import favorites & hidden"));
		menu->addChild(construct<MbResetUsageDataItem>(&MenuItem::text, "Reset usage data"));
	}

	void step() override {
		UiSync::step();
		MenuButton::step();
	}

	MenuBarExButton() {
		ui::SequentialLayout* layout = APP->scene->menuBar->getFirstDescendantOfType<ui::SequentialLayout>();

		this->text = "Extras";
		layout->addChild(this);

		Mb::init();
	}
}; // struct MenuBarExButton


void init() {
	// Need context (via APP or contextGet()) to exist for menubar extension to work.

	// Check if menu bar extension was already initialized.
	if (!menuBarInitialzed) {
		menuBarInitialzed = true;
		// Probably a memory leak
		new MenuBarExButton;
	}
}

} // namespace MenuBarEx
