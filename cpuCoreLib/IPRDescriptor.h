#pragma once
// ============================================================================
// ASA Emulator (c) 2025 Timothy Peer / eNVy Systems, Inc.
// Non-Commercial Use Only.
// ============================================================================
// IPRDescriptor.h
//
// Core IPR descriptor types and family-specific encodings.
//
// Descriptor Architecture:
//   - IPRDescriptorBase: Common fields (access, hooks, timing)
//   - EV4/EV5/EV6Encoding: Family-specific selector encoding
//   - IPRDescriptor: Tagged union of base + encoding
//
// This file defines Tier 3 (hardware) descriptors that Tier 1 maps to.
// ============================================================================

#include <cstdint>
#include <cstddef>
#include <QtGlobal>
#include "EV6_Encoding.h"
#include "Global_IPRInterface.h"
#include "enum_header.h"



// ============================================================================
// HOOK FUNCTION TYPES
// ============================================================================

// Pre-read hook: Can override read value or proceed normally
// Returns: true = override with *outValue, false = read from storage
using PreReadHook = bool (*)(AlphaCPU* argCpu, quint64* outValue);
using PreReadHook_quint64 = quint64 (*)(AlphaCPU* argCpu);

// Pre-write hook: Can modify value or block write
// Returns: true = write *outValue to storage, false = block write
using PreWriteHook = bool (*)(AlphaCPU* argCpu, quint64 valueIn, quint64* outValue);

// Post-read hook: Called after successful read (e.g., clear-on-read)
using PostReadHook = void (*)(AlphaCPU* argCpu, quint64 value);

// Post-write hook: Called after successful write (trigger side effects)
using PostWriteHook = void (*)(AlphaCPU* argCpu, quint64 oldValue, quint64 newValue);
using PostWriteHook_FieldSelect = void (*)(AlphaCPU* argCpu, quint8 fieldSelect, quint64 oldValue, quint64 newValue);
 


	// ============================================================================
	// COMMON DESCRIPTOR BASE (All CPU Families)
	// ============================================================================

	struct IPRDescriptorBase {
		const char* name;
		const char* description;
		quint16 index;
		AccessMode access;
		quint64 readMask;
		quint64 writeMask;
		quint64 clearOnReadMask;
		bool (*readAllowed)(const CPUStateIPRInterface* cpu);
		bool (*writeAllowed)(const CPUStateIPRInterface* cpu);

		struct Hooks {
			PreReadHook onPreRead;
			PreWriteHook onPreWrite;
			PostReadHook onPostRead;
			PostWriteHook onPostWrite;
			PostWriteHook_FieldSelect onPostWrite_fieldSelect;
			PreReadHook_quint64 onPreRead_OnlyCPUStateIPRInterface;

			// 		constexpr Hooks() noexcept
			// 			: onPreRead(nullptr)
			// 			, onPreWrite(nullptr)
			// 			, onPostRead(nullptr)
			// 			, onPostWrite(nullptr)
			// 		{
			// 		}
		} hooks;

		quint8 latencyCycles;
		quint16 scoreboardMask;
		bool volatileRead;
		bool implementationSpecific;

		// ONLY keep the default constructor
	// 	constexpr IPRDescriptorBase() noexcept
	// 		: name(nullptr)
	// 		, tier1Alias(static_cast<Tier1IPR>(0xFF))
	// 		, description(nullptr)
	// 		, index(0)
	// 		, access(AccessMode::NONE)
	// 		, readMask(0)
	// 		, writeMask(0)
	// 		, clearOnReadMask(0)
	// 		, readAllowed(nullptr)
	// 		, writeAllowed(nullptr)
	// 		, hooks{}
	// 		, latencyCycles(0)
	// 		, scoreboardMask(0)
	// 		, volatileRead(false)
	// 		, implementationSpecific(false)
	// 	{
	// 	}

		//  REMOVE THIS - it makes the struct non-aggregate:
		// constexpr IPRDescriptorBase(const char* name_, ...) { ... }
	};


	// ============================================================================
	// UNIFIED IPR DESCRIPTOR (Tagged Union)
	// ============================================================================

	struct IPRDescriptor {
		IPRDescriptorBase base;        // Common fields
		CPUFamily family;              // Which encoding is active

		union {
// 			EV4Encoding ev4;
// 			EV5Encoding ev5;
			EV6Encoding ev6;
		} encoding;

		// Default constructor
	// 	constexpr IPRDescriptor() noexcept
	// 		: base{}
	// 		, family(CPUFamily::EV4)
	// 		, encoding{ .ev4 = EV4Encoding{} }
	// 	{
	// 	}

		// NOTE: No constructors - this keeps IPRDescriptor as an aggregate
		// Use designated initializers: { .base = {...}, .family = ..., .encoding = {...} }


	// 	Constructor for initialization (ADD THIS)
	// 		constexpr IPRDescriptor(const IPRDescriptorBase& base_, CPUFamily family_, const EV4Encoding& encoding_
	// 		) noexcept : base(base_), family(family_), encoding{ .ev4 = encoding_ }
	// 		{
	// 		}
	// 		constexpr IPRDescriptor(const IPRDescriptorBase& base_, CPUFamily family_, const EV5Encoding& encoding_
	// 		) noexcept : base(base_), family(family_), encoding{ .ev5 = encoding_ }
	// 		{
	// 		}
	// 		constexpr IPRDescriptor(const IPRDescriptorBase& base_, CPUFamily family_, const EV6Encoding& encoding_
	// 		) noexcept : base(base_), family(family_), encoding{ .ev6 = encoding_ }
	// 		{
	// 		}
	// 		// EV4 constructor
	// 		constexpr IPRDescriptor(const IPRDescriptorBase& b, const EV4Encoding& e) noexcept
	// 			: base(b)
	// 			, family(CPUFamily::EV4)
	// 			, encoding{ .ev4 = e }
	// 		{
	// 		}
	// 	
	// 		// EV5 constructor
	// 		constexpr IPRDescriptor(const IPRDescriptorBase& b, const EV5Encoding& e) noexcept
	// 			: base(b)
	// 			, family(CPUFamily::EV5)
	// 			, encoding{ .ev5 = e }
	// 		{
	// 		}

	// 	// EV6 constructor
	// 	constexpr IPRDescriptor(const IPRDescriptorBase& b, const EV6Encoding& e) noexcept
	// 		: base(b)
	// 		, family(CPUFamily::EV6)
	// 		, encoding{ .ev6 = e }
	// 	{
	// 	}



		// Helper: Get name
		inline const char* getName() const { return base.name; }

		// Helper: Get Tier1 alias
	/*	inline Tier1IPR getTier1Alias() const { return base.tier1Alias; }*/

		// Helper: Is this readable?
		inline bool isReadable() const {
			return base.access == AccessMode::RO ||
				base.access == AccessMode::RW;
		}

		// Helper: Is this writable?
		inline bool isWritable() const {
			return base.access == AccessMode::WO ||
				base.access == AccessMode::RW ||
				base.access == AccessMode::W1C ||
				base.access == AccessMode::W1S;
		}
	};

	using IPRDescriptorBase = IPRDescriptorBase;

	struct EV6Descriptor : IPRDescriptorBase {
		EV6Encoding ev6;
	};
// 	struct EV5Descriptor : IPRDescriptorBase {
// 	/*	EV5Encoding ev5;*/
// 	};

