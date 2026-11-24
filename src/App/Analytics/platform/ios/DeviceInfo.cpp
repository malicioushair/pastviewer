#include "../DeviceInfo.h"

#include <sys/sysctl.h>

#include <QByteArray>

namespace PastViewer::Platform {
QString DeviceModel()
{
    const auto simulatorModel = qEnvironmentVariable("SIMULATOR_MODEL_IDENTIFIER");
    if (!simulatorModel.isEmpty())
        return simulatorModel;

    size_t size = 0;
    if (sysctlbyname("hw.machine", nullptr, &size, nullptr, 0) != 0 || size == 0)
        return {};
    QByteArray model(static_cast<qsizetype>(size), '\0');
    if (sysctlbyname("hw.machine", model.data(), &size, nullptr, 0) != 0)
        return {};
    return QString::fromUtf8(model.constData());
}
}
