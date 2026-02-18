#include "global_SubSystemCoordinator.h"
#include "SubsystemCoordinator.h"

AXP_HOT AXP_FLATTEN SubsystemCoordinator& global_SubsystemCoordinator() noexcept {
    static SubsystemCoordinator instance;
    return instance;
}


