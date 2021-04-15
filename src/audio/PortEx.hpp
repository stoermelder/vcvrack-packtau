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
		if (driverId != -1) {
			Port::setDriverId(driverId);
		}
		else {
			this->driverId = driverId;
		}
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

		if (driverId == -1) {
			return;
		}

		if (rtAudio && driverId >= 0) {
			if (rtAudio->isStreamRunning()) {
				INFO("Stopping RtAudio stream %d", deviceId);
				try {
					rtAudio->closeStream();
				}
				catch (RtAudioError& e) {
					WARN("Failed to stop RtAudio stream %s", e.what());
				}
				onCloseStream();
				return;
			}
		}
		else {
			rack::audio::Port::closeStream();
		}
	}

	virtual size_t getBufferFillStatus() { 
		return 0; 
	}

	void fromJsonEx(json_t* rootJ) {
		abortStream();

		json_t* driverJ = json_object_get(rootJ, "driver");
		if (driverJ)
			setDriverIdEx(json_number_value(driverJ));

		json_t* deviceNameJ = json_object_get(rootJ, "deviceName");
		if (deviceNameJ) {
			std::string deviceName = json_string_value(deviceNameJ);
			// Search for device ID with equal name
			for (int deviceId = 0; deviceId < getDeviceCount(); deviceId++) {
				if (getDeviceName(deviceId) == deviceName) {
					this->deviceId = deviceId;
					break;
				}
			}
		}

		json_t* offsetJ = json_object_get(rootJ, "offset");
		if (offsetJ)
			offset = json_integer_value(offsetJ);

		json_t* maxChannelsJ = json_object_get(rootJ, "maxChannels");
		if (maxChannelsJ)
			maxChannels = json_integer_value(maxChannelsJ);

		json_t* sampleRateJ = json_object_get(rootJ, "sampleRate");
		if (sampleRateJ)
			sampleRate = json_integer_value(sampleRateJ);

		json_t* blockSizeJ = json_object_get(rootJ, "blockSize");
		if (blockSizeJ)
			blockSize = json_integer_value(blockSizeJ);

		openStream();
	}
};

} // namespace AudioEx