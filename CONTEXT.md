# Luma

Luma is the firmware product that runs on the M5Stack Cardputer ADV.

## Language

**Luma**:
The firmware product that runs on Cardputer ADV, and the coordinator that boots the device and runs each frame.
_Avoid_: OS, system, shell

**Core**:
The platform-independent layer: App lifecycle, input frames, and the Settings, Storage, display, and Audio contracts.
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
The App entered after the Boot screen; it is the Home role and the place from which the user opens other Apps. Its Header shows the product word LUMA, not the App name.
_Avoid_: Home (as a type name), menu, desktop, home screen

**App card**:
A Launcher cell that represents one registered App and is the control that opens it.
_Avoid_: icon, tile, shortcut button

**Boot screen**:
The brief Logo splash Luma shows before entering Launcher. It is not an App.
_Avoid_: splash App, boot App, home splash

**Clock**:
The service that provides frame time and local civil time. Civil time is invalid until the device clock is set.
_Avoid_: RTC, wall clock service, timer

**App**:
A statically compiled, registered program the user can enter from Launcher and leave with Back.
_Avoid_: plugin, package, activity, window

**Settings**:
The persisted device preferences: brightness, sound, and Theme preference.
_Avoid_: Preferences, config

**Theme preference**:
The Dark or Light appearance stored in Settings. 0 is Dark; 1 is Light.
_Avoid_: high contrast, Theme (for the stored 0/1 value)

**Design tokens**:
The DESIGN.md colors in `luma::theme`. Theme preference selects which canvas and text mapping those tokens use.
_Avoid_: Theme preference, palette name as a product setting

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

**Audio**:
The service that emits UI sound events. v0.1 has no audio assets.
_Avoid_: mixer, soundtrack, speaker API

**SDL preview**:
The host-side view of the same Apps. Secondary validation, not a second product.
_Avoid_: emulator, simulator, PC firmware
