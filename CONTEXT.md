# LUMA

LUMA is the firmware product that runs on the M5Stack Cardputer ADV.

## Language

**LUMA**:
The firmware product that runs on Cardputer ADV, and the coordinator that boots the device and runs each frame.
_Avoid_: Luma (as the product spelling), OS, system, shell

**Core**:
The platform-independent layer: App lifecycle, input frames, and the Settings, Storage, display, Audio, Clock, Network, and Battery contracts.
_Avoid_: shell, OS, runtime

**AppContext**:
The service bag handed to an App on enter: display, Settings, Storage, Clock, Network, Battery, and redraw.

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

**App card page**:
One four-row, two-column group of App cards in Launcher. Directional selection turns to another page at an edge when more than one page exists.
_Avoid_: folder, scrolling list, desktop

**Boot screen**:
The brief Logo splash LUMA shows before entering Launcher. It is not an App.
_Avoid_: splash App, boot App, home splash

**Clock**:
The service that provides frame time and local civil time. Civil time stays invalid until the Clock is synchronized; a temporary disconnect keeps the last valid civil time.
_Avoid_: RTC, wall clock service, timer, NTP service

**Time**:
The Settings category for civil-time preferences. It is not the Clock service.
_Avoid_: Clock (as a Settings category), Time zone (as a category name)

**Time zone**:
The persisted IANA civil-time selection the Clock applies. The Time category is where the user edits it.
_Avoid_: TZ env, POSIX string, offset setting

**Time zone section**:
One of UTC, America, Europe, Asia, or Australia in the Time nested split of the Settings App.
_Avoid_: Settings category, continent, tab

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
The compact right-side Header region: a 10x10 network glyph beside a 10x10 battery glyph, civil time right-aligned beneath them. The battery glyph is six fill levels, not a numeric percentage. Its color is the Battery band.
_Avoid_: status bar, system tray, RSSI readout, Header battery percentage

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
One of Display, Sound, Network, Time, Battery, or System in the Settings App.
_Avoid_: tab, menu group

**Battery**:
The Core service that reports charge, samples Battery history, and checkpoints it. It is not a Launcher App. The Settings category of the same name is the pane that shows current charge and the one-hour history. It does not show Charging.
_Avoid_: Power (as a service or Settings category), Battery App

**Charging**:
A Battery reading the platform may not know. LUMA does not display it. Cardputer ADV cannot report it.
_Avoid_: USB connected, Power

**Battery band**:
One of three percent bands the Header battery glyph and Battery history chart share: 0–20 Benihi, 21–30 Yamabuki, 31–100 Wakatake. It is not the six-level Header fill.
_Avoid_: fill level (for this), charging color, Battery range

**Header fill**:
The six-level Header battery glyph quantization of charge: empty, then 1–20, 21–40, 41–60, 61–80, 81–100. It is not the Battery band.
_Avoid_: display node, percentage bar

**Battery sample**:
One minute's Battery reading: percentage, voltage, charging, validity, and time.
_Avoid_: telemetry point, log entry

**Battery history**:
The rolling 60-sample, one-hour window Battery keeps while LUMA runs. A new run after startup is a gap. The Settings chart is right-aligned to now; empty slots on the left are minutes not yet sampled.
_Avoid_: sparkline, power log

**Battery history chart**:
The Settings Battery pane's one-hour bar chart. Percent gridlines mark 0, 50, and 100 without a vertical axis. Elapsed-minute labels mark -60, -30, and 0.
_Avoid_: sparkline, Y-axis

**Category pane**:
The left list of Settings categories.
_Avoid_: sidebar, tab bar

**Detail pane**:
The right list of values or entries for the selected Settings category.
_Avoid_: content pane, inspector

**Notes**:
The App that lists Notes and edits one Notes document at a time.
_Avoid_: notepad

**Note**:
One saved Notes document in the Note list. Notes keeps at most sixteen.
_Avoid_: file, page, entry

