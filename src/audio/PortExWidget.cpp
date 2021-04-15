#include "plugin.hpp"
#include "PortExWidget.hpp"
#include <ui/MenuItem.hpp>
#include <ui/Menu.hpp>
#include <helpers.hpp>

using namespace rack;
using namespace ui;

namespace AudioEx {

struct PortExDriverItem : ui::MenuItem {
	PortEx* port;
	int driverId;
	void onAction(const event::Action& e) override {
		port->setDriverIdEx(driverId);
	}
};

struct PortExDriverChoice : LedDisplayChoice {
	PortEx* port;
	void onAction(const event::Action& e) override {
		if (!port)
			return;

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Audio driver"));
		{
			PortExDriverItem* item = new PortExDriverItem;
			item->port = port;
			item->driverId = -1;
			item->text = "(no driver)";
			item->rightText = CHECKMARK(item->driverId == port->driverId);
			menu->addChild(item);
		}
		for (int driverId : port->getDriverIds()) {
			PortExDriverItem* item = new PortExDriverItem;
			item->port = port;
			item->driverId = driverId;
			item->text = port->getDriverName(driverId);
			item->rightText = CHECKMARK(item->driverId == port->driverId);
			menu->addChild(item);
		}
	}
	void step() override {
		text = (box.size.x >= 200.0) ? "Driver: " : "";
		std::string driverName = (port && port->driverId >= 0) ? port->getDriverName(port->driverId) : "";
		if (driverName != "") {
			text += driverName;
			color.a = 1.f;
		}
		else {
			text += "(No driver)";
			color.a = 0.5f;
		}
	}
};


struct PortExDeviceItem : ui::MenuItem {
	PortEx* port;
	int deviceId;
	int offset;
	void onAction(const event::Action& e) override {
		port->setDeviceIdEx(deviceId, offset);
	}
};

struct PortExDeviceChoice : LedDisplayChoice {
	PortEx* port;
	/** Prevents devices with a ridiculous number of channels from being displayed */
	int maxTotalChannels = 128;

