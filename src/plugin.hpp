#include <rack.hpp>
#include "pluginsettings.hpp"

using namespace rack;

extern StoermelderSettings pluginSettings;

extern Plugin* pluginInstance;

extern Model* modelT7Ctrl;
extern Model* modelT7Midi;
extern Model* modelT7Assistant;

extern Model* modelMx;
extern Model* modelPm;
extern Model* modelRf;
extern Model* modelExit;

bool registerSingleton(std::string name, ModuleWidget* mw);
bool unregisterSingleton(std::string name, ModuleWidget* mw);
