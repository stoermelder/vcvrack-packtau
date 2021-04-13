#pragma once
#include <audio.hpp>

namespace AudioEx {

struct PortEx : rack::audio::Port {
	~PortEx() {
		abortStream();
	}

	void setDriverIdEx(int driverId) {
		// Close device
		setDeviceIdEx(-1, 0);
		Port::setDriverId(driverId);
	}

	void setDeviceIdEx(int deviceId, int offset) {
		abortStream();
		this->deviceId = deviceId;
		this->offset = offset;
		openStream();
	}

	void setSampleRateEx(int sampleRate) {
		if (sampleRate == this->sampleRate)
			return;
		abortStream();
		this->sampleRate = sampleRate;
		openStream();
	}

	void abortStream() {
		setChannels(0, 0);

		if (rtAudio) {
			if (rtAudio->isStreamRunning()) {
				INFO("Stopping RtAudio stream %d", deviceId);
				try {
					rtAudio->closeStream();
				}
				catch (RtAudioError& e) {
					WARN("Failed to stop RtAudio stream %s", e.what());
				}
				return;
			}
		}
		rack::audio::Port::closeStream();
	}

	virtual size_t getBbufferFillStatus() { 
		return 0; 
	}
};

} // namespace AudioEx