// Fallback for platforms without a native editor yet: hosts show their generic parameter UI.
#include "controller.h"
namespace df {
Steinberg::IPlugView* createEditor(Controller*) { return nullptr; }
}  // namespace df
