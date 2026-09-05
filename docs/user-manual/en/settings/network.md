# Network

Network is a Settings category. The Core Network service owns Wi-Fi. This page is the category that edits it.

Open [Settings](../settings.md), select Network, then Confirm on the Wi-Fi row.

![Network](../../assets/luma-settings-network.png)

The Wi-Fi row shows the current state. Connected shows the SSID. Other states show Connecting, Failed, Disconnected, or Unknown.

Confirm opens the Wi-Fi nested split: Status, Saved, and Scan.

## Status

![Wi-Fi Status](../../assets/luma-settings-wifi-status.png)

When the station is connected, Status lists State, SSID, Signal in dBm, IP, and Disconnect. Confirm on Disconnect drops the link. After that, LUMA will not reconnect until you connect again or reboot.

When the station is not connected, Status shows only State.

Footer: `Ent ok`, `Esc back`.

## Saved

![Wi-Fi Saved](../../assets/luma-settings-wifi-saved.png)

Saved lists Wi-Fi profiles. LUMA keeps at most five. A profile is stored only after a successful connection. Confirm on a row connects that profile and returns to Status.

`Del forget` removes the selected profile.

An empty list shows `No saved`.

## Scan

![Wi-Fi Scan](../../assets/luma-settings-wifi-scan.png)

Confirm on Scan starts a scan. While it runs, the list shows `Scanning`. If nothing is found, it shows `No networks`.

Each hit shows the SSID, a lock when the network is encrypted, and a signal glyph. Confirm on an open network connects at once and returns to Status.

Confirm on an encrypted hit opens a password field. There is no screenshot of that screen. It shows the SSID, a Password row masked with stars, and a character count. Type the password, Confirm (`Ent join`) to connect, Delete to erase a character, Esc to return to Scan. A password that never connects is a pending credential. It is discarded on Failed and does not become a profile.

Esc from a Wi-Fi section returns to the section list. Esc again returns to the Network category.
