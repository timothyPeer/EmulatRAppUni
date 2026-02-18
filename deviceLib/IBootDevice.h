#ifndef IBOOTDEVICE_H
#define IBOOTDEVICE_H
#include <QString>
#include <QtGlobal>





class IBootDevice {
public:
    virtual bool hasBootstrapCapability() const = 0;
    virtual BootstrapImage readBootstrap() = 0;
    virtual QString getBootDevicePath() const = 0;
};

// Then boot becomes elegant:
void SRMConsole::cmdBoot(const QString& deviceName) {
    auto* device = deviceManager.getBootDevice(deviceName);
    if (!device || !device->hasBootstrapCapability()) {
        printError("Device not bootable");
        return;
    }

    auto bootstrap = device->readBootstrap();  //  Polymorphic
    transferControl(bootstrap);
}

#endif // IBOOTDEVICE_H
