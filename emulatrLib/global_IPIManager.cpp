// ============================================================================
// global_IPIManager.cpp
// ============================================================================

#include "global_IPIManager.h"

#include "SubsystemCoordinator.h"
#include "global_SubsystemCoordinator.h"
#include "../emulatrLib/IPIManager.h"

IPIManager& global_IPIManager() noexcept
{
    return *global_SubsystemCoordinator().ipiManager();
}