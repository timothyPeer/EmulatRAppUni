#ifndef ICONSOLESERVICE_H
#define ICONSOLESERVICE_H
#include <QtGlobal>

/*
 *

Use a small, emulator-friendly convention that still maps well to firmware expectations:
    GETC:
    - if char available: R0 = (quint64)(unsigned char)c
    - if none available (non-blocking): R0 = (quint64)-1
    PUTC:
    - R0 = 0 (success)

    PUTS:
    - R0 = number_of_bytes_written (0..len), or -1 on fault

    PalService::executeCSERVE
        -> ConsoleService::cserveGetc / cservePutc / cservePuts
            -> active ConsoleDevice (OPA0 backend)

 */

enum class CserveFunc : quint64
{
    GETC = 0x01,  // read one char
    PUTC = 0x02,  // write one char
    PUTS = 0x04,  // write buffer/bytes from guest memory
    // Optional later:
    GETS = 0x03,
    RESET = 0x05,
    STAT = 0x06,
};

class IConsoleService
{
public:
    virtual ~IConsoleService() = default;

    // Non-blocking read. Return true if a char was produced.
    virtual bool tryGetChar(char& out) noexcept = 0;

    // Write one character.
    virtual void putChar(char c) noexcept = 0;

    // Write a byte buffer (already in host memory).
    virtual void putBytes(const char* bytes, quint32 len) noexcept = 0;
};

#endif // ICONSOLESERVICE_H
