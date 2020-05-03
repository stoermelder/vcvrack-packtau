#include "plugin.hpp"

Plugin* pluginInstance;

void init(rack::Plugin* p) {
	pluginInstance = p;

	p->addModel(modelT7Ctrl);
	p->addModel(modelT7Midi);
	p->addModel(modelT7Assistant);

	p->addModel(modelMb);
}