#include "../plugin.hpp"
#include "PortEx.hpp"
#include "AudioWidget.hpp"
#include <audio.hpp>

namespace AudioEx {

template <int AUDIO_OUTPUTS, int AUDIO_INPUTS>
struct AudioExPort : PortEx {
	// Audio thread produces, engine thread consumes
	dsp::DoubleRingBuffer<dsp::Frame<AUDIO_INPUTS>, (1 << 15)> inputBuffer;
	// Audio thread consumes, engine thread produces
	dsp::DoubleRingBuffer<dsp::Frame<AUDIO_OUTPUTS>, (1 << 15)> outputBuffer;
	bool active = false;

	~AudioExPort() {
		setDeviceIdEx(-1, 0);
	}

	void processStream(const float* input, float* output, int frames) override {
		// Reactivate idle stream
		if (!active) {
			active = true;
			inputBuffer.clear();
			outputBuffer.clear();
		}

		if (numInputs > 0) {
			for (int i = 0; i < frames; i++) {
				if (inputBuffer.full())
					break;
				dsp::Frame<AUDIO_INPUTS> inputFrame;
				std::memset(&inputFrame, 0, sizeof(inputFrame));
				std::memcpy(&inputFrame, &input[numInputs * i], numInputs * sizeof(float));
				inputBuffer.push(inputFrame);
			}
		}

		if (numOutputs > 0 && (int)outputBuffer.size() >= frames) {
			// Consume audio block
			for (int i = 0; i < frames; i++) {
				dsp::Frame<AUDIO_OUTPUTS> f = outputBuffer.shift();
				for (int j = 0; j < numOutputs; j++) {
					output[numOutputs * i + j] = clamp(f.samples[j], -1.f, 1.f);
				}
			}
		}
		else {
			// Timed out, fill output with zeros
			std::memset(output, 0, frames * numOutputs * sizeof(float));
			// DEBUG("Audio Interface Port underflow");
		}
	}

	size_t getBufferFillStatus() override {
		return inputBuffer.size();
	}

	void onCloseStream() override {
		inputBuffer.clear();
		outputBuffer.clear();
	}

	void onChannelsChange() override {
	}
}; // struct AudioExPort


template <int AUDIO_OUTPUTS, int AUDIO_INPUTS>
struct AudioExModule : Module {
	enum ParamIds {
		NUM_PARAMS
	};
	enum InputIds {
		ENUMS(AUDIO_INPUT, AUDIO_INPUTS),
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(AUDIO_OUTPUT, AUDIO_OUTPUTS),
		NUM_OUTPUTS
	};
	enum LightIds {
		ENUMS(INPUT_LIGHT, AUDIO_INPUTS / 2),
		ENUMS(OUTPUT_LIGHT, AUDIO_OUTPUTS / 2),
		NUM_LIGHTS
	};

	AudioExPort<AUDIO_OUTPUTS, AUDIO_INPUTS> port;
	int lastSampleRate = 0;
	int lastNumOutputs = -1;
	int lastNumInputs = -1;

	dsp::SampleRateConverter<AUDIO_INPUTS> inputSrc;
	dsp::SampleRateConverter<AUDIO_OUTPUTS> outputSrc;

	// in rack's sample rate
	dsp::DoubleRingBuffer<dsp::Frame<AUDIO_INPUTS>, 16> inputBuffer;
	dsp::DoubleRingBuffer<dsp::Frame<AUDIO_OUTPUTS>, 16> outputBuffer;

	bool active = false;
	dsp::ClockDivider lightDivider;

	AudioExModule() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		port.maxChannels = std::max(AUDIO_OUTPUTS, AUDIO_INPUTS);
		lightDivider.setDivision(1024);
		onReset();
	}