	void onAction(const event::Action& e) override {
		if (!port)
			return;

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Audio device"));
		int deviceCount = port->getDeviceCount();
		{
			PortExDeviceItem* item = new PortExDeviceItem;
			item->port = port;
			item->deviceId = -1;
			item->text = "(No device)";
			item->rightText = CHECKMARK(item->deviceId == port->deviceId);
			menu->addChild(item);
		}
		for (int deviceId = 0; deviceId < deviceCount; deviceId++) {
			int channels = std::min(maxTotalChannels, port->getDeviceChannels(deviceId));
			for (int offset = 0; offset < channels; offset += port->maxChannels) {
				PortExDeviceItem* item = new PortExDeviceItem;
				item->port = port;
				item->deviceId = deviceId;
				item->offset = offset;
				item->text = port->getDeviceDetail(deviceId, offset);
				item->rightText = CHECKMARK(item->deviceId == port->deviceId && item->offset == port->offset);
				menu->addChild(item);
			}
		}
	}
	void step() override {
		text = (box.size.x >= 200.0) ? "Device: " : "";
		std::string detail = (port) ? port->getDeviceDetail(port->deviceId, port->offset) : "";
		if (detail != "") {
			text += detail;
			color.a = (detail == "") ? 0.5f : 1.f;
		}
		else {
			text += "(No device)";
			color.a = 0.5f;
		}
	}
};


struct PortExSampleRateItem : ui::MenuItem {
	PortEx* port;
	int sampleRate;
	void onAction(const event::Action& e) override {
		port->setSampleRateEx(sampleRate);
	}
};

struct PortExSampleRateChoice : LedDisplayChoice {
	PortEx* port;
	void onAction(const event::Action& e) override {
		if (!port)
			return;

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Sample rate"));
		std::vector<int> sampleRates = port->getSampleRates();
		if (sampleRates.empty()) {
			menu->addChild(createMenuLabel("(Locked by device)"));
		}
		for (int sampleRate : sampleRates) {
			PortExSampleRateItem* item = new PortExSampleRateItem;
			item->port = port;
			item->sampleRate = sampleRate;
			item->text = string::f("%g kHz", sampleRate / 1000.0);
			item->rightText = CHECKMARK(item->sampleRate == port->sampleRate);
			menu->addChild(item);
		}
	}
	void step() override {
		text = (box.size.x >= 100.0) ? "Rate: " : "";
		if (port) {
			text += string::f("%g kHz", port->sampleRate / 1000.0);
		}
		else {
			text += "0 kHz";
		}
	}
};


struct PortExBlockSizeItem : ui::MenuItem {
	PortEx* port;
	int blockSize;
	void onAction(const event::Action& e) override {
		port->setBlockSize(blockSize);
	}
};

struct PortExBlockSizeChoice : LedDisplayChoice {
	PortEx* port;
	void onAction(const event::Action& e) override {
		if (!port)
			return;

		ui::Menu* menu = createMenu();
		menu->addChild(createMenuLabel("Block size"));
		std::vector<int> blockSizes = port->getBlockSizes();
		if (blockSizes.empty()) {
			menu->addChild(createMenuLabel("(Locked by device)"));
		}
		for (int blockSize : blockSizes) {
			PortExBlockSizeItem* item = new PortExBlockSizeItem;
			item->port = port;
			item->blockSize = blockSize;
			float latency = (float) blockSize / port->sampleRate * 1000.0;
			item->text = string::f("%d (%.1f ms)", blockSize, latency);
			item->rightText = CHECKMARK(item->blockSize == port->blockSize);
			menu->addChild(item);
		}
	}
	void step() override {
		text = (box.size.x >= 100.0) ? "Block size: " : "";
		if (port) {
			text += string::f("%d", port->blockSize);
		}
		else {
			text += "0";
		}
	}
};

struct PortExBufferFillDisplay : LedDisplayChoice {
	PortEx* port;
	void step() override {
		if (port) {
			text = string::f("%d", port->getBufferFillStatus());
		}
		else {
			text = "0";
		}
	}
};

void PortExWidget::setAudioPort(PortEx* port) {
	clearChildren();

	math::Vec pos;

	PortExDriverChoice* driverChoice = createWidget<PortExDriverChoice>(pos);
	driverChoice->port = port;
	driverChoice->textOffset = math::Vec(6.f, 14.7f);
	driverChoice->box.size = mm2px(math::Vec(box.size.x, 7.5f));
	driverChoice->color = nvgRGB(0xf0, 0xf0, 0xf0);
	addChild(driverChoice);
	pos = driverChoice->box.getBottomLeft();
	this->driverChoice = driverChoice;

	this->driverSeparator = createWidget<LedDisplaySeparator>(pos);
	this->driverSeparator->box.size.x = box.size.x;
	this->driverSeparator->box.pos = driverChoice->box.getBottomLeft();
	addChild(this->driverSeparator);

	PortExDeviceChoice* deviceChoice = createWidget<PortExDeviceChoice>(pos);
	deviceChoice->port = port;
	deviceChoice->textOffset = math::Vec(6.f, 14.7f);
	deviceChoice->box.size = mm2px(math::Vec(box.size.x, 7.5f));
	deviceChoice->box.pos = driverChoice->box.getBottomLeft();
	deviceChoice->color = nvgRGB(0xf0, 0xf0, 0xf0);
	addChild(deviceChoice);
	pos = deviceChoice->box.getBottomLeft();
	this->deviceChoice = deviceChoice;

	this->deviceSeparator = createWidget<LedDisplaySeparator>(pos);
	this->deviceSeparator->box.size.x = box.size.x;
	deviceSeparator->box.pos = deviceChoice->box.getBottomLeft();
	this->addChild(this->deviceSeparator);

	PortExSampleRateChoice* sampleRateChoice = createWidget<PortExSampleRateChoice>(pos);
	sampleRateChoice->port = port;
	sampleRateChoice->textOffset = math::Vec(6.f, 14.7f);
	sampleRateChoice->box.size = mm2px(math::Vec(20.f, 7.5f));
	sampleRateChoice->box.pos = deviceChoice->box.getBottomLeft();
	sampleRateChoice->color = nvgRGB(0xf0, 0xf0, 0xf0);
	addChild(sampleRateChoice);
	this->sampleRateChoice = sampleRateChoice;

	this->sampleRateSeparator = createWidget<LedDisplaySeparator>(pos);
	this->sampleRateSeparator->box.size.y = this->sampleRateChoice->box.size.y;
	this->sampleRateSeparator->box.pos.x = sampleRateChoice->box.pos.x + sampleRateChoice->box.size.x;
	this->sampleRateSeparator->box.pos.y = sampleRateChoice->box.pos.y;
	addChild(this->sampleRateSeparator);

	PortExBlockSizeChoice* bufferSizeChoice = createWidget<PortExBlockSizeChoice>(pos);
	bufferSizeChoice->port = port;
	bufferSizeChoice->textOffset = math::Vec(6.f, 14.7f);
	bufferSizeChoice->box.size = mm2px(math::Vec(16.f, 7.5f));
	bufferSizeChoice->box.pos = sampleRateSeparator->box.pos;
	bufferSizeChoice->color = nvgRGB(0xf0, 0xf0, 0xf0);
	addChild(bufferSizeChoice);
	this->bufferSizeChoice = bufferSizeChoice;

	this->bufferSizeSeparator = createWidget<LedDisplaySeparator>(pos);
	this->bufferSizeSeparator->box.size.y = this->bufferSizeChoice->box.size.y;
	this->bufferSizeSeparator->box.pos.x = bufferSizeChoice->box.pos.x + bufferSizeChoice->box.size.x;
	this->bufferSizeSeparator->box.pos.y = bufferSizeChoice->box.pos.y;
	addChild(this->bufferSizeSeparator);

	PortExBufferFillDisplay* bufferFillDisplay = createWidget<PortExBufferFillDisplay>(pos);
	bufferFillDisplay->port = port;
	bufferFillDisplay->textOffset = math::Vec(6.f, 14.7f);
	bufferFillDisplay->box.size = mm2px(math::Vec(16.f, 7.5f));
	bufferFillDisplay->box.pos = bufferSizeSeparator->box.pos;
	bufferFillDisplay->color = nvgRGB(0xf0, 0xf0, 0xf0);
	addChild(bufferFillDisplay);
	this->bufferFillDisplay = bufferFillDisplay;
}


} // namespace AudioEx