#pragma once
#include "PortEx.hpp"
#include <common.hpp>
#include <app/LedDisplay.hpp>
#include <audio.hpp>

using namespace rack;
using namespace app;

namespace AudioEx {

struct AudioWidget : LedDisplay {
	LedDisplayChoice* driverChoice;
	LedDisplaySeparator* driverSeparator;
	LedDisplayChoice* deviceChoice;
	LedDisplaySeparator* deviceSeparator;
	LedDisplayChoice* sampleRateChoice;
	LedDisplaySeparator* sampleRateSeparator;
	LedDisplayChoice* bufferSizeChoice;
	LedDisplaySeparator* bufferSizeSeparator;
	LedDisplayChoice* bufferFillDisplay;
	void setAudioPort(PortEx* port);
};

} // namespace AudioEx
