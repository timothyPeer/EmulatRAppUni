#ifndef PIPELINESLOT_INL_H
#define PIPELINESLOT_INL_H
#include <QtGlobal>

#include "coreLib/Axp_Attributes_core.h"
#include "coreLib/FetchResult.h"
#include "machineLib/PipeLineSlot.h"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/DecodedInstruction_inl.h"
#include "coreLib/ExecTrace.h"

AXP_HOT AXP_ALWAYS_INLINE void ExecTraceCommitSlot(const PipelineSlot& slot) noexcept
{
    const InstructionGrain* g = slot.di.grain; // or slot.grain if that's your canonical pointer

    const quint8  op   = g ? g->opcode() : 0;
    const quint16 func = g ? g->functionCode() : 0;
	const quint32 raw = slot.di.rawBits();
    const QByteArray mneUtf8 = g ? g->mnemonic().toUtf8() : QByteArray("UNKNOWN");
	const QString mnemonic = getMnemonicFromRaw(raw);
    const auto gt = g ? g->grainType() : GrainType::Unknown; // adjust to your enum
    const char* gtName = g ? getGrainTypeName(gt) : "UNKNOWN";

    /*ExecTrace::recordCommitWithGrain(
        slot.cpuId,
        slot.di.pc,
        slot.di.rawBits(),               // or slot.instructionWord if that’s the canonical 32-bit raw
        op,
        func,
        mneUtf8.constData(),
        gtName,
        static_cast<quint8>(gt),
        (g != nullptr),
        &slot                      // IMPORTANT: lets ExecTrace emit asm if configured
        );*/

	ExecTrace::recordCommitAsAssembly(slot.cpuId, slot.di.pc, raw, mnemonic.toUtf8().constData(), slot);
}

// ================================================================
// BRANCH INSTRUCTION HELPER
// ================================================================
inline void debugBranch(const char* stageName, const PipelineSlot& slot,
						bool taken, quint64 target, quint64 predicted)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	const quint32 raw = slot.di.rawBits();
	const QString mnemonic = getMnemonicFromRaw(raw);

	QString msg = QString("[%1::BRANCH] %2")
		.arg(stageName)
		.arg(mnemonic);

	msg += QString(" | PC: 0x%1")
		.arg(slot.di.pc, 16, 16, QChar('0'));

	msg += QString(" | Ra: R%1").arg(slot.di.ra);

	if (taken) {
		msg += QString(" | TAKEN -> 0x%1")
			.arg(target, 16, 16, QChar('0'));

		if (predicted != 0) {
			if (target == predicted) {
				msg += " v/";
			}
			else {
				msg += QString(" | MISPREDICT (was: 0x%1) x")
					.arg(predicted, 16, 16, QChar('0'));
			}
		}
	}
	else {
		msg += " | NOT TAKEN";
	}
	
//	ExecTrace::recordCommitAsAssembly(slot.cpuId, slot.di.pc, raw, mnemonic.toUtf8().constData(), slot);

	qDebug().noquote() << msg;
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
	Q_UNUSED(taken);
	Q_UNUSED(target);
	Q_UNUSED(predicted);
#endif
}

// ================================================================
// INTEGER INSTRUCTION HELPER
// ================================================================

inline void debugInteger(const char* stageName, const PipelineSlot& slot,
						 quint64 operand1, quint64 operand2, quint64 result,
						 const QString& operation)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	const quint32 raw = slot.di.rawBits();
	const QString mnemonic = getMnemonicFromRaw(raw);
	const bool usesLiteral = (raw >> 12) & 0x1;

	QString msg = QString("[%1::INTEGER] %2 | PC: 0x%3")
		.arg(stageName)
		.arg(mnemonic)
		.arg(slot.di.pc, 16, 16, QChar('0'));

	msg += QString(" | Ra: R%1").arg(slot.di.ra);

	if (usesLiteral) {
		msg += QString(" | Literal: %1 (0x%2)")
			.arg(slot.di.literal_val)
			.arg(slot.di.literal_val, 2, 16, QChar('0'));
	}
	else {
		msg += QString(" | Rb: R%1").arg(slot.di.rb);
	}

	msg += QString(" | Rc: R%1").arg(slot.di.rc);
	msg += QString(" | %1: 0x%2 %3 0x%4 = 0x%5")
		.arg(operation)
		.arg(operand1, 16, 16, QChar('0'))
		.arg(usesLiteral ? "+" : "op")
		.arg(operand2, 16, 16, QChar('0'))
		.arg(result, 16, 16, QChar('0'));

	// Also show decimal for small values
	if (result < 1000000) {
		msg += QString(" (%1)").arg(result);
	}

