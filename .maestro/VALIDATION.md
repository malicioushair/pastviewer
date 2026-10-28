# Validation record

2026-10-28, iPhone 17 Pro simulator, iOS 26.3:

- Built the current source with `cmake --build build-ios-release -j 4` and
  installed `build-ios-release/bin/PastViewer.app` on the simulator.
- `regression_ios_back_gesture.yaml` passed through Maestro MCP with
  `APP_ID=com.dv.pastviewer`. It verified short-swipe cancellation, a non-edge
  swipe staying on Settings, a completed edge swipe returning to the map,
  Settings re-entry and button-back, and the root-page guard.
- Inspected the cancellation and completion screenshots: Settings was restored
  at its normal position after cancellation; the map filled the page after
  completion. These screenshots show settled states, not intermediate animation
  frames. This flow does not prove reversal during a continuous touch or flick
  velocity thresholds, and has not been run on physical hardware.
- The same flow failed its completed-swipe assertion against the previously
  installed app before the refreshed build was installed.

2026-10-27, iPhone 17 Pro simulator, iOS 26.3. The installed PastViewer executable
was x86-64 running under translation. This is a record of the installed build,
not a claim about an ARM build or the current source.

- Maestro `check-syntax` accepted all flow and helper YAML files (prior to the
  removal of the draft deletion step; that change only removed commands).
- `regression_language.yaml` passed end-to-end: changed English to French,
  relaunched, verified French persisted, and returned to English.
- Exploratory interaction with `multi-img` succeeded: entered its stop,
  swiped from `1 / 2` to `2 / 2`, opened the image by double-tap, returned to
  the gallery, returned to the map, and exited the tour. The complete saved
  gallery regression has not passed as a single flow.
- `regression_settings_persistence.yaml` crashed during its run with SIGBUS.
  On the initial inspection, “Show only nearest objects” was not selected; its
  simulator preference is now enabled. The interrupted test did not restore it.
- Subsequent onboarding, gallery and creation-cancel flows failed with app
  crashes reported as SIGSEGV. Additional crash reports were recorded. The
  faulting stacks are not sufficient to attribute the crash to a feature or to
  x86 translation.
- After restarting the simulator, the MCP device connection remained
  unreachable. A separate CLI launch attempt did not reach its first app-window
  assertion; a new PastViewer SIGSEGV report appeared. The CLI attempt was
  interrupted to prevent repeated retries.
- Draft lifecycle and onboarding flows have not passed end-to-end. Syntax checks
  do not validate selector behavior, data persistence, or app rendering.

The suite stops at the first failed flow to avoid cascading retries after an app
crash. The draft test intentionally leaves its uniquely named test draft in place
because the current UI does not expose a reliably row-specific delete selector.
