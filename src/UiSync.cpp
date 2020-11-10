#include "UiSync.hpp"

namespace UiSync {

std::list<UiSyncHandle*> syncHandles;

void registerHandle(UiSyncHandle* h) {
    syncHandles.push_back(h);
}

void step() {
    for (auto h : syncHandles) {
        bool r = h->step();
        if (r) {
            syncHandles.remove(h);
            delete h;
            break;
        }
    }
}

} // namespace UiSync