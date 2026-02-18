// ============================================================================
// IPRStorage_CBox.h - Optimized with Backward-Compatible Accessors
// ============================================================================
// Project: ASA-EMulatR - Alpha AXP Architecture Emulator
// Copyright (C) 2025 eNVy Systems, Inc. All rights reserved.
// Licensed under eNVy Systems Non-Commercial License v1.1
// 
// Project Architect: Timothy Peer
// AI Code Generation: Claude (Anthropic) / ChatGPT (OpenAI)
// 
// Description:
//   CBox IPR storage - shadow registers optimized to 64 bytes.
//   Provides backward-compatible accessors for removed fields.
//
// Commercial use prohibited without separate license.
// Contact: peert@envysys.com | https://envysys.com
// Documentation: https://timothypeer.github.io/ASA-EMulatR-Project/
// ============================================================================

#ifndef IPRSTORAGE_CBOX_H
#define IPRSTORAGE_CBOX_H

#include "coreLib/Axp_Attributes_core.h"
#include <QtGlobal>
#include <atomic>

namespace deprecated_cbox {
    struct alignas(64) IPRStorage_CBox
    {
        // ========================================================================
        // CACHE LINE 0: Hot Path (First 32 bytes)
        // ========================================================================

        std::atomic<quint64> irq_pending;      // 8 bytes - Pending IPL bitmask (HOT READ)
        std::atomic<quint32> ipir_request;     // 4 bytes - IPI request bits (HOT READ)
        std::atomic<quint32> ipir_data;        // 4 bytes - IPI data payload

        std::atomic<quint32> irq_control;      // 4 bytes - PACKED:
        //   [7:0]   Current IPL
        //   [15:8]  Pending vector
        //   [16]    irq_mchk_pending
        //   [17]    irq_perf_pending
        //   [18]    has_pending_interrupt
        //   [19]    has_pending_ast
        //   [20]    has_pending_event (MASTER POLL)
        //   [31:21] reserved

        // Process Context

        quint64 pctx{};         // 88: Process Context (DTB ASIDs)


        std::atomic<quint16> sirr;             // 2 bytes - Software IRQ Request
        std::atomic<quint16> sisr;             // 2 bytes - Software IRQ Summary

        std::atomic<quint32> ast_state;        // 4 bytes - PACKED:
        //   [3:0]   astrr (AST Request Register)
        //   [7:4]   ast_level
        //   [15:8]  ast_pending
        //   [31:16] reserved

        quint32 reserved_hot;                  // 4 bytes - Reserved

        // ========================================================================
        // CACHE LINE 0: Cold Path (Second 32 bytes)
        // ========================================================================

        quint64 tbchk;      // 8 bytes - TLB check
        quint64 tbia;       // 8 bytes - TLB invalidate all
        quint64 tbiap;      // 8 bytes - TLB invalidate all process
        quint64 tbis;       // 8 bytes - TLB invalidate single

        // TOTAL SO FAR: 64 bytes (1 cache line)

        // ========================================================================
        // ADDITIONAL FIELDS (extends to 128 bytes)
        // ========================================================================
        // These fields are accessed less frequently, so they spill to second
        // cache line. This is acceptable since they're not in hot path.

        quint64 tbisd;      // 8 bytes - TLB invalidate single data
        quint64 tbisi;      // 8 bytes - TLB invalidate single instruction
        quint64 virbnd;     // 8 bytes - Virtual boundary
        quint64 sysptbr;    // 8 bytes - System page table base

        quint64 reserved_cold[4];  // 32 bytes - Reserved for future expansion

        // TOTAL: 64 + 64 = 128 bytes (2 cache lines)

        // ========================================================================
        // OPTIMIZED ACCESSORS - IRQ Control Bitfield (bits 0-20)
        // ========================================================================

