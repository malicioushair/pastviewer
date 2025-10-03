#!/usr/bin/env bash
SRC_FOLDER=$1

create-dmg \
  --volname "PastViewer Installer" \
  --volicon "$SRC_FOLDER/../resources/mac/PastViewer.icns" \
  --background "$SRC_FOLDER/../resources/dmg_background.png" \
  --window-pos 200 200 \
  --window-size 600 450 \
  --icon-size 128 \
  --icon "PastViewer.app" 150 170 \
  --app-drop-link 450 170 \
  "$SRC_FOLDER/install/PastViewer.dmg" \
  "$SRC_FOLDER/install/"