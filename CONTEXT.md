# Luma

Luma is the firmware product that runs on the M5Stack Cardputer ADV.

## Language

**Luma**:
The firmware product that runs on Cardputer ADV, and the coordinator that boots the device and runs each frame.
_Avoid_: OS, system, shell

**Core**:
The platform-independent layer: App lifecycle, input frames, and the Settings, Storage, and display contracts.
_Avoid_: shell, OS, runtime

**AppContext**:
The service bag handed to an App on enter: display, Settings, Storage, Clock, and redraw.

**AppManager**:
The owner of the one current App and the static registry that routes enter, exit, Back, and shortcuts.

**InputManager**:
The single reader of an InputSource; it reports InputFrame values and does not decide which App to open.

**InputFrame**:
One frame of normalized input: an action, optional text, and pressed/repeated flags.
_Avoid_: key event, raw keyboard state

**Launcher**:
The App entered after boot; the place from which the user opens other Apps.
_Avoid_: Home (as a type name), menu, desktop, home screen

**App**:
A statically compiled, registered program the user can enter from Launcher and leave with Back.
_Avoid_: plugin, package, activity, window

**Settings**:
The persisted device preferences: brightness, sound, and theme.
_Avoid_: Preferences, config

**Settings App**:
The App that edits Settings.

**Notes**:
The App that edits a bounded plain-text Notes document.
_Avoid_: notepad

**Notes document**:
The bounded plain-text content Notes edits.
_Avoid_: file, note, rich text

**About**:
The read-only App that identifies the installed Luma build and target hardware.

**Cardputer ADV**:
The M5Stack hardware Luma runs on; the v0.1 release authority.
_Avoid_: DevKit, ESP32 board

**SDL preview**:
The host-side view of the same Apps. Secondary validation, not a second product.
_Avoid_: emulator, simulator, PC firmware
