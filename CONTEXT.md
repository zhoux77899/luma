# Luma

Luma is the firmware product that runs on the M5Stack Cardputer ADV.

## Language

**Luma**:
The firmware product that runs on Cardputer ADV, and the coordinator that boots the device and runs each frame.
_Avoid_: OS, system, shell

**Core**:
The platform-independent layer: App lifecycle, input frames, and the Settings, Storage, display, Audio, Clock, and Network contracts.
_Avoid_: shell, OS, runtime

**AppContext**:
The service bag handed to an App on enter: display, Settings, Storage, Clock, Network, and redraw.

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
A Launcher cell that represents one registered App, shows that App's Accent, and is the control that opens it.
_Avoid_: icon, tile, shortcut button

**Boot screen**:
The brief Logo splash Luma shows before entering Launcher. It is not an App.
_Avoid_: splash App, boot App, home splash

**Clock**:
The service that provides frame time and local civil time. Civil time stays invalid until the Clock is synchronized; a temporary disconnect keeps the last valid civil time.
_Avoid_: RTC, wall clock service, timer, NTP service

**Time zone**:
The persisted IANA civil-time selection the Clock applies. The Network category is where the user edits it.
_Avoid_: TZ env, POSIX string, offset setting

**Network**:
The Core service that owns Wi-Fi station connectivity, profiles, scan, and reconnect. It is not the Settings category of the same name. A user Disconnect holds reconnect until the next connect or reboot.
_Avoid_: Wi-Fi manager, connectivity stack, radio

**Wi-Fi profile**:
A remembered SSID and credential pair. At most five are kept, and a profile is persisted only after a successful connection. A pending credential is discarded on Failed and does not become a profile.
_Avoid_: known network, saved network, wifi config

**Wi-Fi section**:
One of Status, Saved, or Scan in the Wi-Fi nested split of the Settings App.
_Avoid_: Settings category, tab

**Pending credential**:
The password typed in the Wi-Fi Scan detail to join an encrypted scan hit. It is not a Wi-Fi profile until the connection succeeds.
_Avoid_: WPA key, PSK, saved password

**Luma UI font**:
The shared bitmap face for all on-screen text. Latin stays narrow; Simplified Chinese uses the wider CJK cell. A missing character becomes one question mark.
_Avoid_: M5GFX Font0, system font, per-host typeface

**Header status cluster**:
The compact right-side Header region that stacks a 10x10 network glyph above civil time, both right-aligned.
_Avoid_: status bar, system tray, RSSI readout

**Signal strength**:
The four-level quantization of a radio. Header, Saved, and Scan share one 10x10 glyph: a 2x2 origin and two arcs when RSSI is known. Weakest paints every layer as secondary text. Non-connected Header states use the same 10x10 mark with a diagonal slash. Numeric dBm does not appear in the Header or in those rows. The Wi-Fi Status Signal row shows dBm without a level name.
_Avoid_: Header RSSI, color-coded status dot, Strong/Mid/Weak as Status copy

**App**:
A statically compiled, registered program the user can enter from Launcher and leave with Back.
_Avoid_: plugin, package, activity, window

**Accent**:
The color that identifies an App. Launcher shows it on that App's card, and the App uses it for selection and interactive emphasis. An App that does not declare one uses Benimidori, the Theme's default selection color.
_Avoid_: Emphasis, highlight, Theme preference, card color

**Settings**:
The persisted device preferences: brightness, Volume, and Theme preference.
_Avoid_: Preferences, config

**Volume**:
The persisted 0..100 UI audio level in Settings. 0 silences UI sound.
_Avoid_: Sound (for the 0..100 value), mute flag, gain

**Theme preference**:
The Dark or Light appearance stored in Settings. 0 is Dark; 1 is Light.
_Avoid_: high contrast, Theme (for the stored 0/1 value)

**Design tokens**:
The DESIGN.md colors in `luma::theme`. Theme preference selects canvas, card, and text mapping. Dark canvas is Kuro with Sumi cards; Light canvas is Gofun with Shironezumi cards. Ginnezumi is secondary text in both Themes.
_Avoid_: Theme preference, palette name as a product setting

**Settings App**:
The App that edits Settings. It uses a category pane and a detail pane.

**Settings category**:
One of Display, Sound, Network, Power, or System in the Settings App.
_Avoid_: tab, menu group

**Category pane**:
The left list of Settings categories.
_Avoid_: sidebar, tab bar

**Detail pane**:
The right list of values or entries for the selected Settings category.
_Avoid_: content pane, inspector

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
The service that emits UI sound events at the current Volume. Click uses a generated tick, not an audio file.
_Avoid_: mixer, soundtrack, speaker API

**SDL preview**:
The host-side view of the same Apps. Secondary validation, not a second product.
_Avoid_: emulator, simulator, PC firmware

**Flash package**:
The zip of split Cardputer ADV flash images and their burn metadata.
_Avoid_: firmware zip, segmented archive

**Merged image**:
The complete 8 MB ESP32-S3 flash image written at 0x00000000.
_Avoid_: combined bin, full flash dump
