// ============================================================================
// PciScsiScriptEngine.H  -  Abstract Microcode / Script Engine for PCI SCSI
// ============================================================================
// Purpose:
//   Many PCI SCSI controllers (NCR 53C8xx, Adaptec AIC-7xxx, QLogic ISP series)
//   include a programmable engine that executes a small instruction set,
//   referred to as:
//
//     • "SCRIPTS" (NCR 53C810/815/825/875)
//     • "Sequencer" (Adaptec AIC controllers)
//     • "RISC Engine" or "Command Processor" (QLogic)
//
//   This header defines a device-agnostic abstraction of such a programmable
//   micro-engine. It does NOT define any specific instruction set. Instead it
//   provides:
//
//     - Program memory storage (bytecode or words)
//     - Instruction fetch state (PC, nextPC)
//     - Execution hooks to be implemented by concrete controllers
//     - Ability to stop/start/reset the engine
//     - A thread-safe control mechanism if desired
//
// Design constraints:
//   - Header-only
//   - Depends only on QtCore (QtGlobal, QVector, QMutex)
//   - Pure ASCII (UTF-8, no BOM)
//   - NO dependency on SafeMemory, MMIOManager, or AlphaCPU
//
// Intended usage:
//   Derived devices (e.g., NCR53C8xxPciController) subclass the engine and
//   implement:
//
//       executeOneInstruction()
//       decodeInstruction(...)
//       fetchInstruction(...)
//       onEngineStart()
//       onEngineStop()
//       onEngineReset()
//
//   They load program memory from registers (DMA pointers or host writes).
//
// ============================================================================

#ifndef PCI_SCSI_SCRIPT_ENGINE_H
#define PCI_SCSI_SCRIPT_ENGINE_H

#include <QtGlobal>
#include <QVector>
#include <QMutex>

// ============================================================================
// PciScsiScriptEngineState
// ============================================================================
enum class PciScsiScriptEngineState : quint8
{
	Stopped = 0,
	Running,
	Halted,
	Error
};

// ============================================================================
// PciScsiScriptEngine
// ============================================================================
class PciScsiScriptEngine
{
public:
	explicit PciScsiScriptEngine(bool threadSafe = false) noexcept
		: m_pc(0)
		, m_nextPc(0)
		, m_state(PciScsiScriptEngineState::Stopped)
		, m_threadSafe(threadSafe)
	{
	}

	virtual ~PciScsiScriptEngine() noexcept = default;

	// ------------------------------------------------------------------------
	// Program memory management
	// ------------------------------------------------------------------------

	inline void clearProgram() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_program.clear();
		m_pc = 0;
		m_nextPc = 0;
	}

	inline void loadProgram(const QVector<quint32>& words) noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_program = words;
		m_pc = 0;
		m_nextPc = 0;
	}

	inline const QVector<quint32>& program() const noexcept
	{
		QMutexLocker locker(m_threadSafe ? const_cast<QMutex*>(&m_mutex) : nullptr);
		return m_program;
	}

	// ------------------------------------------------------------------------
	// Execution control
	// ------------------------------------------------------------------------

	inline void start() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (m_program.isEmpty())
		{
			m_state = PciScsiScriptEngineState::Error;
			return;
		}

		m_state = PciScsiScriptEngineState::Running;
		m_pc = 0;
		m_nextPc = 0;
		onEngineStart();
	}

	inline void stop() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_state = PciScsiScriptEngineState::Stopped;
		onEngineStop();
	}

	inline void halt() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_state = PciScsiScriptEngineState::Halted;
	}

	inline void reset() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);
		m_pc = 0;
		m_nextPc = 0;
		m_state = PciScsiScriptEngineState::Stopped;
		onEngineReset();
	}

	inline bool isRunning() const noexcept
	{
		return m_state == PciScsiScriptEngineState::Running;
	}

	inline PciScsiScriptEngineState state() const noexcept
	{
		return m_state;
	}

	// ------------------------------------------------------------------------
	// Execution step
	// ------------------------------------------------------------------------
	//
	// Concrete devices implement:
	//     executeOneInstruction()
	//     decodeInstruction(word, ...)
	//     fetchInstruction(pc)
	//
	// A real controller's PCI/MMIO layer will call step() during:
	//     - MMIO writes to doorbells
	//     - DMA kicks
	//     - Timer-driven polling
	//
	// ------------------------------------------------------------------------

	inline void step() noexcept
	{
		QMutexLocker locker(m_threadSafe ? &m_mutex : nullptr);

		if (m_state != PciScsiScriptEngineState::Running)
			return;

		if (m_pc >= static_cast<quint64>(m_program.size()))
		{
			m_state = PciScsiScriptEngineState::Halted;
			return;
		}

		const quint32 instr = m_program.at(static_cast<int>(m_pc));
		m_nextPc = m_pc + 1;

		executeOneInstruction(instr);

		m_pc = m_nextPc;
	}

protected:
	// ------------------------------------------------------------------------
	// Microcode engine hooks
	// ------------------------------------------------------------------------
	//
	// These must be implemented by actual controller-specific script engines.
	// ------------------------------------------------------------------------

	virtual void executeOneInstruction(quint32 instruction) noexcept = 0;
	virtual void onEngineStart() noexcept {}
	virtual void onEngineStop()  noexcept {}
	virtual void onEngineReset() noexcept {}

	// ------------------------------------------------------------------------
	// Protected helpers
	// ------------------------------------------------------------------------

	inline void jumpTo(quint64 newPc) noexcept
	{
		m_nextPc = newPc;
	}

	inline quint64 pc() const noexcept
	{
		return m_pc;
	}

	inline void setPc(quint64 newPc) noexcept
	{
		m_pc = newPc;
	}

private:
	QVector<quint32> m_program;
	quint64          m_pc;
	quint64          m_nextPc;

	PciScsiScriptEngineState m_state;

	bool           m_threadSafe;
	mutable QMutex m_mutex;
};

#endif // PCI_SCSI_SCRIPT_ENGINE_H
