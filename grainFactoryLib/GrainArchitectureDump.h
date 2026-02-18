#ifndef GRAIN_ARCHITECTURE_DUMP_H
#define GRAIN_ARCHITECTURE_DUMP_H

// ============================================================================
// GrainArchitectureDump.h
//
// Purpose: Comprehensive grain registry analysis and reporting
//
// Features:
//   - Enumerate all registered grains
//   - Detect duplicate opcode/function registrations
//   - Identify gaps in instruction coverage
//   - Generate human-readable reports
//   - Export to JSON for tooling
//
// Usage:
//   auto& dump = GrainArchitectureDump::instance();
//   dump.analyze();
//   dump.writeReport("logs/architecture_dump.txt");
//   dump.writeJSON("logs/grain_registry.json");
// ============================================================================

#include <QtGlobal>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QSet>
#include <QVector>
#include "InstructionGrainRegistry.h"
#include "InstructionGrain.h"

// ============================================================================
// GrainInfo - Detailed information about a registered grain
// ============================================================================
struct GrainInfo
{
    quint8 opcode;
    quint16 functionCode;
    QString mnemonic;
    QString grainTypeName;
    GrainType grainType;
    const InstructionGrain* grain;

    QString key() const {
        return QString("%1:%2").arg(opcode, 2, 16, QChar('0'))
            .arg(functionCode, 4, 16, QChar('0'));
    }
};

// ============================================================================
// DuplicateReport - Information about duplicate registrations
// ============================================================================
struct DuplicateReport
{
    quint8 opcode;
    quint16 functionCode;
    QStringList mnemonics;
    int count;
};

// ============================================================================
// GapReport - Missing opcode/function combinations
// ============================================================================
struct GapReport
{
    quint8 opcode;
    QString opcodeName;
    QVector<quint16> missingFunctions;
    int expectedCount;
    int actualCount;
};

// ============================================================================
// GrainArchitectureDump - Main analysis and reporting class
// ============================================================================
class GrainArchitectureDump
{
public:
    // Singleton accessor
    static GrainArchitectureDump& instance() noexcept
    {
        static GrainArchitectureDump s_instance;
        return s_instance;
    }

    // ========================================================================
    // Analysis API
    // ========================================================================

    /**
     * @brief Analyze grain registry and collect statistics
     * @return true if analysis successful
     */
    bool analyze() noexcept;

    /**
     * @brief Write comprehensive text report
     */
    bool writeReport(const QString& filepath) noexcept;

    /**
     * @brief Write JSON export for tooling
     */
    bool writeJSON(const QString& filepath) noexcept;

    /**
     * @brief Get summary statistics
     */
    QString getSummary() const noexcept;

    /**
     * @brief Check if any duplicates detected
     */
    bool hasDuplicates() const noexcept { return !m_duplicates.isEmpty(); }

    /**
     * @brief Check if any gaps detected
     */
    bool hasGaps() const noexcept { return !m_gaps.isEmpty(); }

    /**
     * @brief Get total grain count
     */
    int totalGrains() const noexcept { return m_grains.size(); }

    // ============================================================================
    // Add to class declaration (public section):
    // ============================================================================

    /**
     * @brief Write tab-delimited file for spreadsheet import
     * Format: Family\tOpcode\tFunction\tMnemonic\tDescription
     */

    // ============================================================================
    // Add to implementation section:
    // ============================================================================

