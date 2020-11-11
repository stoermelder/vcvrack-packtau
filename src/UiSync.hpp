#include "plugin.hpp"

namespace UiSync {

struct UiSyncHandle {
	virtual ~UiSyncHandle() { }
	virtual void step() { }
};

void registerHandle(UiSyncHandle* h);
void unregisterHandle(UiSyncHandle* h);
void step();

} // namespace UiSync