#pragma once
#include <QtGlobal>


enum class MachineCheckReason : quint8 {
	NONE = 0x0000,
	PROCESSOR_ERROR,                  ///< General CPU logic error
	SYSTEM_ERROR,                     ///< System-level fault
	SYSTEM_CORRECTABLE_ERROR,         ///< Correctable system error 
	PROCESSOR_CORRECTABLE_ERROR,		  ///< Correctable machine error 

	// Cache Errors
	ICACHE_PARITY_ERROR,              ///< Instruction cache parity
	DCACHE_PARITY_ERROR,              ///< Data cache parity
	BCACHE_ERROR,                     ///< Backup cache error
	SCACHE_ERROR,                     ///< Secondary cache error
	CACHE_TAG_ERROR,                  ///< Cache tag parity/ECC error
	CACHE_COHERENCY_ERROR,            ///< Multi-processor cache coherency violation

	// Memory Errors
	SYSTEM_MEMORY_ERROR,              ///< Main memory uncorrectable error
	MEMORY_CONTROLLER_ERROR,          ///< Memory controller fault
	CORRECTABLE_ERROR,                ///< Correctable memory error (ECC)
	UNCORRECTABLE_ERROR,              ///< Uncorrectable memory error
	BUFFER_WRITE_ERROR,

	// System Bus and I/O
	SYSTEM_BUS_ERROR,                 ///< System bus transaction error
	IO_BUS_ERROR,                     ///< I/O bus error
	EXTERNAL_INTERFACE_ERROR,         ///< External interface fault
	MEMORY_BUS_ERROR, 
	// Processor Internal
	EXECUTION_UNIT_ERROR,             ///< ALU/FPU execution error
	REGISTER_FILE_ERROR,              ///< Register file parity error
	PIPELINE_ERROR,                   ///< Pipeline state corruption
	CONTROL_LOGIC_ERROR,              ///< Control logic fault



	// MMU and TLB
	MMU_ERROR,                        ///< Memory management unit error
	TRANSLATION_BUFFER_ERROR,         ///< TLB error
	PAGE_FAULT,
	TLB_INSERTION_FAILURE,
	// Multi-processor
	INTERPROCESSOR_ERROR,             ///< Inter-processor communication error

	// Environmental
	THERMAL_ERROR,                    ///< Over-temperature condition
	POWER_SUPPLY_ERROR,               ///< Power supply fault
	CLOCK_ERROR,                      ///< Clock generation/distribution error

	// PALcode
	PALCODE_ERROR,                    ///< PALcode execution error

	// Critical
	DOUBLE_MACHINE_CHECK,             ///< Machine check during machine check handler
	SMP_BARRIER_TIMEOUT,			  /// SMP barrier ACK fault.  
	UNKNOWN_MACHINE_CHECK
};