    inline bool writeTSV(const QString& filepath) noexcept
    {
        QFile file(filepath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            ERROR_LOG(QString("Failed to write TSV: %1").arg(filepath));
            return false;
        }

        QTextStream out(&file);

        // Header
        out << "Family\tOpcode\tFunction\tMnemonic\tDescription\n";

        // Sort grains by opcode, then function code
        QVector<GrainInfo> sortedGrains = m_grains;
        std::sort(sortedGrains.begin(), sortedGrains.end(),
            [](const GrainInfo& a, const GrainInfo& b) {
                if (a.opcode != b.opcode)
                    return a.opcode < b.opcode;
                return a.functionCode < b.functionCode;
            });

        // Data rows
        for (const auto& grain : sortedGrains)
        {
            QString family = getInstructionFamily(grain.opcode);
            QString opcodeStr = QString("0x%1").arg(grain.opcode, 2, 16, QChar('0'));
            QString functionStr = QString("0x%1").arg(grain.functionCode, 4, 16, QChar('0'));
            QString mnemonic = grain.mnemonic;
            QString description = getInstructionDescription(grain.opcode, grain.functionCode, mnemonic);

            out << family << "\t"
                << opcodeStr << "\t"
                << functionStr << "\t"
                << mnemonic << "\t"
                << description << "\n";
        }

        INFO_LOG(QString("TSV export written: %1 (%2 instructions)")
            .arg(filepath)
            .arg(sortedGrains.size()));

        return true;
    }

    // ============================================================================
    // Helper: Get instruction family name
    // ============================================================================

    inline QString getInstructionFamily(quint8 opcode) const noexcept
    {
        // Opcode ranges and families
        if (opcode >= 0x00 && opcode <= 0x07) return "PALcode";
        if (opcode == 0x08) return "Memory";      // LDA
        if (opcode == 0x09) return "Memory";      // LDAH
        if (opcode >= 0x0A && opcode <= 0x0F) return "Memory";
        if (opcode >= 0x10 && opcode <= 0x13) return "IntegerOperate";
        if (opcode >= 0x14 && opcode <= 0x17) return "FloatingPoint";
        if (opcode == 0x18) return "Miscellaneous";
        if (opcode == 0x1A) return "ControlFlow";  // JMP/JSR/RET
        if (opcode == 0x1C) return "Miscellaneous"; // Byte manipulation
        if (opcode >= 0x20 && opcode <= 0x2F) return "Memory";
        if (opcode >= 0x30 && opcode <= 0x3F) return "ControlFlow";

        return "Unknown";
    }

    // ============================================================================
    // Helper: Get detailed instruction description
    // ============================================================================

