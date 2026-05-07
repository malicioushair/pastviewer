from conan import ConanFile


class PastViewerConan(ConanFile):
    name = "pastviewer"
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps"

    def requirements(self):
        self.requires("glog/0.7.1")
        self.requires("gflags/2.2.2")
        self.requires("gtest/1.14.0")

        # sentry-native on Apple desktop and iOS. Android uses Sentry via Gradle.
        if self.settings.get_safe("os") in ("Macos", "iOS"):
            self.requires("sentry-native/0.12.1")
