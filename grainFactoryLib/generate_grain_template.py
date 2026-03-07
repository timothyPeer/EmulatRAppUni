def generate_grain_template(grain, box_dir):
    """Generate complete grain header file content with CPU trace hooks."""

    opcode = grain['Opcode']
    function = grain['Function']
    mnemonic = grain['Mnemonic']
    description = grain['Description']
    box = grain['Box']
    grain_type = grain['Type']

    class_name = get_grain_class_name(mnemonic)
    header_guard = generate_header_guard(mnemonic)
    box_header = BOX_HEADERS[box]
    execute_method = get_execute_method_name(box, mnemonic)
    filename = get_grain_filename(mnemonic)
    box_member = get_box_member_name(box)

    # Generate flags, latency, throughput
    flags = generate_grain_flags(grain)
    latency = get_latency(mnemonic, opcode, box)
    throughput = get_throughput(mnemonic, opcode)

    # Generate copyright
    copyright = get_copyright_header(
        filename, f"{mnemonic} Instruction Grain"
    )

    # ------------------------------------------------------------------------
    # Execute method with CPU trace hooks
    # ------------------------------------------------------------------------
    execute_body = f"""
        // --------------------------------------------------------------------
        // Pipeline stage trace
        // --------------------------------------------------------------------
    #ifdef ENABLE_CPU_TRACE
        CpuTrace::pipeline(slot.cycle, "EXECUTE", "unit={box}");
    #endif

        // Delegate to box execution
        slot.{box_member}->{execute_method}(slot);

        // --------------------------------------------------------------------
        // Instruction trace
        // --------------------------------------------------------------------
    #ifdef ENABLE_CPU_TRACE
        QString operands = slot.getOperandsString();
        QString result = slot.getResultString();

        CpuTrace::instruction(
            slot.cycle,
            slot.pc,
            slot.di.rawBits(),
            "{mnemonic}",
            operands,
            result
        );
    #endif
"""

    template = f"""{copyright}
//
//  Instruction: {mnemonic} - {description}
//  Opcode: {opcode}, Function: {function}
//  Execution Box: {box}
//  Format: {get_instruction_format(opcode, mnemonic, box)}
//  Latency: {latency} cycles, Throughput: {throughput}/cycle
//
//  Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}
//
// ============================================================================

#ifndef {header_guard}
#define {header_guard}

#include "coreLib/Axp_Attributes_core.h"
#include "{box_header}"
#include "grainFactoryLib/InstructionGrain.h"
#include "grainFactoryLib/InstructionGrainRegistry.h"
#include "grainFactoryLib/executionBoxDecoder_inl.h"
#include "machineLib/PipeLineSlot.h"
#include "machineLib/CpuTrace.h"  // <-- Added CpuTrace include

// ============================================================================
// {mnemonic} Instruction Grain
// ============================================================================

class {class_name} : public InstructionGrain
{{
public:
    {class_name}()
        : InstructionGrain(
            0,           // rawBits (updated per-fetch)
            {flags},     // flags
            {latency},   // latency (cycles)
            {throughput} // throughput (instructions/cycle)
          )
        , m_mnemonic("{mnemonic}")
        , m_opcode({opcode})
        , m_functionCode({function})
        , m_platform(GrainPlatform::Alpha)
    {{
    }}

    // ========================================================================
    // Virtual Method Implementations
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE
    void execute(PipelineSlot& slot) const noexcept override
    {{
{execute_body}
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    ExecutionBox executionBox() const noexcept
    {{
        return ExecutionBox::{box};
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    GrainType grainType() const noexcept override
    {{
        return GrainType::{grain_type};
    }}

    // ========================================================================
    // Pure Virtual Accessor Implementations
    // ========================================================================

    AXP_HOT AXP_ALWAYS_INLINE
    QString mnemonic() const noexcept override
    {{
        return m_mnemonic;
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    quint8 opcode() const noexcept override
    {{
        return m_opcode;
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    quint16 functionCode() const noexcept override
    {{
        return m_functionCode;
    }}

    AXP_HOT AXP_ALWAYS_INLINE
    GrainPlatform platform() const noexcept override
    {{
        return m_platform;
    }}

private:
    QString m_mnemonic;
    quint8 m_opcode;
    quint16 m_functionCode;
    GrainPlatform m_platform;
}};

// ============================================================================
// Auto-registration
// ============================================================================

namespace {{
    static GrainAutoRegistrar<{class_name}> s_{sanitize_mnemonic(mnemonic).lower()}_registrar(
        {opcode}, {function}
    );
}}

#endif // {header_guard}
"""
    return template
