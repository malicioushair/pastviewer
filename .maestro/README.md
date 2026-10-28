# PastViewer UI regression tests

These flows exercise the installed app, not a newly built copy of the source.
Use an iPhone 17 Pro simulator in portrait (402 × 874 points), English UI, with
location and notification permissions already granted. The installed build must
expose Settings, Tours, Back, Compass, Re-center and History near you accessibility
labels. Some Qt text inputs and image gestures still require documented points.

## Running

Every flow and helper requires `APP_ID`. Choose the installed app's ID when
running a test; do not rely on an iOS default for Android.

In Maestro Studio, open this folder and create two environments under Settings →
Environments: set `APP_ID` to `org.qtproject.PastViewer` for Android and to
`com.dv.pastviewer` for iOS. Select the appropriate environment before running.

For the CLI, pass the app ID explicitly for a directory or individual flow:

```sh
# Android
maestro test -e APP_ID=org.qtproject.PastViewer .maestro

# iOS
maestro test -e APP_ID=com.dv.pastviewer .maestro

# Single flow example
maestro test -e APP_ID=org.qtproject.PastViewer .maestro/smoke_launch.yaml
```

Run on one device serially. Helpers are nested and tagged `helper`; they are not
standalone tests. `config.yaml` includes both smoke and regression flows.
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
| regression_onboarding | Reset, all map steps, complete, restart, skip and restart |
| regression_tour_gallery | Start tour, stop details, no-audio state, two-image swipe both ways, inspect/back, next/return, exit |
| regression_creation_cancel | Admin menu, browse, both creation choices and cancellation, drafts navigation |
| regression_draft_lifecycle | Local creation, point removal cancel/confirm, stop-form cancel, rename/save, restart persistence, route reopening |

On Android, the draft lifecycle flow adds the saved screenshot in
`fixtures/pastviewer-maestro-cover.png` to the emulator and selects it in the
native file picker. It also captures a separate screenshot immediately before
opening the picker for run diagnostics. On iOS, the flow skips image selection:
importing the fixture into the native Files picker is not yet covered.

The gallery fixture defaults to the catalog tour `multi-img`, with a first stop
named `st1`, exactly two images and no narration. It must be free or already
owned. `GALLERY_TOUR` can override the tour name but the fixture contract still
applies. Missing fixtures fail explicitly rather than skipping assertions.
Existing smoke tests also require one of the named catalog tours.

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
  Stop audio selection currently calls UploadAsset immediately, so it is not a
  local-only test.
- Timeline and notification-distance sliders, map centering/rotation and route
  geometry need observable values/accessibility support or visual assertions;
  merely tapping those controls would not prove their effects.
- Background proximity notifications require a controlled location path and
  notification delivery checks.
- Publishing and purchases are excluded at the user's request.

See VALIDATION.md for actual run results and limitations. Authored coverage is
not proof of runtime success.
