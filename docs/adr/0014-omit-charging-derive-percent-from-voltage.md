# Omit Charging; derive percent from voltage

ADR 0013 paints the Header battery glyph Wakatake while charging. Cardputer ADV cannot report Charging: `M5.Power.isCharging()` is `charge_unknown`, which became a permanent Charging label when stored as a bool. Luma therefore omits Charging from Settings and from the Cardputer reading. Percent comes from `getBatteryVoltage()` via the same 3300–4150 mV curve as M5Unified, not from `getBatteryLevel()`, which stays 0% on battery and jitters across Header fill buckets on USB.