    inline QString getInstructionDescription(
        quint8 opcode, quint16 functionCode, const QString& mnemonic) const noexcept
    {
        // Build description based on opcode and mnemonic
        static const QMap<QString, QString> descriptions = {
            // Memory operations
            {"LDA", "Load address"},
            {"LDAH", "Load address high (shifted)"},
            {"LDL", "Load sign-extended longword"},
            {"LDQ", "Load quadword"},
            {"LDL_L", "Load longword locked"},
            {"LDQ_L", "Load quadword locked"},
            {"STL", "Store longword"},
            {"STQ", "Store quadword"},
            {"STL_C", "Store longword conditional"},
            {"STQ_C", "Store quadword conditional"},

            // Integer arithmetic
            {"ADDL", "Add longword"},
            {"ADDQ", "Add quadword"},
            {"SUBL", "Subtract longword"},
            {"SUBQ", "Subtract quadword"},
            {"S4ADDL", "Scaled add longword by 4"},
            {"S4ADDQ", "Scaled add quadword by 4"},
            {"S8ADDL", "Scaled add longword by 8"},
            {"S8ADDQ", "Scaled add quadword by 8"},
            {"CMPEQ", "Compare signed quadword equal"},
            {"CMPLT", "Compare signed quadword less than"},
            {"CMPLE", "Compare signed quadword less or equal"},
            {"CMPULT", "Compare unsigned quadword less than"},
            {"CMPULE", "Compare unsigned quadword less or equal"},
            {"CMPBGE", "Compare byte greater or equal"},

            // Integer logical
            {"AND", "Logical product"},
            {"BIC", "Logical product with complement (bit clear)"},
            {"BIS", "Logical sum (OR)"},
            {"ORNOT", "Logical sum with complement"},
            {"XOR", "Logical difference"},
            {"EQV", "Logical equivalence (XNOR)"},

            // Integer conditional moves
            {"CMOVEQ", "Conditional move if equal to zero"},
            {"CMOVNE", "Conditional move if not equal to zero"},
            {"CMOVLT", "Conditional move if less than zero"},
            {"CMOVGE", "Conditional move if greater or equal to zero"},
            {"CMOVLE", "Conditional move if less or equal to zero"},
            {"CMOVGT", "Conditional move if greater than zero"},
            {"CMOVLBS", "Conditional move if low bit set"},
            {"CMOVLBC", "Conditional move if low bit clear"},

            // Shifts and masks
            {"SLL", "Shift left logical"},
            {"SRL", "Shift right logical"},
            {"SRA", "Shift right arithmetic"},
            {"EXTBL", "Extract byte low"},
            {"EXTWL", "Extract word low"},
            {"EXTLL", "Extract longword low"},
            {"EXTQL", "Extract quadword low"},
            {"INSBL", "Insert byte low"},
            {"INSWL", "Insert word low"},
            {"INSLL", "Insert longword low"},
            {"INSQL", "Insert quadword low"},
            {"MSKBL", "Mask byte low"},
            {"MSKWL", "Mask word low"},
            {"MSKLL", "Mask longword low"},
            {"MSKQL", "Mask quadword low"},
            {"ZAP", "Zero bytes"},
            {"ZAPNOT", "Zero bytes not"},

            // Integer multiply
            {"MULL", "Multiply longword"},
            {"MULQ", "Multiply quadword"},
            {"UMULH", "Unsigned multiply quadword high"},

            // Floating-point arithmetic
            {"ADDS", "Add S_floating"},
            {"ADDT", "Add T_floating"},
            {"SUBS", "Subtract S_floating"},
            {"SUBT", "Subtract T_floating"},
            {"MULS", "Multiply S_floating"},
            {"MULT", "Multiply T_floating"},
            {"DIVS", "Divide S_floating"},
            {"DIVT", "Divide T_floating"},
            {"SQRTS", "Square root S_floating"},
            {"SQRTT", "Square root T_floating"},

            // Floating-point compare
            {"CMPTEQ", "Compare T_floating equal"},
            {"CMPTLT", "Compare T_floating less than"},
            {"CMPTLE", "Compare T_floating less or equal"},
            {"CMPTUN", "Compare T_floating unordered"},

            // Floating-point conversion
            {"CVTTS", "Convert T_floating to S_floating"},
            {"CVTST", "Convert S_floating to T_floating"},
            {"CVTTQ", "Convert T_floating to quadword"},
            {"CVTQT", "Convert quadword to T_floating"},
            {"CVTQS", "Convert quadword to S_floating"},

            // Floating-point conditional moves
            {"FCMOVEQ", "FP conditional move if equal to zero"},
            {"FCMOVNE", "FP conditional move if not equal to zero"},
            {"FCMOVLT", "FP conditional move if less than zero"},
            {"FCMOVGE", "FP conditional move if greater or equal to zero"},
            {"FCMOVLE", "FP conditional move if less or equal to zero"},
            {"FCMOVGT", "FP conditional move if greater than zero"},

            // Branches
            {"BR", "Unconditional branch"},
            {"BSR", "Branch to subroutine"},
            {"BEQ", "Branch if equal to zero"},
            {"BNE", "Branch if not equal to zero"},
            {"BLT", "Branch if less than zero"},
            {"BGE", "Branch if greater or equal to zero"},
            {"BLE", "Branch if less or equal to zero"},
            {"BGT", "Branch if greater than zero"},
            {"BLBC", "Branch if low bit clear"},
            {"BLBS", "Branch if low bit set"},
            {"FBEQ", "FP branch if equal to zero"},
            {"FBNE", "FP branch if not equal to zero"},
            {"FBLT", "FP branch if less than zero"},
            {"FBGE", "FP branch if greater or equal to zero"},
            {"FBLE", "FP branch if less or equal to zero"},
            {"FBGT", "FP branch if greater than zero"},

            // Jumps
            {"JMP", "Jump"},
            {"JSR", "Jump to subroutine"},
            {"RET", "Return from subroutine"},
            {"JSR_COROUTINE", "Jump to subroutine return"},

            // Byte manipulation
            {"CTPOP", "Count population (number of bits set)"},
            {"CTLZ", "Count leading zeros"},
            {"CTTZ", "Count trailing zeros"},
            {"PKLB", "Pack longwords to bytes"},
            {"PKWB", "Pack words to bytes"},
            {"UNPKBL", "Unpack bytes to longwords"},
            {"UNPKBW", "Unpack bytes to words"},

            // SIMD
            {"MINUB8", "Minimum unsigned byte (8-way)"},
            {"MINSB8", "Minimum signed byte (8-way)"},
            {"MINUW4", "Minimum unsigned word (4-way)"},
            {"MINSW4", "Minimum signed word (4-way)"},
            {"MAXUB8", "Maximum unsigned byte (8-way)"},
            {"MAXSB8", "Maximum signed byte (8-way)"},
            {"MAXUW4", "Maximum unsigned word (4-way)"},
            {"MAXSW4", "Maximum signed word (4-way)"},

            // FP utility
            {"CPYS", "Copy sign"},
            {"CPYSN", "Copy sign negate"},
            {"CPYSE", "Copy sign and exponent"},
            {"MT_FPCR", "Move to floating-point control register"},
            {"MF_FPCR", "Move from floating-point control register"},
            {"FTOIT", "Floating to integer move, T_floating"},
            {"FTOIS", "Floating to integer move, S_floating"},
            {"ITOFT", "Integer to floating move, T_floating"},
            {"ITOFS", "Integer to floating move, S_floating"},

            // Miscellaneous
            {"TRAPB", "Trap barrier"},
            {"EXCB", "Exception barrier"},
            {"MB", "Memory barrier"},
            {"WMB", "Write memory barrier"},
            {"FETCH", "Prefetch data"},
            {"FETCH_M", "Prefetch data, modify intent"},
            {"RC", "Read and clear"},
            {"RS", "Read and set"},
        };

        // Check if we have a description for this mnemonic
        QString baseDesc = descriptions.value(mnemonic, "");

        // If no description found, try to infer from mnemonic
        if (baseDesc.isEmpty())
        {
            // Check for variant suffixes
            if (mnemonic.endsWith("/C")) return descriptions.value(mnemonic.left(mnemonic.length() - 2), "") + " (chopped rounding)";
            if (mnemonic.endsWith("/M")) return descriptions.value(mnemonic.left(mnemonic.length() - 2), "") + " (round to minus infinity)";
            if (mnemonic.endsWith("/D")) return descriptions.value(mnemonic.left(mnemonic.length() - 2), "") + " (dynamic rounding)";
            if (mnemonic.endsWith("/U")) return descriptions.value(mnemonic.left(mnemonic.length() - 2), "") + " (underflow enable)";
            if (mnemonic.endsWith("/V")) return descriptions.value(mnemonic.left(mnemonic.length() - 2), "") + " (overflow enable /V)";
            if (mnemonic.endsWith("/S")) return descriptions.value(mnemonic.left(mnemonic.length() - 2), "") + " (software completion)";
            if (mnemonic.endsWith("/SU")) return descriptions.value(mnemonic.left(mnemonic.length() - 3), "") + " (software, suppress underflow)";
            if (mnemonic.endsWith("/SUI")) return descriptions.value(mnemonic.left(mnemonic.length() - 4), "") + " (software, suppress underflow+inexact)";

            // Generic description
            return QString("Opcode 0x%1, Function 0x%2")
                .arg(opcode, 2, 16, QChar('0'))
                .arg(functionCode, 4, 16, QChar('0'));
        }

        return baseDesc;
    }

