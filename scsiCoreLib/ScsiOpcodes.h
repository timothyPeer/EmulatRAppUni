// ============================================================================
// ScsiOpcodes.h - ============================================================================
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

#ifndef SCSI_OPCODES_H
#define SCSI_OPCODES_H
// ============================================================================
// ScsiOpcodes.H  -  SCSI Command Opcode Definitions for scsiCoreLib
// ============================================================================
// This header defines standard SCSI primary command opcodes used by both
// virtual SCSI devices and SCSI controllers. These opcodes cover common
// 6-, 10-, 12-, and 16-byte CDB formats.
//
// Design Goals:
//   - Header-only, pure constants.
//   - No dependencies on coreLib or controllerLib.
//   - Aligns with SPC-3 and SBC-3 standards.
//   - Pure ASCII, UTF-8 (no BOM).
//
// References:
//   - SCSI Primary Commands - 3 (SPC-3)
//   - SCSI Block Commands - 3 (SBC-3)
//   - SCSI Multimedia Commands (MMC-5) - for CD/DVD devices.
//
// ============================================================================


#include <QtGlobal>

// ============================================================================
// 6-BYTE COMMAND OPCODES (CDB6)
// Reference: SPC-3 Table 31 - "Command Codes (CDB6)"
// ============================================================================
namespace ScsiOpcode6
{
	static const quint8 TEST_UNIT_READY = 0x00;
	static const quint8 REWIND = 0x01; // Tape only
	static const quint8 REQUEST_SENSE = 0x03;
	static const quint8 FORMAT_UNIT = 0x04;
	static const quint8 READ_BLOCK_LIMITS = 0x05;
	static const quint8 REASSIGN_BLOCKS = 0x07;
	static const quint8 READ6 = 0x08;
	static const quint8 WRITE6 = 0x0A;
	static const quint8 SEEK6 = 0x0B;
	static const quint8 READ_REVERSE = 0x0F; // Tape-only
	static const quint8 WRITE_FILEMARKS = 0x10; // Tape-only
	static const quint8 SPACE = 0x11; // Tape-only
	static const quint8 INQUIRY = 0x12;
	static const quint8 RECOVER_BUFFERED_DATA = 0x14; // Obsolete
	static const quint8 MODE_SELECT6 = 0x15;
	static const quint8 RESERVE6 = 0x16;
	static const quint8 RELEASE6 = 0x17;
	static const quint8 COPY = 0x18; // Obsolete
	static const quint8 ERASE = 0x19; // Tape-only
	static const quint8 MODE_SENSE6 = 0x1A;
	static const quint8 START_STOP_UNIT = 0x1B;
	static const quint8 RECEIVE_DIAGNOSTIC = 0x1C;
	static const quint8 SEND_DIAGNOSTIC = 0x1D;
	static const quint8 PREVENT_ALLOW = 0x1E;

	// Vendor-specific range: 0xC0�0xFF
}

// ============================================================================
// 10-BYTE COMMAND OPCODES (CDB10)
// Reference: SPC-3 & SBC-3
// ============================================================================
namespace ScsiOpcode10
{
	static const quint8 READ_CAPACITY10 = 0x25;
	static const quint8 READ10 = 0x28;
	static const quint8 WRITE10 = 0x2A;
	static const quint8 SEEK10 = 0x2B;
	static const quint8 WRITE_VERIFY10 = 0x2E;
	static const quint8 VERIFY10 = 0x2F;

	static const quint8 SYNCHRONIZE_CACHE10 = 0x35;

	static const quint8 LOCK_UNLOCK_CACHE = 0x36;
	static const quint8 READ_DEFECT_DATA = 0x37;

	static const quint8 MEDIUM_SCAN = 0x38;
	static const quint8 COMPARE = 0x39;
	static const quint8 COPY_VERIFY = 0x3A;
	static const quint8 WRITE_BUFFER = 0x3B;
	static const quint8 READ_BUFFER = 0x3C;
	static const quint8 UPDATE_BLOCK = 0x3D;
	static const quint8 READ_LONG = 0x3E;
	static const quint8 WRITE_LONG = 0x3F;
}

// ============================================================================
// 12-BYTE COMMAND OPCODES (CDB12)
// Reference: SPC-3 Table - CDB12
// ============================================================================
namespace ScsiOpcode12
{
	static const quint8 READ12 = 0xA8;
	static const quint8 WRITE12 = 0xAA;
	static const quint8 WRITE_VERIFY12 = 0xAE;
	static const quint8 VERIFY12 = 0xAF;

	static const quint8 READ_DEFECT_DATA12 = 0xB7;
}

// ============================================================================
// 16-BYTE COMMAND OPCODES (CDB16)
// Reference: SPC-3, SBC-3 - CDB16
// ============================================================================
namespace ScsiOpcode16
{
	static const quint8 READ16 = 0x88;
	static const quint8 WRITE16 = 0x8A;
	static const quint8 VERIFY16 = 0x8F;

	static const quint8 SYNCHRONIZE_CACHE16 = 0x91;

	static const quint8 WRITE_SAME16 = 0x93;
	static const quint8 SERVICE_ACTION_IN16 = 0x9E;
}

// ============================================================================
// SERVICE ACTIONS (for opcode 0x9E / 0x9F)
// ============================================================================
namespace ScsiServiceAction
{
	// For 0x9E (SERVICE ACTION IN)
	static const quint16 READ_CAPACITY16 = 0x10;
	static const quint16 GET_LBA_STATUS = 0x12;

	// Vendor and reserved actions are left undefined intentionally.
}

#endif // SCSI_OPCODES_H
