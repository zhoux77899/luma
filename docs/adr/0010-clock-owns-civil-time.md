# Clock owns civil time; Network owns the radio

NTP and IANA conversion belong on Clock so Apps read one civil-time seam. Network stays a Wi-Fi policy object (scan, profiles, reconnect) and only exposes a Connected edge. Putting sync APIs on Network would couple time to connectivity and duplicate Host/Cardputer NTP fakes. Luma calls `Clock.synchronize()` when Network reports that edge.