	void process(const ProcessArgs& args) override {
		if (!active && port.active) {
			// Fill up the buffer
			if (port.inputBuffer.size() < 16) {
				return;
			}
			else {
				active = true;
			}
		}

		// Update SRC states
		inputSrc.setRates(port.sampleRate, args.sampleRate);
		outputSrc.setRates(args.sampleRate, port.sampleRate);
		inputSrc.setChannels(port.numInputs);
		outputSrc.setChannels(port.numOutputs);

		// Inputs: audio engine -> rack engine
		if (port.active && port.numInputs > 0) {
			if (port.inputBuffer.size() > 0) {
				int inLen = port.inputBuffer.size();
				int outLen = inputBuffer.capacity();
				inputSrc.process(port.inputBuffer.startData(), &inLen, inputBuffer.endData(), &outLen);
				port.inputBuffer.startIncr(inLen);
				inputBuffer.endIncr(outLen);
			}
			else {
				active = true;
			}
		}
		dsp::Frame<AUDIO_INPUTS> inputFrame;
		if (!inputBuffer.empty()) {
			// Take input from buffer
			inputFrame = inputBuffer.shift();
		}
		else {
			std::memset(&inputFrame, 0, sizeof(inputFrame));
		}
		for (int i = 0; i < port.numInputs; i++) {
			outputs[AUDIO_OUTPUT + i].setVoltage(10.f * inputFrame.samples[i]);
		}
		for (int i = port.numInputs; i < AUDIO_INPUTS; i++) {
			outputs[AUDIO_OUTPUT + i].setVoltage(0.f);
		}

		// Outputs: rack engine -> audio engine
		if (!outputBuffer.full()) {
			dsp::Frame<AUDIO_OUTPUTS> outputFrame;
			for (int i = 0; i < AUDIO_OUTPUTS; i++) {
				outputFrame.samples[i] = inputs[AUDIO_INPUT + i].getVoltageSum() / 10.f;
			}
			outputBuffer.push(outputFrame);
		}
		if (port.active && port.numOutputs > 0 && outputBuffer.size() > 0) {
			int inLen = outputBuffer.size();
			int outLen = port.outputBuffer.capacity();
			outputSrc.process(outputBuffer.startData(), &inLen, port.outputBuffer.endData(), &outLen);
			outputBuffer.startIncr(inLen);
			port.outputBuffer.endIncr(outLen);
		}

		// Set channel lights infrequently
		if (lightDivider.process()) {
			// Turn on light if at least one port is enabled in the nearby pair
			for (int i = 0; i < AUDIO_INPUTS / 2; i++) {
				lights[INPUT_LIGHT + i].setBrightness(port.active && port.numOutputs >= 2 * i + 1);
			}
			for (int i = 0; i < AUDIO_OUTPUTS / 2; i++) {
				lights[OUTPUT_LIGHT + i].setBrightness(port.active && port.numInputs >= 2 * i + 1);
			}
		}
	}

	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "audio", port.toJson());
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* audioJ = json_object_get(rootJ, "audio");
		port.fromJsonEx(audioJ);
	}

	void onReset() override {
		port.setDriverIdEx(-1);
		port.setDeviceIdEx(-1, 0);
	}
}; // struct AudioExModule


struct AudioEx16Widget : ModuleWidget {
	typedef AudioExModule<16, 16> TAudioInterface;

	AudioEx16Widget(TAudioInterface* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AudioEx16.svg")));

		for (int i = 0; i < 2; i++) {
			float xIn = 38.3f;
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 136.4f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 0));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 163.7f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 1));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 191.0f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 2));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 218.6f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 3));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 246.3f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 4));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 273.6f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 5));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 300.8f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 6));
			addInput(createInputCentered<PJ301MPort>(Vec(xIn + i * 32.2f, 328.1f), module, TAudioInterface::AUDIO_INPUT + i * 8 + 7));

			float xOut = 111.f;
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 136.4f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 0));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 163.7f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 1));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 191.0f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 2));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 218.6f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 3));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 246.3f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 4));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 273.6f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 5));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 300.8f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 6));
			addOutput(createOutputCentered<PJ301MPort>(Vec(xOut + i * 32.2f, 328.1f), module, TAudioInterface::AUDIO_OUTPUT + i * 8 + 7));

			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xIn + 9.8f + i * 32.2f, 150.1f), module, TAudioInterface::INPUT_LIGHT + i * 4 + 0));
			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xIn + 9.8f + i * 32.2f, 205.0f), module, TAudioInterface::INPUT_LIGHT + i * 4 + 1));
			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xIn + 9.8f + i * 32.2f, 259.9f), module, TAudioInterface::INPUT_LIGHT + i * 4 + 2));
			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xIn + 9.8f + i * 32.2f, 314.5f), module, TAudioInterface::INPUT_LIGHT + i * 4 + 3));

			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xOut - 9.8f + i * 32.2f, 150.1f), module, TAudioInterface::OUTPUT_LIGHT + i * 4 + 0));
			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xOut - 9.8f + i * 32.2f, 205.0f), module, TAudioInterface::OUTPUT_LIGHT + i * 4 + 1));
			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xOut - 9.8f + i * 32.2f, 259.9f), module, TAudioInterface::OUTPUT_LIGHT + i * 4 + 2));
			addChild(createLightCentered<TinyLight<GreenLight>>(Vec(xOut - 9.8f + i * 32.2f, 314.5f), module, TAudioInterface::OUTPUT_LIGHT + i * 4 + 3));
		}

		AudioEx::AudioWidget* audioWidget = createWidget<AudioEx::AudioWidget>(Vec(13.f, 36.f));
		audioWidget->box.size = Vec(154.f, 67.f);
		audioWidget->setAudioPort(module ? &module->port : NULL);
		addChild(audioWidget);
	}
}; // struct AudioEx16Widget

} // namespace AudioEx

Model* modelAudioEx16 = createModel<AudioEx::AudioExModule<16, 16>, AudioEx::AudioEx16Widget>("AudioEx16");