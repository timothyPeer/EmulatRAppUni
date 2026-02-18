#pragma once
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace DecAsm {

    // -----------------------------
    // Options controlling alignment
    // -----------------------------
    struct FormatOptions final {
        int pcHexDigits = 8;   // 8 -> 0x20001000, 16 -> 0x0000000020001000
        int mnemonicWidth = 6;   // column width for mnemonic
        int operandWidth = 24;  // column width for operands (everything after mnemonic)
        bool uppercaseMnemonic = true;
        bool lowercaseHexPc = true; // "0x..." lower-case
    };

    // -----------------------------
    // Core formatter (explicit fields)
    // -----------------------------
    [[nodiscard]] static inline QString formatLine(quint64 pc,
        QString mnemonic,
        const QString& operands = QString(),
        const QString& comment = QString(),
        const FormatOptions& opt = {})
    {
        if (opt.uppercaseMnemonic)
            mnemonic = mnemonic.toUpper();

        QString pcStr = QString("0x%1").arg(pc, opt.pcHexDigits, 16, QLatin1Char('0'));
        pcStr = opt.lowercaseHexPc ? pcStr.toLower() : pcStr.toUpper(); // affects hex digits too

        const QString mcol = mnemonic.leftJustified(opt.mnemonicWidth, QLatin1Char(' '));
        const QString ocol = operands.leftJustified(opt.operandWidth, QLatin1Char(' '));

        QString line = QString("%1:  %2%3").arg(pcStr, mcol, ocol);

        const QString c = comment.trimmed();
        if (!c.isEmpty())
            line += QString(" ; %1").arg(c);

        return line;
    }

    // -----------------------------
    // Convenience overload (already assembled "LDQ   R1, 0(R2)")
    // You still control exactly what is displayed: pass instrText verbatim.
    // -----------------------------
    [[nodiscard]] static inline QString formatLineText(quint64 pc,
        const QString& instrText,
        const QString& comment = QString(),
        const FormatOptions& opt = {})
    {
        QString pcStr = QString("0x%1").arg(pc, opt.pcHexDigits, 16, QLatin1Char('0'));
        pcStr = opt.lowercaseHexPc ? pcStr.toLower() : pcStr.toUpper();

        // Left-pad to keep it visually consistent with the other overload.
        // (We don’t parse instrText; we just print it.)
        QString line = QString("%1:  %2").arg(pcStr, instrText);

        const QString c = comment.trimmed();
        if (!c.isEmpty())
            line += QString(" ; %1").arg(c);

        return line;
    }

    // -----------------------------
    // Small helper: format reg names in your preferred "R<n>" style
    // -----------------------------
    [[nodiscard]] static inline QString R(int n) { return QString("R%1").arg(n); }

    // Helper: address mode "disp(base)" like "8(R6)"
    [[nodiscard]] static inline QString dispBase(qint64 disp, int baseReg)
    {
        return QString("%1(%2)").arg(disp).arg(R(baseReg));
    }

} // namespace DecAsm

/*
 *
 *
 *
 *
 *
 using namespace DecAsm;

FormatOptions opt;
opt.pcHexDigits = 8;
opt.mnemonicWidth = 6;
opt.operandWidth = 26;

// You pass exactly what gets printed:
QString s0 = formatLine(pc0, "LDQ",  QString("%1, %2").arg(R(1), dispBase(0, 2)),
                       "Load that might fault (DTB miss)", opt);

QString s1 = formatLine(pc1, "ADDQ", QString("%1, %2, %3").arg(R(1), R(3), R(4)),
                       "Arithmetic that might trap (overflow)", opt);

QString s2 = formatLine(pc2, "EXCB", "", "Exception barrier - WAIT for faults", opt);


*/