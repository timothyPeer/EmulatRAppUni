// ============================================================================
// EnvironVariables.h - Environ Variables
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
//
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef ENVIRONVARIABLES_H
#define ENVIRONVARIABLES_H
#include <QtGlobal>

/* section 26.2 */


/* have meaning to console and system software */
    enum class environVars {
        AUTO_ACTION = 0x01,
        BOOT_DEV = 0x02,
        BOOTDEF_DEV = 0x03,
        BOOTED_DEV = 0x04,
        BOOT_FILE = 0x05,
        BOOTED_FILE = 0x06,
        BOOT_OS_FLAG = 0x07,
        BOOTED_OS_FLAGS = 0x08,
        BOOT_RESET = 0x09,
        DUMP_DEV = 0x0A,
        ENABLE_AUDIT = 0x0B,
        LICENSE = 0x0C,
        CHAR_SET = 0x0D,
        LANGUAGE = 0x0E,
        TTU_DEV = 0x0F,

    };


    enum class AUTO_ACTION : quint64 {
        BOOT = 0x544F4F42,
        HALT = 0x544C4148,
        RESTART = 0x54524154534552,
    };
    enum class BOOT_RESET : quint64 {
        OFF = 0x46464F,
        ON = 0x4E4F
    };

    enum class ENABLE_AUDIT : quint64 {
        OFF = 0x46464F,
        ON = 0x4E4F
    };



// namespace Alpha_ENV_Specific {
// /* have meaning to console and system software */
// enum class environVars {

// };
// }

// namespace Alpha_ENV_Alpha {
// /* have meaning to console and system software */
// enum class environVars {

// };
// }


// namespace Alpha_ENV_Tru64 {
// /* have meaning to console and system software */
// enum class environVars {

// };
// }
namespace Alpha_State_Transitions  {

    enum class MajorState : quint8 {
        POWERED_OFF = 0x00,
        HALTED = 0x01,
        BOOTSTRAPPING = 0x02,
        RESTARTING = 0x03,
        RUNNING = 0x04
    };
}


    // ============================================================================
    // BIP_RC flags (Primary CPU0 only)
    // ----------------------------------------------------------------------------
    // Encodes bootstrap / halt / restart outcomes using independent bit flags.
    // ============================================================================
    enum BIP_RC_Flag : quint8
    {
	    // ---------------------------------------------------------------------
	    // Base / state
	    // ---------------------------------------------------------------------
	    BIP_NONE = 0x00, // No error / normal state

	    // ---------------------------------------------------------------------
	    // Bootstrap & restart lifecycle
	    // ---------------------------------------------------------------------
	    BIP_BOOTSTRAP_FAIL = 0x01, // Initial bootstrap failed
	    BIP_RESTART_ATTEMPT = 0x02, // Processor restart attempted
	    BIP_RESTART_FAIL = 0x04, // Restart attempt failed
	    BIP_RESTART_SUCCESS = 0x08, // Restart succeeded (informational)

	    // ---------------------------------------------------------------------
	    // HALT sources
	    // ---------------------------------------------------------------------
	    BIP_HALT_INSTRUCTION = 0x10, // CALL_PAL HALT executed
	    BIP_HALT_CSERVE = 0x20, // CSERVE / console-requested halt
	    BIP_HALT_OPA0 = 0x40, // Operator attention (OPA0 / external console) ctr-P >>> Halt
	    BIP_HALT_FATAL = 0x80  // Fatal condition (BUGCHK / MCHK escalation)
    };

	struct environ_vars
	{
		// ---------------------------------------------------------------------
		// Primary CPU0 only flags
		// ---------------------------------------------------------------------
		quint8 bip_rc = BIP_NONE;          // Bootstrap / halt / restart flags
		quint8 state_transition = 0;       // bitwise MajorState

		// ---------------------------------------------------------------------
		// BIP_RC accessors
		// ---------------------------------------------------------------------
		inline bool bipBootstrapFailed() const noexcept {
			return testFlag(bip_rc, BIP_BOOTSTRAP_FAIL);
		}

		inline bool bipRestartAttempted() const noexcept {
			return testFlag(bip_rc, BIP_RESTART_ATTEMPT);
		}

		inline bool bipRestartFailed() const noexcept {
			return testFlag(bip_rc, BIP_RESTART_FAIL);
		}

		inline bool bipRestartSucceeded() const noexcept {
			return testFlag(bip_rc, BIP_RESTART_SUCCESS);
		}

		// ---------------------------------------------------------------------
		// HALT source accessors
		// ---------------------------------------------------------------------
		inline bool bipHaltInstruction() const noexcept {
			return testFlag(bip_rc, BIP_HALT_INSTRUCTION);
		}

		inline bool bipHaltCserve() const noexcept {
			return testFlag(bip_rc, BIP_HALT_CSERVE);
		}

		inline bool bipHaltOpa0() const noexcept {
			return testFlag(bip_rc, BIP_HALT_OPA0);
		}

		inline bool bipHaltFatal() const noexcept {
			return testFlag(bip_rc, BIP_HALT_FATAL);
		}

		inline bool bipAnyHalt() const noexcept {
			return bip_rc & (BIP_HALT_INSTRUCTION |
				BIP_HALT_CSERVE |
				BIP_HALT_OPA0 |
				BIP_HALT_FATAL);
		}

		// ---------------------------------------------------------------------
		// Major state accessors
		// ---------------------------------------------------------------------
		inline bool isPoweredOff() const noexcept {
			return state_transition == static_cast<quint8>(
				Alpha_State_Transitions::MajorState::POWERED_OFF);
		}

		inline bool isHalted() const noexcept {
			return state_transition == static_cast<quint8>(
				Alpha_State_Transitions::MajorState::HALTED);
		}

		inline bool isBootstrapping() const noexcept {
			return state_transition == static_cast<quint8>(
				Alpha_State_Transitions::MajorState::BOOTSTRAPPING);
		}

		inline bool isRestarting() const noexcept {
			return state_transition == static_cast<quint8>(
				Alpha_State_Transitions::MajorState::RESTARTING);
		}

		inline bool isRunning() const noexcept {
			return state_transition == static_cast<quint8>(
				Alpha_State_Transitions::MajorState::RUNNING);
		}
		inline bool testFlag(quint64 value, quint64 flagMask) const noexcept {
			return (value & flagMask) != 0;
		}
	};


#endif // ENVIRONVARIABLES_H
