#ifndef PAL_TLB_SHOOTDOWN_H
#define PAL_TLB_SHOOTDOWN_H
#include "../cpuCoreLib/global_SMPManager.h"
#include "../pteLib/global_Ev6TLB_Singleton.h"
#include "../coreLib/types_core.h"

// ============================================================================
// PAL TLB Shootdown Helper
// ============================================================================
// Called by PAL DTB/ITB miss handlers after inserting PTE.
// Broadcasts TLB shootdown IPI to all other CPUs.
// ============================================================================

/**
 * @brief Send TLB shootdown IPI to all CPUs except source.
 *
 * @param sourceCpu CPU that inserted the PTE (excluded from broadcast)
 * @param va Virtual address of inserted PTE
 * @param asn Address space number
 * @param realm I or D realm
 */
inline void sendTLBShootdown(
    CPUIdType sourceCpu,
    quint64 va,
    ASNType asn,
    Realm realm) noexcept
{
    auto& smpMgr = global_SMPManager();

    // Only send if multi-CPU system
    if (smpMgr.cpuCount() <= 1) {
        return;
    }

    // Encode parameters for IPI
    quint32 vaHigh = static_cast<quint32>(va >> 32);
    quint32 vaLow = static_cast<quint32>(va & 0xFFFFFFFF);
    quint64 asnRealm = (static_cast<quint64>(asn) << 32) |
                       (static_cast<quint64>(realm) & 0xFF);

    // Broadcast to all CPUs except source
    quint16 successCount = smpMgr.broadcastIPI(
        sourceCpu,
        IPIQueue::MessageType::TLB_SHOOTDOWN,
        vaHigh,
        vaLow,
        asnRealm
        );

    if (successCount == 0 && smpMgr.cpuCount() > 1) {
        WARN_LOG(QString("CPU%1: TLB shootdown broadcast failed").arg(sourceCpu));
    }
}
#endif // PAL_TLB_SHOOTDOWN_H