    quint32 duplicateCount() const noexcept { return this->m_duplicates.count(); }
    quint32 gapCount() const noexcept { return this->m_gaps.count();  }
private:
    GrainArchitectureDump() = default;
    ~GrainArchitectureDump() = default;
    Q_DISABLE_COPY(GrainArchitectureDump)

        // ========================================================================
        // Analysis Methods
        // ========================================================================

        void collectGrains() noexcept;
    void detectDuplicates() noexcept;
    void detectGaps() noexcept;
    void categorizeByType() noexcept;

    // ========================================================================
    // Report Generation
    // ========================================================================

    QString generateTextReport() const noexcept;
    QString generateOpcodeTable() const noexcept;
    QString generateDuplicateReport() const noexcept;
    QString generateGapReport() const noexcept;
    QString generateCoverageReport() const noexcept;
    QJsonDocument generateJSON() const noexcept;

    // ========================================================================
    // Helper Methods
    // ========================================================================

    QString grainTypeToString(GrainType type) const noexcept;
    QString getOpcodeDescription(quint8 opcode) const noexcept;
    int getExpectedFunctionCount(quint8 opcode) const noexcept;

    // ========================================================================
    // Data
    // ========================================================================

    QVector<GrainInfo> m_grains;
    QMap<QString, QVector<GrainInfo>> m_grainsByOpcode;
    QMap<GrainType, QVector<GrainInfo>> m_grainsByType;
    QVector<DuplicateReport> m_duplicates;
    QVector<GapReport> m_gaps;
    QSet<InstructionGrain*> seen; // stores makeGrainKey results

