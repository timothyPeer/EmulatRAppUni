#ifndef HWRPB_CORE_H
#define HWRPB_CORE_H
#include <QtGlobal>
/*
 * Hardware privileged context block (HWPCB) for this processor. See Table 27–10 for the contents as set by the console.
 */

enum class HWRPB_slottedEnums : quint64 {

    /*
     * [23:16]  HALT REQUESTED1,2,3
                Indicates the console action requested by system software executing on this processor. Values include:
                - Set to "0" at system bootstraps and secondary processor starts. May be set to non-zero by system software before processor halt and               subsequent processor entry into console I/O mode. See Sections 27.5.7 and 27.4.5.
        [5:9]   RESERVED; MBZ.
        [8]     PALCODE LOADED (PL)3,4,5
        [7]     PALCODE MEMORY VALID (PMV)3,4,5
                    Indicates that this processor’s PALcode memory and scratch space addresses are valid. Set after the necessary memory is allocated and the addresses are written into the processor’s slot. See Sections 27.3.1 and 27.4.3.3.

        [6]     PALCODE VALID (PV)4,5
                    Indicates that this processor’s PALcode is valid. Set after PALcode has been successfully loaded and initialized. See Sections 27.3.1 and 27.4.3.3.

        [5]     CONTEXT VALID (CV)1,3
                    Indicates that the HWPCB in this slot is valid. Set after the console or system software initializes the HWPCB in this slot. See Sections 27.3.1 and 27.4.3.
        [4]     OPERATOR HALTED (OH)1,6
                    Indicates that this processor is in console I/O mode as the result of explicit operator action. See Section 27.5.8.
        [3]     PROCESSOR PRESENT (PP)4,5
                    Indicates that this processor is physically present in the configuration.
        [2]     PROCESSOR AVAILABLE (PA)4,5
                    Indicates that this processor is available for use by system software. The PA bit may differ from the PP bit based on self-test or other diagnostics, or as the result of a console command that explicitly sets this processor unavailable.

        [1]     RESTART CAPABLE (RC)1,2,3,6
                    Indicates that system software executing on this processor is capable of being restarted if a detected error halt, powerfail recovery, or other error condition occurs. Cleared by the console and set by system software. See Sections 27.4.1.3, 27.4.3.6, and 27.5.1.

        [0]     BOOTSTRAP IN PROGRESS (BIP)1,2,3
                    For the primary, this bit indicates that this processor is undergoing a system bootstrap. For a secondary, this bit indicates that a CPU start operation is in progress. Set by the console and cleared by system software. See Sections 27.4.1.3, 27.4.3.6, and 27.5.1.
*/
    perCpuStateFlagBits = 0x128,
    PalCodeMemoryLength= 0x136,
    PalcodeScratchLength=0x144,
    PhysicalAddressOfPalcodeMemorySpace=0x152,
    PhysicalAddressofPalcodeScratchSpace=0x160,
    PalcodeRevisionRequiredByProcessor=0x168,
    ProcessorType=0x176,
    ProcessorVariation=0x184,
    ProcessorRevision=0x192,
    ProcessorSerialNumber=0x200,
    ConfigDataBlock     = 0x208,        // Section 26.1.4
    PhysicalAddressOfLogoutArea = 0x216,
    LogoutAreaLength=0x224,
    HaltPCBB = 0x232,
    HaltPC = 0x240,
    HaltPS = 0x248,
    HaltArgumentList_R25=0x256,
    HaltReturnAddress_R26 = 0x264,
    HaltProcedureValue_R27 = 0x272,
    ReasonForHalt=0x280,
    ReservedForSoftware = 0x288,
    InterprocessorConsoleBufferArea = 0x296,
    PALcodeRevisionsAvailableBlock = 0x464,
    ProcessorSoftwareCompatibilityField=0x592,
    ConsoleDataLogPhysicalAddress = 0x600,
    ConsoleDataLogLength=0x608,
    CacheInformation=0x616,
    CycleCounterFrequency=0x624,
    ReservedForArchitectureUse=0x632
};

#endif // HWRPB_CORE_H
