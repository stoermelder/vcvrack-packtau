#include "MenuBarEx.hpp"

Plugin* pluginInstance;

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

	// At this point no context is known and it is impossible to initialize the menubar extension
	// as this initialization is needed for multiple windows.
	// I don't know how to hook this up at the moment...
	//MenuBarEx::init();
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