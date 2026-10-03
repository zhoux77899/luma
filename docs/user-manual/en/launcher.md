# LAUNCHER

Launcher is the screen after the Boot screen. It is where you open other Apps. The Header shows the product word LUMA, not the name LAUNCHER.

![Launcher](../assets/luma-home.png)

## Header

The left side is the Logo and LUMA.

The right side is the Header status cluster: a network glyph, a battery glyph, and civil time under them.

The network glyph is four signal levels when LUMA knows RSSI. A slash through the same mark means the station is not connected. The Header does not show dBm.

The battery glyph has six fill levels. Its color is the Battery band: low charge is red, the next band is amber, and the rest is green. The Header does not show a percentage. Charging is not displayed. Cardputer ADV cannot report it.

Civil time is 24-hour `HH:MM`. It stays blank until the Clock has synchronized. After that, a temporary disconnect keeps the last valid time.

## App cards

Launcher shows one App card per registered App. The cards are SETTINGS, NOTES, DOTS, and REMOTE. Launcher itself is not a card.

Each card has an Accent color and a letter. The selected card fills with that Accent.

Arrow keys move in the two-column grid. Confirm opens the selected App. There are no letter shortcuts.

Esc from another App returns here. Launcher has no Footer.

- [Settings](settings.md)
- [Notes](notes.md)
- [DOTS](dots.md)
- [REMOTE](remote.md)