**Note list**:
The first screen of Notes: saved Notes newest-modified first inside one rounded shell, each as a separate index chip and Title chip, with New note pinned at the bottom. Overflow keeps the scrollbar inside that shell.
_Avoid_: index, file browser, notebook

**Note title**:
The first line of a Notes document. The editor draws it at size 2; the Note list uses it as the row label.
_Avoid_: filename, heading field, metadata title

**New note**:
The pinned Note list control that opens an empty editor.
_Avoid_: add button, create file

**Notes document**:
The bounded plain-text content of one Note.
_Avoid_: file, rich text

**DOTS**:
The App that lists Matrices and paints one Matrix of LED cells at a time.
_Avoid_: SIGN, LED editor, 排版工具

**Matrix**:
The 60×30 document of LED cells that DOTS paints. DOTS keeps at most sixteen.
_Avoid_: canvas, Slot (as the user-facing name), file, sign

**Matrix list**:
The first screen of DOTS: saved Matrices newest-modified first, each as a separate index chip and name chip, with New matrix pinned at the bottom.
_Avoid_: Slot list, file browser, index

**New matrix**:
The pinned Matrix list control that opens an empty paint view.
_Avoid_: add button, create file, New slot

**LED cell**:
One lamp in a Matrix. Off is Kuro; a lit cell holds one Pen color.
_Avoid_: screen pixel, lamp pixel

**Dot**:
The lit drawing of one LED cell: a center mark with a Kuro gutter.
_Avoid_: circle API, bitmap cell

**Matrix cursor**:
The current LED cell in the DOTS paint view.
_Avoid_: canvas cursor, pointer, caret

**Pen color**:
The palette color Confirm paints onto the current LED cell. Off is not a Pen color; Delete extinguishes.
_Avoid_: brush, ink, Theme preference

**About**:
The System nested view that identifies the installed LUMA build, hardware, and repository.
_Avoid_: About App

**Build identity**:
The version string About shows: a release is X.Y.Z; an unreleased build is X.Y.Z.{commit}.
_Avoid_: semver +build, git describe

**Cardputer ADV**:
The M5Stack hardware LUMA runs on; the v0.1 release authority.
_Avoid_: DevKit, ESP32 board

**Audio**:
The service that emits UI sound events at the current Volume. Click uses a generated tick, not an audio file.
_Avoid_: mixer, soundtrack, speaker API

**SDL preview**:
The host-side view of the same Apps. Secondary validation, not a second product.
_Avoid_: emulator, simulator, PC firmware

**Preview canvas**:
The 240×135 logical DisplaySurface buffer the SDL preview presents. It is the same pixel grid as Cardputer ADV.
_Avoid_: simulated screen, emulator framebuffer, host window

**Preview screenshot**:
A capture of the current integer-scaled, brightness-presented Preview canvas, without letterbox or menu chrome.
_Avoid_: window screenshot, full-window capture, device dump

**Preview menu bar**:
The native OS File menu on the Windows or macOS SDL preview window.
_Avoid_: in-window menu, application shell, toolbar

**Preview menu strip**:
The Linux-only in-window File strip reserved at the top of the SDL preview client. It is not part of a Preview screenshot.
_Avoid_: Preview menu bar (on Linux), title bar, overlay HUD

**Flash package**:
The zip of split Cardputer ADV flash images and their burn metadata.
_Avoid_: firmware zip, segmented archive

**Merged image**:
The complete 8 MB ESP32-S3 flash image written at 0x00000000.
_Avoid_: combined bin, full flash dump

**User guide**:
The bilingual handbook for flashing LUMA and using each App. Its source is `docs/user-manual/`.
_Avoid_: user-manual (as the product name), docs, documentation, wiki

**User guide site**:
The published site that presents the User guide with a product cover. App names on that site match the firmware name() paint: LAUNCHER, SETTINGS, NOTES, DOTS.
_Avoid_: docs site, documentation portal, LUMA website, 官网
