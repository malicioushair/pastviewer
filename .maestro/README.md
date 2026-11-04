# PastViewer UI regression tests

These flows test the installed app. Rebuild and reinstall it after changing
application code; running Maestro does not rebuild it.
Use a disposable Android emulator or iOS simulator in portrait with English UI.
Current validation targets are Pixel 6 Pro (Android 16) and iPhone 17 Pro
(iOS 26.3, 402 × 874 points). Launch helpers grant permissions where supported.
Physical-device file pickers can differ. The installed build must
expose Settings, Tours, Back, Compass, Re-center and History near you accessibility
labels. Image gestures still require documented points; draft renaming uses the observed
editor TextField identifier instead of a screen coordinate. Qt does not expose
these text-field values to Maestro on either platform, so title assertions happen
in the draft list after saving.

## Running

Every flow and helper requires `APP_ID`. Choose the installed app's ID when
running a test; do not rely on an iOS default for Android.

In Maestro Studio, open this folder and create two environments under Settings →
Environments: set `APP_ID` to `org.qtproject.PastViewer` for Android and to
`com.dv.pastviewer` for iOS. Select the appropriate environment before running.

For the CLI, pass the app ID explicitly for a directory or individual flow:

```sh
# Android
maestro --device emulator-5554 test -e APP_ID=org.qtproject.PastViewer .maestro

# iOS
maestro --device IOS_SIMULATOR_UDID test -e APP_ID=com.dv.pastviewer .maestro

# Single flow example
maestro test -e APP_ID=org.qtproject.PastViewer .maestro/smoke_launch.yaml
```

Select the device explicitly when more than one is connected. Run flows serially
on each device. Platform-specific navigation tests guard their entire body, so
the other OS does not launch or interact with the app for those flows.