//	ExecTrace::recordCommitAsAssembly(slot.cpuId, slot.di.pc, raw, mnemonic.toUtf8().constData(), slot);

	qDebug().noquote() << msg;
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
	Q_UNUSED(operand1);
	Q_UNUSED(operand2);
	Q_UNUSED(result);
	Q_UNUSED(operation);
#endif
}

// ================================================================
// FLOATING-POINT INSTRUCTION HELPER
// ================================================================

inline void debugFloat(const char* stageName, const PipelineSlot& slot,
					   quint64 operand1, quint64 operand2, quint64 result,
					   const QString& operation)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	const quint32 raw = slot.di.rawBits();
	const QString mnemonic = getMnemonicFromRaw(raw);
	const quint16 funcCode = getFunctionCode(slot.di);

	QString msg = QString("[%1::FLOAT] %2 | PC: 0x%3")
		.arg(stageName)
		.arg(mnemonic)
		.arg(slot.di.pc, 16, 16, QChar('0'));

	msg += QString(" | Fa: F%1").arg(slot.di.ra);
	msg += QString(" | Fb: F%1").arg(slot.di.rb);
	msg += QString(" | Fc: F%1").arg(slot.di.rc);
	msg += QString(" | Func: 0x%1").arg(funcCode, 3, 16, QChar('0'));

	msg += QString(" | %1:").arg(operation);
	msg += QString(" 0x%1 op 0x%2 = 0x%3")
		.arg(operand1, 16, 16, QChar('0'))
		.arg(operand2, 16, 16, QChar('0'))
		.arg(result, 16, 16, QChar('0'));

	// Interpret as doubles for readability
	double d1, d2, dr;
	memcpy(&d1, &operand1, sizeof(double));
	memcpy(&d2, &operand2, sizeof(double));
	memcpy(&dr, &result, sizeof(double));

	msg += QString(" | (%.6g op %.6g = %.6g)").arg(d1).arg(d2).arg(dr);


	//ExecTrace::recordCommitAsAssembly(slot.cpuId, slot.di.pc, raw, mnemonic.toUtf8().constData(), slot);

	qDebug().noquote() << msg;
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
	Q_UNUSED(operand1);
	Q_UNUSED(operand2);
	Q_UNUSED(result);
	Q_UNUSED(operation);
#endif
}

// ================================================================
// MEMORY INSTRUCTION HELPER
// ================================================================

inline void debugMemory(const char* stageName, const PipelineSlot& slot,
						bool isLoad, quint64 address, quint64 value,
						quint8 size)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	const quint32 raw = slot.di.rawBits();
	const QString mnemonic = getMnemonicFromRaw(raw);
	const qint32 disp = static_cast<qint16>(raw & 0xFFFF);  // Sign-extend 16-bit

	QString msg = QString("[%1::MEMORY] %2 | PC: 0x%3")
		.arg(stageName)
		.arg(mnemonic)
		.arg(slot.di.pc, 16, 16, QChar('0'));

	msg += QString(" | Ra: R%1").arg(slot.di.ra);
	msg += QString(" | Rb: R%1").arg(slot.di.rb);
	msg += QString(" | Disp: %1 (0x%2)")
		.arg(disp)
		.arg(static_cast<quint16>(disp), 4, 16, QChar('0'));

	msg += QString(" | Addr: 0x%1").arg(address, 16, 16, QChar('0'));

	const char* sizeStr = (size == 1) ? "byte" :
		(size == 2) ? "word" :
		(size == 4) ? "long" :
		(size == 8) ? "quad" : "???";

	if (isLoad) {
		msg += QString(" | LOAD %1: 0x%2")
			.arg(sizeStr)
			.arg(value, size * 2, 16, QChar('0'));

		// Show decimal for reasonable values
		if (size <= 4 && value < 1000000) {
			msg += QString(" (%1)").arg(value);
		}
	}
	else {
		msg += QString(" | STORE %1: 0x%2")
			.arg(sizeStr)
			.arg(value, size * 2, 16, QChar('0'));
	}

	// Check for unaligned access
	if (address % size != 0) {
		msg += " |  UNALIGNED ACCESS";
	}

	//ExecTrace::recordCommitAsAssembly(slot.cpuId, slot.di.pc, raw, mnemonic.toUtf8().constData(), slot);
	qDebug().noquote() << msg;
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
	Q_UNUSED(isLoad);
	Q_UNUSED(address);
	Q_UNUSED(value);
	Q_UNUSED(size);
#endif
}

