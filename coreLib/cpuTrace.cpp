#include "cpuTrace.h"
#include <QDateTime>
#include <QTextStream>
#include <QString>
#include <QFile>
#include <QElapsedTimer>



QFile CpuTrace::decFile;
QTextStream CpuTrace::decOut;

QFile CpuTrace::machineFile;
QTextStream CpuTrace::machineOut;

uint32_t CpuTrace::traceMask = TRACE_ALL;

uint64_t CpuTrace::shadowIntRegs[32] = {0};
uint64_t CpuTrace::shadowFpRegs[32] = {0};

//QElapsedTimer CpuTrace::m_timer;

QMutex CpuTrace::mutex;

// Flush every N lines to reduce I/O overhead
static const int FLUSH_EVERY = 4096;
static int lineCounter = 0;

void CpuTrace::initialize(const QString& decLogFile, const QString& machineLogFile)
{
    QMutexLocker lock(&mutex);

    decFile.setFileName(decLogFile);
    if (!decFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    decOut.setDevice(&decFile);

    machineFile.setFileName(machineLogFile);
    if (!machineFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return;
    machineOut.setDevice(&machineFile);

    // Optional header
    decOut << "CPU TRACE LOG - DEC Style Listing\n";
    decOut << "Generated: " << QDateTime::currentDateTime().toString() << "\n";
    decOut << "---------------------------------------------------------------\n";
    decOut.flush();

    machineOut << "# CPU TRACE LOG - Machine Parsable\n";
    machineOut << "# Generated: " << QDateTime::currentDateTime().toString() << "\n";
    machineOut.flush();
}

void CpuTrace::setTraceMask(uint32_t mask)
{
    QMutexLocker lock(&mutex);
    traceMask = mask;
}

uint32_t CpuTrace::getTraceMask()
{
    QMutexLocker lock(&mutex);
    return traceMask;
}

// Human-readable DEC-style instruction log + machine log
void CpuTrace::instruction(uint64_t cycle,
                           uint64_t pc,
                           uint32_t rawInstr,
                           const char* mnemonic,
                           const QString& operands,
                           const QString& result)
{
    if (!(traceMask & TRACE_INSTR))
        return;

    QMutexLocker lock(&mutex);

    QString decLine = QString("%1 %2 %3 %4 %5 %6")
                          .arg(cycle, 8, 10, QChar('0'))
                          .arg(pc, 16, 16, QChar('0'))
                          .arg(rawInstr, 8, 16, QChar('0'))
                          .arg(mnemonic, -6)
                          .arg(operands, -20)
                          .arg(result);
    writeDecLine(decLine);

    QString machineLine = QString("INS cycle=%1 pc=%2 instr=%3 mnem=%4 ops=\"%5\" result=\"%6\"")
                              .arg(cycle)
                              .arg(pc, 16, 16, QChar('0'))
                              .arg(rawInstr, 8, 16, QChar('0'))
                              .arg(mnemonic)
                              .arg(operands)
                              .arg(result);
    writeMachineLine(machineLine);

}

// Delta register logging
void CpuTrace::logRegisterDeltas(uint64_t cycle,
                                 const uint64_t intRegs[32],
                                 const uint64_t fpRegs[32])
{
    QMutexLocker lock(&mutex);

    QString regLine;
    bool anyChange = false;

    // Integer registers
    for (int i = 0; i < 32; ++i)
    {
        if (shadowIntRegs[i] != intRegs[i])
        {
            regLine += QString(" r%1=%2").arg(i).arg(intRegs[i], 16, 16, QChar('0'));
            shadowIntRegs[i] = intRegs[i];
            anyChange = true;
        }
    }

    // Floating point registers
    for (int i = 0; i < 32; ++i)
    {
        if (shadowFpRegs[i] != fpRegs[i])
        {
            regLine += QString(" f%1=%2").arg(i).arg(fpRegs[i], 16, 16, QChar('0'));
            shadowFpRegs[i] = fpRegs[i];
            anyChange = true;
        }
    }

    if (anyChange)
    {
        QString machineLine = QString("REG cycle=%1%2").arg(cycle).arg(regLine);
        writeMachineLine(machineLine);
    }
}

// Architectural or system event
void CpuTrace::event(uint64_t cycle,
                     const char* eventType,
                     uint64_t pc)
{
    if (!(traceMask & TRACE_EVENT))
        return;

    QMutexLocker lock(&mutex);

    QString line = QString("EVT cycle=%1 type=%2").arg(cycle).arg(eventType);
    if (pc != 0)
        line += QString(" pc=%1").arg(pc, 16, 16, QChar('0'));

    writeMachineLine(line);
    writeDecLine(QString("### EVENT: %1 PC=%2").arg(eventType).arg(pc, 16, 16, QChar('0')));
}

// Pipeline logging
void CpuTrace::pipeline(uint64_t cycle,
                        const char* stage,
                        const QString& info)
{
    if (!(traceMask & TRACE_PIPELINE))
        return;

    QMutexLocker lock(&mutex);

    QString line = QString("PIP cycle=%1 stage=%2 info=\"%3\"")
                       .arg(cycle)
                       .arg(stage)
                       .arg(info);

    writeMachineLine(line);
}

// Flush buffers
void CpuTrace::flush()
{
    QMutexLocker lock(&mutex);
    decOut.flush();
    machineOut.flush();
    lineCounter = 0;
}

// Internal helpers
void CpuTrace::writeDecLine(const QString& line)
{
    decOut << line << "\n";
    if (++lineCounter % FLUSH_EVERY == 0)
        decOut.flush();
}

void CpuTrace::writeMachineLine(const QString& line)
{
    machineOut << line << "\n";
    if (lineCounter % FLUSH_EVERY == 0)
        machineOut.flush();
}