    // Statistics
    int m_totalGrains{ 0 };
    int m_uniqueOpcodes{ 0 };
    QMap<GrainType, int> m_typeCounts;
};

// ============================================================================
// Implementation
// ============================================================================

inline bool GrainArchitectureDump::analyze() noexcept
{
    INFO_LOG("Starting grain architecture analysis...");

    m_grains.clear();
    m_grainsByOpcode.clear();
    m_grainsByType.clear();
    m_duplicates.clear();
    m_gaps.clear();

    collectGrains();
    detectDuplicates();
    detectGaps();
    categorizeByType();

    INFO_LOG(QString("Analysis complete: %1 grains, %2 duplicates, %3 gaps")
        .arg(m_totalGrains)
        .arg(m_duplicates.size())
        .arg(m_gaps.size()));

    return true;
}



inline void GrainArchitectureDump::collectGrains() noexcept
{
    auto& registry = InstructionGrainRegistry::instance();

    seen.clear();
    // Iterate through all possible opcodes (0x00 - 0x3F)
    for (quint16 opcode = 0; opcode <= 0x3F; ++opcode)
    {
        // For each opcode, check common function codes
        // Most opcodes use 0, some use 0-127 (7-bit), some use 0-2047 (11-bit)
        for (quint16 func = 0; func < 2048; ++func)
        {
            auto* grain = registry.lookup(static_cast<quint8>(opcode), func,
                GrainPlatform::Alpha);
            if (!grain || seen.contains(grain))
                break;
            seen.insert(grain);

            GrainInfo info;
            info.opcode = static_cast<quint8>(opcode);
            info.functionCode = func;
            info.mnemonic = grain->mnemonic();
            info.grainType = grain->grainType();
            info.grainTypeName = grainTypeToString(info.grainType);
            info.grain = grain;
            m_grains.append(info);
            m_grainsByOpcode[QString("%1").arg(opcode, 2, 16, QChar('0'))].append(info);
        }
    }

    m_totalGrains = m_grains.size();
    m_uniqueOpcodes = m_grainsByOpcode.size();
}

