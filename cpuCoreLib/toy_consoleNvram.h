#ifndef TOY_CONSOLENVRAM_H
#define TOY_CONSOLENVRAM_H
#include <QtGlobal>
#include <QMap>
#include <QString>
#include <QSettings>
#include "../coreLib/EnvironVariables.h"

struct ConsoleNvram
{
    QMap<QString, QString> env;

    QString get(const QString& key) const {
    }
    void set(const QString& key, const QString& value) noexcept {

    }
    void initialize() {
        set("BOOT_DEV", "");
        set("BOOTDEF_DEV", "");
        set("BOOTED_DEV", "");
        set("BOOT_FILE", "");
        set("BOOTED_FILE", "");
        set("BOOTED_OSFLAGS", "");
        set("BOOT_RESET", "");
        set("DUMP_DEV", "");
        set("ENABLE_AUDIT", "");
        set("LICENSE", "");
        set("CHAR_SET", "");
        set("LANGUAGE", "");
        set("TTY_DEV", "");

    }
    AUTO_ACTION autoAction{ AUTO_ACTION::HALT };
    ENABLE_AUDIT enableAudit{ ENABLE_AUDIT::OFF };
    BOOT_RESET bootReset{ BOOT_RESET::OFF };

    void loadFromNonVolatileStorage(const QSettings &settings) noexcept {

    }

};
#endif // TOY_CONSOLENVRAM_H
