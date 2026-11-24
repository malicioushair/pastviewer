#include "../DeviceInfo.h"

#include <QByteArray>
#include <sys/sysctl.h>

namespace PastViewer::Platform {
QString DeviceModel()
{
    size_t size = 0;
    if (sysctlbyname("hw.model", nullptr, &size, nullptr, 0) != 0 || size == 0)
        return {};
    QByteArray model(static_cast<qsizetype>(size), '\0');
    if (sysctlbyname("hw.model", model.data(), &size, nullptr, 0) != 0)
        return {};
    return QString::fromUtf8(model.constData());
}
}
