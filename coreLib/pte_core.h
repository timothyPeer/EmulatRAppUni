// ============================================================================
// pte_core.h - include/alpha_pte_core.h
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

#ifndef _EMULATRAPPUNI_CORELIB_PTE_CORE_H
#define _EMULATRAPPUNI_CORELIB_PTE_CORE_H

#pragma once
// include/alpha_pte_core.h

#include <bit>
#include <QtGlobal>
#include <type_traits>



// 	struct PTETraits {
// 		enum class AccessPermission {
// 			NONE,
// 			READ,
// 			WRITE,
// 			EXECUTE
// 		};



// Cache replacement policy when cache lines must be evicted
enum class ReplacementPolicy {
	MRU,     // Most Recently Used
	LRU,     // Least Recently Used (most common, good performance)
	LFU,     // Least Frequently Used (good for specific workloads)
	RANDOM,  // Random replacement (simple, unpredictable)
	FIFO     // First In, First Out (simple but can cause thrashing)
};

/*
Stage 0-1 (Fetch)
- Always ITB realm
- Even when sequential or predicted
Stage 3-4 (Execute load/store)
- DTB realm
- Load queue / store queue stage determines request type
*/
// PTE realm is a pipeline state.

//enum class Realm : quint8 {
//	Instruction = 0x1,   // ITB
//	Data = 0x2    // DTB
//};


// ============================================================================
// constexpr bitfield extract/insert helpers
// Template parameters:
//    Bit     = starting bit position (0 = LSB)
//    Width   = number of bits in the field
// ============================================================================

// Extract a bitfield from a 64-bit integer.
template<unsigned Bit, unsigned Width>
[[nodiscard]] constexpr quint64 extractField(quint64 value) noexcept
{
	static_assert(Bit < 64, "Bit out of range");
	static_assert(Width > 0, "Width must be > 0");
	static_assert(Bit + Width <= 64, "Bit+Width exceeds 64");

	constexpr quint64 mask =
		(Width == 64 ? ~0ull : ((1ull << Width) - 1ull));

	return (value >> Bit) & mask;
}

// Insert a bitfield into a 64-bit integer.
template<unsigned Bit, unsigned Width>
constexpr void insertField(quint64& target, quint64 fieldValue) noexcept
{
	static_assert(Bit < 64, "Bit out of range");
	static_assert(Width > 0, "Width must be > 0");
	static_assert(Bit + Width <= 64, "Bit+Width exceeds 64");

	constexpr quint64 mask =
		(Width == 64 ? ~0ull : ((1ull << Width) - 1ull));

	target &= ~(mask << Bit);           // clear destination field
	target |= ((fieldValue & mask) << Bit);
}

// Convenience wrappers with the names you already use:
template<unsigned Bit, unsigned Width>
[[nodiscard]] constexpr quint64 extract(quint64 value) noexcept
{
	return extractField<Bit, Width>(value);
}

#endif
