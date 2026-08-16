# Theme preference selects Dark or Light token invert

RFC #1 named theme=1 high contrast, but v0.1 uses the existing DESIGN.md tokens as a polarity invert instead of a second palette. Theme preference 0 is Dark (Boot `neutral-kuro`, App `neutral-sumi`, text `neutral-gofun`); 1 is Light (Boot and App `neutral-gofun`, text `neutral-sumi`). Boot follows the preference because Settings load before the Boot screen. This revises ADR-0005's "v0.1 is dark only / Boot is always kuro".