inline void debugJump(const char* stageName, const PipelineSlot& slot,
					  quint8 targetReg, quint64 regValue, quint64 target,
					  quint64 predicted)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	const quint32 raw = slot.di.rawBits();
	const QString mnemonic = getMnemonicFromRaw(raw);
	const quint16 hint = (raw >> 14) & 0x3FFF;  // 14-bit hint field

	QString msg = QString("[%1::JUMP] %2")
		.arg(stageName)
		.arg(mnemonic);

	msg += QString(" | PC: 0x%1")
		.arg(slot.di.pc, 16, 16, QChar('0'));

	msg += QString(" | Ra: R%1").arg(slot.di.ra);

	msg += QString(" | Rb: R%1 (0x%2)")
		.arg(targetReg)
		.arg(regValue, 16, 16, QChar('0'));

	msg += QString(" | Target: (R%1 & ~3) = 0x%2")
		.arg(targetReg)
		.arg(target, 16, 16, QChar('0'));

	if (hint != 0) {
		msg += QString(" | Hint: 0x%1")
			.arg(hint, 4, 16, QChar('0'));
	}

	if (predicted != 0) {
		if (target == predicted) {
			msg += " v/";
		}
		else {
			msg += QString(" | MISPREDICT (was: 0x%1) x")
				.arg(predicted, 16, 16, QChar('0'));
		}
	}

	//ExecTrace::recordCommitAsAssembly(slot.cpuId, slot.di.pc, raw, mnemonic.toUtf8().constData(), slot);
	qDebug().noquote() << msg;
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
	Q_UNUSED(targetReg);
	Q_UNUSED(regValue);
	Q_UNUSED(target);
	Q_UNUSED(predicted);
#endif
}

#pragma region Debugging Scaffolding 





inline void debugTickEntry(const FetchResult& fr)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE::TICK] valid:" << fr.valid
		<< "PC:" << Qt::hex << Qt::showbase << fr.di.pc
		<< "raw:" << fr.di.rawBits()
		<< "opcode:" << ((fr.di.rawBits() >> 26) & 0x3F);
#else
	Q_UNUSED(fr);
#endif
}

inline void debugStageTransition(const char* fromStage,  const char* toStage,
	quint64 pc, bool valid)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE::STAGE]" << fromStage << "->" << toStage
		<< "PC:" << Qt::hex << Qt::showbase << pc
		<< "valid:" << valid;
#else
	Q_UNUSED(fromStage);
	Q_UNUSED(toStage);
	Q_UNUSED(pc);
	Q_UNUSED(valid);
#endif
}

inline void debugExecutionEntry(const DecodedInstruction& di)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	const quint32 raw = di.rawBits();
	const quint8 opcode = (raw >> 26) & 0x3F;

	qDebug() << "[PIPELINE::EXEC] PC:" << Qt::hex << Qt::showbase << di.pc
		<< "opcode:" << opcode
		<< "mnemonic:" << getMnemonicFromRaw(raw)
		<< "Ra:" << di.ra << "Rb:" << di.rb << "Rc:" << di.rc
		<< "semantics:" << Qt::hex << (di.semantics & 0xFFFFFFFF);
#else
	Q_UNUSED(di);
#endif
}


inline void debugPipelineStall(const char* reason, quint64 pc)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE::STALL]" << reason
		<< "PC:" << Qt::hex << Qt::showbase << pc;
#else
	Q_UNUSED(reason);
	Q_UNUSED(pc);
#endif
}



inline void debugRetirement(const DecodedInstruction& di, bool committed)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << QString("[PIPELINE::RETIRE] PC: 0x%1 -- Committed: %2 -- Mneumonic: %3")
		.arg(di.pc, 16, 16, QChar('0'))
		.arg(committed)
		.arg(getMnemonicFromRaw(di.rawBits()));

#else
	Q_UNUSED(di);
	Q_UNUSED(committed);
#endif
}

inline void debugRegisterWrite(quint8 reg, quint64 value, const char* stage)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	if (reg != 31) {  // Don't spam for R31 writes
		qDebug() << "[PIPELINE::REG]" << stage
			<< "R" << reg << "<-" << Qt::hex << Qt::showbase << value;
	}
#else
	Q_UNUSED(reg);
	Q_UNUSED(value);
	Q_UNUSED(stage);
#endif
}
inline void debugPCUpdate(quint64 oldPC, quint64 newPC, const char* reason)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE::PC]" << reason
		<< "PC:" << Qt::hex << Qt::showbase << oldPC
		<< "->" << newPC;
#else
	Q_UNUSED(oldPC);
	Q_UNUSED(newPC);
	Q_UNUSED(reason);