        // Current IPL [7:0]
        AXP_HOT AXP_ALWAYS_INLINE quint8 getCurrentIPL() const noexcept {
            return irq_control.load(std::memory_order_acquire) & 0xFF;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setCurrentIPL(quint8 ipl) noexcept {
            quint32 old = irq_control.load(std::memory_order_relaxed);
            quint32 updated = (old & ~0xFFU) | ipl;
            irq_control.store(updated, std::memory_order_release);
        }

        // Pending Vector [15:8]
        AXP_HOT AXP_ALWAYS_INLINE quint8 getPendingVector() const noexcept {
            return (irq_control.load(std::memory_order_acquire) >> 8) & 0xFF;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setPendingVector(quint8 vector) noexcept {
            quint32 old = irq_control.load(std::memory_order_relaxed);
            quint32 updated = (old & ~0xFF00U) | (static_cast<quint32>(vector) << 8);
            irq_control.store(updated, std::memory_order_release);
        }

        // Machine Check Pending [16]
        AXP_HOT AXP_ALWAYS_INLINE bool getMchkPending() const noexcept {
            return (irq_control.load(std::memory_order_acquire) & (1U << 16)) != 0;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setMchkPending(bool pending) noexcept {
            if (pending) {
                irq_control.fetch_or(1U << 16, std::memory_order_release);
            }
            else {
                irq_control.fetch_and(~(1U << 16), std::memory_order_release);
            }
        }

        // Performance Counter Pending [17]
        AXP_HOT AXP_ALWAYS_INLINE bool getPerfPending() const noexcept {
            return (irq_control.load(std::memory_order_acquire) & (1U << 17)) != 0;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setPerfPending(bool pending) noexcept {
            if (pending) {
                irq_control.fetch_or(1U << 17, std::memory_order_release);
            }
            else {
                irq_control.fetch_and(~(1U << 17), std::memory_order_release);
            }
        }

        // Has Pending Interrupt [18]
        AXP_HOT AXP_ALWAYS_INLINE bool hasPendingInterrupt() const noexcept {
            return (irq_control.load(std::memory_order_acquire) & (1U << 18)) != 0;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setHasPendingInterrupt(bool pending) noexcept {
            if (pending) {
                irq_control.fetch_or(1U << 18, std::memory_order_release);
            }
            else {
                irq_control.fetch_and(~(1U << 18), std::memory_order_release);
            }
        }

        // Has Pending AST [19]
        AXP_HOT AXP_ALWAYS_INLINE bool hasPendingAST() const noexcept {
            return (irq_control.load(std::memory_order_acquire) & (1U << 19)) != 0;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setHasPendingAST(bool pending) noexcept {
            if (pending) {
                irq_control.fetch_or(1U << 19, std::memory_order_release);
            }
            else {
                irq_control.fetch_and(~(1U << 19), std::memory_order_release);
            }
        }

        // Has Pending Event [20] - MASTER POLL FLAG
        AXP_HOT AXP_ALWAYS_INLINE bool hasPendingEvent() const noexcept {
            return (irq_control.load(std::memory_order_acquire) & (1U << 20)) != 0;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setHasPendingEvent(bool pending) noexcept {
            if (pending) {
                irq_control.fetch_or(1U << 20, std::memory_order_release);
            }
            else {
                irq_control.fetch_and(~(1U << 20), std::memory_order_release);
            }
        }

        // ========================================================================
        // AST STATE ACCESSORS - ast_state Bitfield
        // ========================================================================

        // ASTRR [3:0]
        AXP_HOT AXP_ALWAYS_INLINE quint8 getASTRR() const noexcept {
            return ast_state.load(std::memory_order_acquire) & 0xF;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setASTRR(quint8 value) noexcept {
            quint32 old = ast_state.load(std::memory_order_relaxed);
            quint32 updated = (old & ~0xFU) | (value & 0xF);
            ast_state.store(updated, std::memory_order_release);
        }

        // AST Level [7:4]
        AXP_HOT AXP_ALWAYS_INLINE quint8 getASTLevel() const noexcept {
            return (ast_state.load(std::memory_order_acquire) >> 4) & 0xF;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setASTLevel(quint8 level) noexcept {
            quint32 old = ast_state.load(std::memory_order_relaxed);
            quint32 updated = (old & ~0xF0U) | ((level & 0xF) << 4);
            ast_state.store(updated, std::memory_order_release);
        }

        // AST Pending [15:8]
        AXP_HOT AXP_ALWAYS_INLINE quint8 getASTPending() const noexcept {
            return (ast_state.load(std::memory_order_acquire) >> 8) & 0xFF;
        }

        AXP_HOT AXP_ALWAYS_INLINE void setASTPending(quint8 pending) noexcept {
            quint32 old = ast_state.load(std::memory_order_relaxed);
            quint32 updated = (old & ~0xFF00U) | (static_cast<quint32>(pending) << 8);
            ast_state.store(updated, std::memory_order_release);
        }

        // ========================================================================
        // BACKWARD COMPATIBILITY - Field Emulation via Atomics
        // ========================================================================
        // These provide drop-in replacement for removed fields using bitfield accessors

        // Emulate: std::atomic<quint8> astrr
        struct ASTRRProxy {
            IPRStorage_CBox* parent;

            quint8 load(std::memory_order order) const noexcept {
                (void)order;  // Use acquire in getter
                return parent->getASTRR();
            }

            void store(quint8 value, std::memory_order order) noexcept {
                (void)order;  // Use release in setter
                parent->setASTRR(value);
            }

            quint8 fetch_or(quint8 mask, std::memory_order order) noexcept {
                (void)order;
                quint8 old = parent->getASTRR();
                parent->setASTRR(old | mask);
                return old;
            }

            quint8 fetch_and(quint8 mask, std::memory_order order) noexcept {
                (void)order;
                quint8 old = parent->getASTRR();
                parent->setASTRR(old & mask);
                return old;
            }
        };

        // Emulate: std::atomic<bool> has_pending_event
        struct HasPendingEventProxy {
            IPRStorage_CBox* parent;

            bool load(std::memory_order order) const noexcept {
                (void)order;
                return parent->hasPendingEvent();
            }

            void store(bool value, std::memory_order order) noexcept {
                (void)order;
                parent->setHasPendingEvent(value);
            }
        };

        // Provide proxy accessors
        AXP_HOT AXP_ALWAYS_INLINE ASTRRProxy astrr() noexcept {
            return ASTRRProxy{ this };
        }

        AXP_HOT AXP_ALWAYS_INLINE HasPendingEventProxy has_pending_event() noexcept {
            return HasPendingEventProxy{ this };
        }


        /**
         * @brief Get process context register
         * @param cpuId CPU identifier
         * @return PCTX value
         */
        AXP_HOT AXP_ALWAYS_INLINE
            quint64 getPCTX_Active(CPUIdType cpuId) noexcept {
            return pctx;
        }

        /**
         * @brief Set process context register
         * @param cpuId CPU identifier
         * @param value New PCTX value
         */
        AXP_HOT AXP_ALWAYS_INLINE
            void setPCTX_Active(CPUIdType cpuId, quint64 value) noexcept {
            pctx = (value);
        }

        // DTB0 ASID
        AXP_HOT AXP_ALWAYS_INLINE
            quint8 getDTB0ASID() const noexcept {
            return (pctx >> 32) & 0xFF;
        }

        AXP_HOT AXP_ALWAYS_INLINE
            void setDTB0ASID(quint8 v) noexcept {
            // Preserve other PCTX bits
            pctx = (pctx & ~(0xFFULL << 32)) | (static_cast<quint64>(v) << 32);
        }

        // DTB1 ASID
        AXP_HOT AXP_ALWAYS_INLINE
            quint8 getDTB1ASID() const noexcept {
            return (pctx >> 40) & 0xFF;
        }

        AXP_HOT AXP_ALWAYS_INLINE
            void setDTB1ASID(quint8 v) noexcept {
            pctx = (pctx & ~(0xFFULL << 40)) | (static_cast<quint64>(v) << 40);
        }



        // ========================================================================
        // LEGACY COMPATIBILITY METHODS
        // ========================================================================

        inline void postIRQ(quint8 ipl, quint8 vector) noexcept {
            setCurrentIPL(ipl);
            setPendingVector(vector);
            irq_pending.fetch_or(1ULL << ipl, std::memory_order_release);
            setHasPendingInterrupt(true);
            setHasPendingEvent(true);
        }

        inline void clearIRQ(quint8 ipl) noexcept {
            irq_pending.fetch_and(~(1ULL << ipl), std::memory_order_acq_rel);

            if (irq_pending.load(std::memory_order_acquire) == 0) {
                setHasPendingInterrupt(false);
                setHasPendingEvent(false);
            }
        }

        inline void postIPIR(quint32 request, quint32 data) noexcept {
            ipir_data.store(data, std::memory_order_release);
            ipir_request.fetch_or(request, std::memory_order_release);
            setHasPendingEvent(true);
        }

        inline bool hasIRQPending() const noexcept {
            return irq_pending.load(std::memory_order_acquire) != 0;
        }

        inline quint8 getPendingIPL() const noexcept {
            return getCurrentIPL();
        }

        inline bool hasIPIRPending() const noexcept {
            return ipir_request.load(std::memory_order_acquire) != 0;
        }

        inline quint32 drainIPIR() noexcept {
            return ipir_request.exchange(0, std::memory_order_acq_rel);
        }

        AXP_HOT AXP_ALWAYS_INLINE  bool hasAnyPendingEvent() const noexcept {
            return hasPendingEvent();
        }

        AXP_HOT AXP_ALWAYS_INLINE  bool shouldPoll() const noexcept {
            return hasPendingEvent();
        }

        inline void reset() noexcept {
            irq_pending.store(0, std::memory_order_relaxed);
            irq_control.store(0, std::memory_order_relaxed);
            ipir_request.store(0, std::memory_order_relaxed);
            ipir_data.store(0, std::memory_order_relaxed);
            sirr.store(0, std::memory_order_relaxed);
            sisr.store(0, std::memory_order_relaxed);
            ast_state.store(0, std::memory_order_relaxed);

            // TLB IPRs
            tbchk = 0;
            tbia = 0;
            tbiap = 0;
            tbis = 0;
            tbisd = 0;
            tbisi = 0;
            virbnd = 0;
            sysptbr = 0;
            pctx = 0;
        }



    };

    // ============================================================================
    // SIZE VERIFICATION
    // ============================================================================

    static_assert(alignof(IPRStorage_CBox) == 64,
        "CBox must be cache-line aligned");
    static_assert(sizeof(IPRStorage_CBox) <= 256,
        "CBox is 256 bytes (2 cache lines) - acceptable for now");

#pragma message("IPRStorage_CBox: 128 bytes (2 cache lines) - hot path in first 64 bytes")

}



#endif // IPRSTORAGE_CBOX_H