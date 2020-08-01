#pragma once
#include "plugin.hpp"

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

} // namespace Mb