The workspace orders flows using their declared `name` values, following
[Maestro workspace ordering](https://docs.maestro.dev/maestro-flows/workspace-management/sequential-execution).
Automatic flow retries are disabled so crashes remain visible.

Run the catalog-independent subset with `--include-tags offline`. Catalog flows
are tagged `catalog`; `CATALOG_TOUR` defaults to `Belgrade City Center Walk` for the catalog smoke
tests and can be overridden with `-e CATALOG_TOUR=...`.

Helpers are nested and tagged `helper`; they are not standalone tests.
`config.yaml` includes both smoke and regression flows.
Shared helpers cover map readiness, known-overlay dismissal, opening Settings,
admin tours/drafts, new draft forms, title replacement, native media selection,
and gallery assertions. The checkbox persistence helper is parameterized by
label; only `checked` (Android) versus `selected` (iOS) assertions differ.

Do not use `helpers/launch_fresh.yaml` on a device containing valuable app data:
it clears app state. Normal flows use warm launches and preserve existing drafts.

## Coverage

| Flow | Behavior |
| --- | --- |
| smoke_launch | Map controls and history row |
| smoke_settings | Settings entry, controls and back navigation |
| smoke_available_tours | Catalog loading and back navigation |
| smoke_tour_info | Details and start/unlock action visibility |
| regression_language | English → French, restart persistence, return to English |
| regression_settings_persistence | Three checkbox settings persist across restart; restore original values; reload action |
| regression_ios_back_gesture | iOS short-swipe cancellation, non-edge drag, completed edge swipe, Settings re-entry and root-page guard |
| regression_android_system_back | Android system Back and edge gesture return from Settings; Home preserves the open page; Back backgrounds the app at the root |
| regression_onboarding | Reset, all map steps, complete, restart, skip and restart |
| optional/regression_tour_gallery | Start tour, stop details, no-audio state, two-image swipe both ways, inspect/back, next/return, exit |
| regression_creation_cancel | Admin menu, browse, both creation choices and cancellation, drafts navigation |
| regression_draft_lifecycle | Local creation, point removal cancel/confirm, stop media controls and cancellation, rename/save, restart persistence, route reopening |
| optional/stop_image_gallery | Local two-image selection, compact-gallery swipes/counter and stop cancellation; explicit fixture setup |

The Android system Back flow requires gesture navigation enabled on the device.
It exercises both a system Back command and a physical edge-swipe path; it does
not change the device's navigation settings.

On Android, the draft lifecycle flow adds the saved screenshot in
`fixtures/pastviewer-maestro-cover.png` to the emulator and selects it in the
native file picker. It also captures a separate screenshot immediately before
opening the picker for run diagnostics. On iOS, the flow skips image selection:
importing the fixture into the native Files picker is not yet covered.

The gallery fixture defaults to the catalog tour `multi-img`, with a first stop
named `st1`, exactly two images and no narration. It must be free or already
owned. `GALLERY_TOUR` can override the tour name but the fixture contract still
applies. Missing fixtures fail explicitly rather than skipping assertions.
The gallery flow is opt-in because this fixture is absent from the current catalog.
Run it explicitly after making the fixture available:

```sh
maestro --device IOS_SIMULATOR_UDID test -e APP_ID=com.dv.pastviewer .maestro/optional/regression_tour_gallery.yaml
```

The default workspace still checks catalog listing and tour details using
`Belgrade City Center Walk`; override `CATALOG_TOUR` if the catalog changes.

## Side effects and recovery

Settings tests restore the original checkbox values on successful completion.
Language testing ends in English. Onboarding testing resets onboarding flags and
completes/skips the map introduction; other screens can show their tips again.
Admin mode is session-only; relaunching returns it to its default.
Draft lifecycle tests use a timestamped `Maestro-...` name and leave the test draft for inspection and manual removal. The draft list
does not expose a reliable row-specific delete selector; do not delete other
drafts to reset tests. No publishing or purchase actions are included.

## Remaining coverage prerequisites

- Narration play/pause/resume/seek and multiple-stop next/previous boundaries need
  a stable free/owned tour with known stop titles and a sufficiently long audio file.
- Historical-photo selection, photo zoom, camera overlay/capture and share-sheet
  cancellation need loaded historical photos at a known location; camera capture
  additionally needs a device that supplies a camera feed.
- iOS media import and media removal need known files in the native Files picker.
  Selecting audio in Add tour stop calls UploadAssets immediately, so it is not
  a local-only test. The separate Upload Audio retry button currently references
  the removed UploadAsset method; tests do not work around or exercise that
  production upload path against the live backend.
- Timeline and notification-distance sliders, map centering/rotation and route
  geometry need observable values/accessibility support or visual assertions;
  merely tapping those controls would not prove their effects.
- Background proximity notifications require a controlled location path and
  notification delivery checks.
- Publishing and purchases are excluded at the user's request.

See VALIDATION.md for actual run results and limitations. Authored coverage is
not proof of runtime success.

## Local compact-gallery regression (opt-in)

`optional/stop_image_gallery.yaml` follows the current Add tour stop UI: select
multiple images, check `1 / 2`, swipe forward/back, check that no load-error label
appears, and cancel the stop form. It leaves a uniquely named empty test draft.
It does not assert server upload success or saved-stop media persistence.
The optional folder is outside the default `config.yaml` flow patterns.

Run it explicitly, using the same APP_ID/device selection as above:

```sh
maestro --device emulator-5554 test -e APP_ID=org.qtproject.PastViewer .maestro/optional/stop_image_gallery.yaml
maestro --device IOS_SIMULATOR_UDID test -e APP_ID=com.dv.pastviewer .maestro/optional/stop_image_gallery.yaml
```

Android imports `fixtures/pastviewer-maestro-cover.png` and
`fixtures/pastviewer-maestro-second.png` using addMedia. The helper expects the
English system Files picker with filename rows and its multi-select Select action. On iOS,
copy both fixture files into a location visible in Files and make them available
in the document picker's initial view before the run. Importing into Photos
with addMedia does not meet this requirement. The test fails if either fixture
is missing; it does not skip gallery assertions. Both fixtures intentionally use
the same image bytes: the regression checks multiple selection and pagination,
not visual distinction between photographs.

The existing draft lifecycle selects a cover only on Android. iOS still checks
local draft creation/edit/restart without a cover. Saved stop image removal,
Android URI access after restart, and iOS media import remain separate validation
gaps. Upload, publication and purchase tests require a dedicated test backend or
store environment and are excluded from these local flows.