inline void GrainArchitectureDump::detectDuplicates() noexcept
{
    QMap<QString, QVector<GrainInfo>> duplicateMap;

    for (const auto& grain : m_grains)
    {
        QString key = grain.key();
        duplicateMap[key].append(grain);
    }

    for (auto it = duplicateMap.begin(); it != duplicateMap.end(); ++it)
    {
        if (it.value().size() > 1)
        {
            DuplicateReport dup;
            dup.opcode = it.value().first().opcode;
            dup.functionCode = it.value().first().functionCode;
            dup.count = it.value().size();

            for (const auto& grain : it.value())
            {
                dup.mnemonics.append(grain.mnemonic);
            }

            m_duplicates.append(dup);

            WARN_LOG(QString("Duplicate grain: opcode 0x%1, function 0x%2, count %3")
                .arg(dup.opcode, 2, 16, QChar('0'))
                .arg(dup.functionCode, 4, 16, QChar('0'))
                .arg(dup.count));
        }
    }
}

inline void GrainArchitectureDump::detectGaps() noexcept
{
    // Define expected instruction counts for known opcode families
    struct OpcodeFamily {
        quint8 opcode;
        QString name;
        int expectedCount;
    };

    QVector<OpcodeFamily> families = {
        {0x10, "IntOperate (Add/Sub/Cmp)", 19},
        {0x11, "IntOperate (Logical/CMov)", 14},
        {0x12, "IntOperate (Shift/Mask)", 26},
        {0x13, "IntMultiply", 5},
        {0x14, "FltIEEE (ITOF/ITOFS)", 2},
        {0x15, "FltVAX", 8},
        {0x16, "FltIEEE (Arithmetic)", 32},
        {0x17, "FltIEEE (Convert)", 8},
        {0x1C, "ByteManip/FP_Misc", 16},
        // Add more as needed
    };

    for (const auto& family : families)
    {
        QString opcodeKey = QString("%1").arg(family.opcode, 2, 16, QChar('0'));
        int actual = m_grainsByOpcode.value(opcodeKey).size();

        if (actual < family.expectedCount)
        {
            GapReport gap;
            gap.opcode = family.opcode;
            gap.opcodeName = family.name;
            gap.expectedCount = family.expectedCount;
            gap.actualCount = actual;

            // Could detect specific missing functions here

            m_gaps.append(gap);

            WARN_LOG(QString("Gap detected: opcode 0x%1 (%2), expected %3, found %4")
                .arg(gap.opcode, 2, 16, QChar('0'))
                .arg(gap.opcodeName)
                .arg(gap.expectedCount)
                .arg(gap.actualCount));
        }
    }
}

inline void GrainArchitectureDump::categorizeByType() noexcept
{
    m_grainsByType.clear();
    m_typeCounts.clear();

    for (const auto& grain : m_grains)
    {
        m_grainsByType[grain.grainType].append(grain);
        m_typeCounts[grain.grainType]++;
    }
}

inline QString GrainArchitectureDump::generateTextReport() const noexcept
{
    QString report;
    QTextStream out(&report);

    out << "================================================================================\n";
    out << "ALPHA AXP GRAIN ARCHITECTURE DUMP\n";
    out << "Generated: " << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    out << "================================================================================\n\n";

    // Summary
    out << "SUMMARY:\n";
    out << QString("  Total Grains: %1\n").arg(m_totalGrains);
    out << QString("  Unique Opcodes: %1\n").arg(m_uniqueOpcodes);
    out << QString("  Duplicates: %1\n").arg(m_duplicates.size());
    out << QString("  Gaps: %1\n\n").arg(m_gaps.size());

    // By Type
    out << "GRAINS BY TYPE:\n";
    for (auto it = m_typeCounts.begin(); it != m_typeCounts.end(); ++it)
    {
        out << QString("  %1:\t%2\n")
            .arg(grainTypeToString(it.key()), -20)
            .arg(it.value());
    }
    out << "\n";

    // Opcode Table
    out << generateOpcodeTable();
    out << "\n";

    // Duplicates
    if (!m_duplicates.isEmpty())
    {
        out << generateDuplicateReport();
        out << "\n";
    }

    // Gaps
    if (!m_gaps.isEmpty())
    {
        out << generateGapReport();
        out << "\n";
    }

    // Coverage
    out << generateCoverageReport();

    return report;
}

