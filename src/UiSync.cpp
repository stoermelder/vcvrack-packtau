#include "UiSync.hpp"

namespace UiSync {

std::list<std::tuple<UiSyncHandle*, bool*>> syncHandles;

// Takes ownership of h
void registerHandle(UiSyncHandle* h) {
	bool* b = new bool;
	*b = false;
	syncHandles.push_back(std::make_tuple(h, b));
}

// Marks h for deletion
void unregisterHandle(UiSyncHandle* h) {
	for (auto p : syncHandles) {
		if (std::get<0>(p) == h) {
			*std::get<1>(p) = true;
		}
	}
}

void step() {
	for (auto p : syncHandles) {
		if (!*std::get<1>(p)) {
			std::get<0>(p)->step();
		}
		// bool might have changed in the meantime
		if (*std::get<1>(p)) {
			syncHandles.remove(p);
			delete std::get<0>(p);
			delete std::get<1>(p);
			// break loop as list has changed
			break;
		}
	}
}

} // namespace UiSync
