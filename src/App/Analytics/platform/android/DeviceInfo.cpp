#include "../DeviceInfo.h"

#include <QJniObject>

namespace PastViewer::Platform {
QString DeviceModel()
{
    return QJniObject::getStaticObjectField<jstring>("android/os/Build", "MODEL").toString();
}
}