inline QString GrainArchitectureDump::generateOpcodeTable() const noexcept
{
    QString table;
    QTextStream out(&table);

    out << "INSTRUCTION COVERAGE BY OPCODE:\n";
    out << QString("%1 %2 %3 %4\n")
        .arg("Opcode", -6)        // %1: left-align, 6 chars
        .arg("Description", -30)  // %2: left-align, 30 chars
        .arg("Count", 5)          // %3: right-align, 5 chars
        .arg("Mnemonics");        // %4: no width

    out << "--------------------------------------------------------------------------------\n";


    for (auto it = m_grainsByOpcode.begin(); it != m_grainsByOpcode.end(); ++it)
    {
        quint8 opcode = it.value().first().opcode;
        QString desc = getOpcodeDescription(opcode);
        int count = it.value().size();

        QStringList mnemonics;
        for (const auto& grain : it.value())
        {
            if (!mnemonics.contains(grain.mnemonic))
                mnemonics.append(grain.mnemonic);
        }
        out << QString("0x%1   %2  %3  %4\n")
            .arg(opcode, 2, 16, QChar('0'))    // %1: hex, 2 chars, pad with '0'
            .arg(desc, -30)                     // %2: left-align, 30 chars (negative = left)
            .arg(count, 5)                      // %3: right-align, 5 chars
            .arg(mnemonics.join(", "));        // %4: string
    }

    return table;
}

inline QString GrainArchitectureDump::generateDuplicateReport() const noexcept
{
    QString report;
    QTextStream out(&report);

    out << "??  DUPLICATE REGISTRATIONS:\n";
    out << "================================================================================\n";

    for (const auto& dup : m_duplicates)
    {
        out << QString("Opcode 0x%1, Function 0x%2: %3 registrations\n")
            .arg(dup.opcode, 2, 16, QChar('0'))
            .arg(dup.functionCode, 4, 16, QChar('0'))
            .arg(dup.count);
        out << QString("  Mnemonics: %1\n").arg(dup.mnemonics.join(", "));
    }

    return report;
}

inline QString GrainArchitectureDump::generateGapReport() const noexcept
{
    QString report;
    QTextStream out(&report);

    out << "  COVERAGE GAPS:\n";
    out << "================================================================================\n";

    for (const auto& gap : m_gaps)
    {
        out << QString("Opcode 0x%1 (%2): Expected %3, Found %4 (-%5)\n")
            .arg(gap.opcode, 2, 16, QChar('0'))
            .arg(gap.opcodeName)
            .arg(gap.expectedCount)
            .arg(gap.actualCount)
            .arg(gap.expectedCount - gap.actualCount);
    }

    return report;
}

inline QString GrainArchitectureDump::generateCoverageReport() const noexcept
{
    QString report;
    QTextStream out(&report);

    out << "COVERAGE ANALYSIS:\n";
    out << "================================================================================\n";

    // Calculate coverage percentage
    int totalExpected = 256; // Rough estimate of Alpha instruction count
    double coverage = (m_totalGrains * 100.0) / totalExpected;

    out << QString("Estimated Coverage: %1% (%2/%3 instructions)\n")
        .arg(coverage, 0, 'f', 1)
        .arg(m_totalGrains)
        .arg(totalExpected);

    return report;
}

inline QString GrainArchitectureDump::grainTypeToString(GrainType type) const noexcept
{
    switch (type)
    {
    case GrainType::IntegerOperate: return "IntegerOperate";
    case GrainType::FloatOperate: return "FloatingPoint";
    case GrainType::IntegerMemory: return "Memory";
    case GrainType::ControlFlow: return "ControlFlow";
    case GrainType::Pal: return "PALcode";
    case GrainType::Miscellaneous: return "Miscellaneous";
    default: return "Unknown";
    }
}

