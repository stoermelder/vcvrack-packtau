#include "plugin.hpp"

Plugin* pluginInstance;
bool menuBarInitialzed = false;

void init(rack::Plugin* p) {
	pluginInstance = p;

	p->addModel(modelT7Ctrl);
	p->addModel(modelT7Midi);
	p->addModel(modelT7Assistant);

	p->addModel(modelMx);
	p->addModel(modelPm);
	p->addModel(modelRf);
	p->addModel(modelExit);

	pluginSettings.readFromJson();
}


std::map<std::string, ModuleWidget*> singletons;

bool registerSingleton(std::string name, ModuleWidget* mw) {
	auto it = singletons.find(name);
	if (it == singletons.end()) {
		singletons[name] = mw;
		return true;
	}
	return false;
}

bool unregisterSingleton(std::string name, ModuleWidget* mw) {
	auto it = singletons.find(name);
	if (it != singletons.end() && it->second == mw) {
		singletons.erase(it);
		return true;
	}
	return false;
}
