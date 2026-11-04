# Validation record

2026-11-03, suite refresh (Android and iOS):

- Installed the existing signed Android release APK on Pixel_6_Pro / Android 16
  (`emulator-5554`). Rebuilt `build-ios-release` and installed it on iPhone 17
  Pro / iOS 26.3 (`0A92CF42-4D72-4252-AE4D-D7C5B1A764A3`). Runs below used Maestro MCP.
- Passed on both platforms: `smoke_launch`, `smoke_settings`, and
  `regression_settings_persistence`. All three checkbox preferences were toggled,
  checked after relaunch and restored by the shared helper.
- iOS additionally passed `regression_creation_cancel`, `regression_draft_lifecycle`,
  `regression_language`, `regression_onboarding`, and `regression_ios_back_gesture`.
  Language finished in English. A focused rename/save probe also passed using the
  observed editor TextField resource identifier, without a title-field coordinate.
- Android's focused `regression_draft_lifecycle` passed after the final guarded
  keyboard-dismiss helper was added. It selected a cover from the native picker,
  checked point-removal cancel/confirm, stop form controls/cancel, renamed the
  draft, relaunched, and reopened the saved route.
- Android `optional/stop_image_gallery.yaml` passed end-to-end: two fixture
  images selected, `1 / 2` -> `2 / 2` -> `1 / 2` checked, no image-load error
  label, stop form cancelled, local draft retained, normal map restored.
  The native multi-select action is `Select` (`action_menu_select`), not Open.
  A guarded launch helper also closes an orphaned external Android Files picker
  left behind by a failed run before restarting the app. No other page receives
  that helper's Back action.
- Initial title-value assertions failed even though screenshots showed the
  correct input: Qt exposes the title-field accessibility label rather than the
  typed value on both platforms. The tests now verify actual titles in Drafts
  after saving. A single center-tap/erase could leave a suffix; two passes were
  verified with a focused iOS rename/save run.
- Android `hideKeyboard` after returning from a file picker could navigate Back
  out of the form. The shared helper now taps `android:id/input_method_nav_back`
  only when that keyboard-dismiss button is visible. The lifecycle rerun passed.
- Standalone CLI attempts hit `DeviceServerDiedException` / gRPC UNAVAILABLE before
  flow commands ran. These are not assertion failures or passing validations.
  A six-flow Android MCP batch exceeded the tool's 300-second request timeout;
  no individual passes are credited from that batch. Focused MCP runs were used.
- Final checks: all 35 flow/helper YAML files passed Maestro `check-syntax`;
  all 77 helper references resolve; all 12 ordered workspace flow names resolve;
  `git diff --check` passed.
- Workspace configuration now uses the declared flow names for ordering, removes
  unsupported `name`/`tags` workspace keys, and disables automatic retries.
- The default suite preserves existing drafts. New timestamped test drafts remain
  for inspection; no publishing, purchase or upload action was executed.

Current evidence boundaries:

- Catalog smoke/gallery flows require the configured server fixtures and have not
  been rerun during this refresh. Missing fixtures fail explicitly.
- The new opt-in compact-gallery flow passed on Android. iOS needs
  both PNG fixtures available in the native Files picker; iOS media import and
  gallery execution have not been verified during this refresh.
- No physical-device UI run is claimed. New local stop persistence, saved-media
  removal, narration playback, upload and publication are not covered by the new
  selection/cancellation gallery regression.

2026-10-28, Android Pixel_6_Pro emulator:

- `regression_android_system_back.yaml` was accepted by Maestro's syntax check.
- The initial window-shortcut implementation failed on the installed release
  build: system Back left the app instead of returning to the map. The installed
  native library matched the local release build, ruling out a stale APK.
- Replaced that implementation with `ApplicationWindow.onClosing`, which is
  compatible with this app's Qt 6.10 Android close-request behavior. A focused
  offscreen test passed child-control-focus navigation and root close handling.
- The close handler alone also failed on Android 16 with target SDK 36 because
  Android no longer dispatched legacy Back events. Added the documented
  `android:enableOnBackInvokedCallback="false"` compatibility setting for Qt 6.10.
  This disables predictive system previews while retaining system Back gestures.
- Rebuilt the release APK successfully, then the user signed and installed it.
  The full `regression_android_system_back.yaml` passed through Maestro MCP on
  `emulator-5554` with `APP_ID=org.qtproject.PastViewer`. Verified system Back and
  an actual left-edge system gesture returning from Settings to the map, Home
  preserving Settings across resume, and system Back leaving the app at the
  root followed by successful relaunch. No app data was cleared. Physical-device
  behavior and predictive preview animations are not covered.

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


## 2026-11-04 catalog fixture correction

The Studio workspace run ending at 20:27 had nine completed flows and three
failures. All three catalog flows failed while looking for `multi-img`; the
Available tours screen contained only `Belgrade City Center Walk`.

- Updated the two catalog smoke flows to default to `Belgrade City Center Walk`.
- Re-ran both saved smoke flows on iPhone 17 Pro, iOS 26.3, with
  `APP_ID=com.dv.pastviewer` using Maestro MCP: both passed (18 commands total).
- Moved the fixture-dependent gallery flow to
  `optional/regression_tour_gallery.yaml`, preserving its assertions and updating
  relative helper paths. It remains unverified until `multi-img` (or an equivalent
  explicitly configured fixture) is available. It is excluded from default Studio
  discovery and CLI workspace ordering.
- No full workspace rerun or Android validation was performed for this change.
