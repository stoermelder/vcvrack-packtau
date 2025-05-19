#include "plugin.hpp"

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
}


std::map<std::tuple<std::string, Context*>, Widget*> singletons;

bool registerSingleton(std::string name, Widget* mw) {
	auto it = singletons.find(std::make_tuple(name, APP));
	if (it == singletons.end()) {
		singletons[std::make_tuple(name, APP)] = mw;
		return true;
	}
	return false;
}

bool unregisterSingleton(std::string name, Widget* mw) {
	auto it = singletons.find(std::make_tuple(name, APP));
	if (it != singletons.end() && it->second == mw) {
		singletons.erase(it);
		return true;
	}
	return false;
}