#endif
}



inline void debugStageEntry(const char* stageName, const PipelineSlot& slot)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	if (slot.valid) {
		qDebug() << "[" << stageName << "::ENTRY] PC:" << Qt::hex << Qt::showbase << slot.di.pc
			<< "opcode:" << ((slot.di.rawBits() >> 26) & 0x3F)
			<< "mnemonic:" << getMnemonicFromRaw(slot.di.rawBits());
	}
	else {
		qDebug() << "[" << stageName << "::ENTRY] BUBBLE (invalid slot)";
	}
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
#endif
}

inline void debugStageExit(const char* stageName, const PipelineSlot& slot)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	if (slot.valid) {
		qDebug() << "[" << stageName << "::EXIT] PC:" << Qt::hex << Qt::showbase << slot.di.pc
			<< "needsWriteback:" << slot.needsWriteback
			<< "writeRa:" << slot.writeRa
			<< "branchTaken:" << slot.branchTaken;
	}
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
#endif
}

inline void debugBranchTaken(const PipelineSlot& slot)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[BRANCH] PC:" << Qt::hex << Qt::showbase << slot.di.pc
		<< "-> TARGET:" << slot.branchTarget
		<< "predicted:" << slot.predictionTarget
		<< "misprediction:" << (slot.branchTarget != slot.predictionTarget);
#else
	Q_UNUSED(slot);
#endif
}

inline void debugPipelineFlush(const char* reason, quint64 flushPC)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE::FLUSH]" << reason << "at PC:" << Qt::hex << Qt::showbase << flushPC;
	qDebug() << "  *** FLUSHING IF, ID, RR stages ***";
#else
	Q_UNUSED(reason);
	Q_UNUSED(flushPC);
#endif
}



inline void debugMemoryAccess(const char* type, quint64 addr, quint64 value)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[MEM::" << type << "] addr:" << Qt::hex << Qt::showbase << addr
		<< "value:" << value;
#else
	Q_UNUSED(type);
	Q_UNUSED(addr);
	Q_UNUSED(value);
#endif
}

inline void debugSlotState(const PipelineSlot& slot, const char* location)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[SLOT@" << location << "]"
		<< "valid:" << slot.valid
		<< "PC:" << Qt::hex << Qt::showbase << slot.di.pc
		<< "needsWB:" << slot.needsWriteback
		<< "writeRa:" << slot.writeRa
		<< "Ra:" << slot.di.ra
		<< "payload:" << slot.payLoad;
#else
	Q_UNUSED(slot);
	Q_UNUSED(location);
#endif
}


inline void debugLog(const QString& message)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE]" << message;
#else
	Q_UNUSED(message);
#endif
}

// Stage-specific message
inline void debugLogStage(const char* stageName, const QString& message)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[" << stageName << "]" << message;
#else
	Q_UNUSED(stageName);
	Q_UNUSED(message);
#endif
}

// Slot with message (shows PC, opcode, mnemonic + custom message)
inline void debugLogSlot(const char* stageName, const PipelineSlot& slot, const QString& message)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	if (slot.valid) {
		const quint32 raw = slot.di.rawBits();
		const quint8 opcode = (raw >> 26) & 0x3F;
		qDebug() << "[" << stageName << "]" << message
			<< "| PC:" << Qt::hex << Qt::showbase << slot.di.pc
			<< "opcode:" << opcode
			<< getMnemonicFromRaw(raw);
	}
	else {
		qDebug() << "[" << stageName << "]" << message << "| BUBBLE (invalid slot)";
	}
#else
	Q_UNUSED(stageName);
	Q_UNUSED(slot);
	Q_UNUSED(message);
#endif
}

// PC-focused log
inline void debugLogPC(const QString& message, quint64 pc)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[PIPELINE]" << message << "PC:" << Qt::hex << Qt::showbase << pc;
#else
	Q_UNUSED(message);
	Q_UNUSED(pc);
#endif
}

// Branch-specific log
inline void debugLogBranch(const QString& message, quint64 pc, quint64 target)  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "[BRANCH]" << message
		<< "| PC:" << Qt::hex << Qt::showbase << pc
		<< "-> TARGET:" << target;
#else
	Q_UNUSED(message);
	Q_UNUSED(pc);
	Q_UNUSED(target);
#endif
}

// Visual separator for cycle boundaries
inline void debugSeparator()  noexcept
{
#ifdef AXP_DEBUG_PIPELINE
	qDebug() << "================================================================================";
#endif
}
#pragma endregion Debugging Scaffolding


#endif // PIPELINESLOT_INL_H
