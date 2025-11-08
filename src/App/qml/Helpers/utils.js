function setTimeout(callback, delayMs) {
    const timer = Qt.createQmlObject(
        'import QtQuick 2.0; Timer { repeat: false; running: false }',
        Qt.application,
        "setTimeoutTimer"
    )

    timer.interval = delayMs

    timer.triggered.connect(function() {
        try {
            if (callback)
                callback()
        } finally {
            timer.destroy()
        }
    })

    timer.start()
}