
#include "global_EmulatR_init.h"
#include "EmulatR_init.h"
#include "global_SubsystemCoordinator.h"

EmulatR_init& global_EmulatR_init() noexcept
{
    static EmulatR_init instance(&global_SubsystemCoordinator());
    return instance;
}

