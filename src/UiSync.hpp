#include "plugin.hpp"

namespace UiSync {

struct UiSyncHandle {
    virtual ~UiSyncHandle() { }
    virtual bool step() { return false; }
};

void registerHandle(UiSyncHandle* h);
void step();

} // namespace UiSync