inline QString GrainArchitectureDump::getOpcodeDescription(quint8 opcode) const noexcept
{
    static const QMap<quint8, QString> descriptions = {
        {0x08, "LDA - Load Address"},
        {0x09, "LDAH - Load Address High"},
        {0x10, "IntOperate - Add/Sub/Cmp"},
        {0x11, "IntOperate - Logical/CMov"},
        {0x12, "IntOperate - Shift/Mask"},
        {0x13, "IntMultiply"},
        {0x14, "FltIEEE - ITOF"},
        {0x15, "FltVAX"},
        {0x16, "FltIEEE - Arithmetic"},
        {0x17, "FltIEEE - Convert"},
        {0x1A, "Jump/JSR/RET"},
        {0x1C, "ByteManip/FP_Misc"},
        {0x28, "LDL - Load Longword"},
        {0x29, "LDQ - Load Quadword"},
        {0x2A, "LDL_L - Load Locked Long"},
        {0x2B, "LDQ_L - Load Locked Quad"},
        {0x2C, "STL - Store Longword"},
        {0x2D, "STQ - Store Quadword"},
        {0x2E, "STL_C - Store Conditional Long"},
        {0x2F, "STQ_C - Store Conditional Quad"},
        {0x30, "BR - Branch"},
        {0x31, "FBEQ - FP Branch if Equal"},
        {0x34, "BSR - Branch Subroutine"},
        {0x38, "BLBC - Branch if Low Bit Clear"},
        {0x39, "BEQ - Branch if Equal"},
        {0x3A, "BLT - Branch if Less Than"},
        {0x3D, "BNE - Branch if Not Equal"},
    };

    return descriptions.value(opcode, "Unknown");
}

inline bool GrainArchitectureDump::writeReport(const QString& filepath) noexcept
{
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        ERROR_LOG(QString("Failed to write report: %1").arg(filepath));
        return false;
    }

    QTextStream out(&file);
    out << generateTextReport();

    INFO_LOG(QString("Architecture dump written: %1").arg(filepath));
    return true;
}

inline bool GrainArchitectureDump::writeJSON(const QString& filepath) noexcept
{
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly))
    {
        ERROR_LOG(QString("Failed to write JSON: %1").arg(filepath));
        return false;
    }

    QJsonDocument doc = generateJSON();
    file.write(doc.toJson(QJsonDocument::Indented));

    INFO_LOG(QString("JSON export written: %1").arg(filepath));
    return true;
}

inline QJsonDocument GrainArchitectureDump::generateJSON() const noexcept
{
    QJsonObject root;
    root["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["totalGrains"] = m_totalGrains;
    root["uniqueOpcodes"] = m_uniqueOpcodes;

    // Grains array
    QJsonArray grainsArray;
    for (const auto& grain : m_grains)
    {
        QJsonObject obj;
        obj["opcode"] = QString("0x%1").arg(grain.opcode, 2, 16, QChar('0'));
        obj["function"] = QString("0x%1").arg(grain.functionCode, 4, 16, QChar('0'));
        obj["mnemonic"] = grain.mnemonic;
        obj["type"] = grain.grainTypeName;
        grainsArray.append(obj);
    }
    root["grains"] = grainsArray;

    // Duplicates array
    QJsonArray dupArray;
    for (const auto& dup : m_duplicates)
    {
        QJsonObject obj;
        obj["opcode"] = QString("0x%1").arg(dup.opcode, 2, 16, QChar('0'));
        obj["function"] = QString("0x%1").arg(dup.functionCode, 4, 16, QChar('0'));
        obj["count"] = dup.count;
        QJsonArray mnems;
        for (const auto& m : dup.mnemonics)
            mnems.append(m);
        obj["mnemonics"] = mnems;
        dupArray.append(obj);
    }
    root["duplicates"] = dupArray;

    return QJsonDocument(root);
}

inline QString GrainArchitectureDump::getSummary() const noexcept
{
    return QString("Grains: %1, Opcodes: %2, Duplicates: %3, Gaps: %4")
        .arg(m_totalGrains)
        .arg(m_uniqueOpcodes)
        .arg(m_duplicates.size())
        .arg(m_gaps.size());
}

#endif // GRAIN_ARCHITECTURE_DUMP_H