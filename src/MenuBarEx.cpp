#include "MenuBarEx.hpp"
#include "UiSync.hpp"
#include "mb/Mb.hpp"
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
		menu->addChild(construct<MbExportItem>(&MenuItem::text, "Export favorites & hidden"));
		menu->addChild(construct<MbImportItem>(&MenuItem::text, "Import favorites & hidden"));
		menu->addChild(construct<MbResetUsageDataItem>(&MenuItem::text, "Reset usage data"));
	}

	void step() override {
		UiSync::step();
		MenuButton::step();
	}
}; // struct MenuBarExButton


void init() {
	std::thread t([](){
		while (!APP || !APP->scene || !APP->scene->menuBar) {
			std::this_thread::sleep_for(std::chrono::milliseconds(200));
		}

		ui::SequentialLayout* layout = APP->scene->menuBar->getFirstDescendantOfType<ui::SequentialLayout>();

		MenuBarExButton* menuBarExButton = new MenuBarExButton;
		menuBarExButton->text = "Extras";
		layout->addChild(menuBarExButton);
		/*
		auto it1 = layout->children.end()--;
		auto it2 = it1--;
		std::swap(it1, it2);
		*/

		Mb::init();
	});
	t.detach();
}

} // namespace MenuBarEx