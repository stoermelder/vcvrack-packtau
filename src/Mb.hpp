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

} // namespace Mb