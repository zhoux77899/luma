# Launcher keeps four-row App card pages as the registry grows

The next-phase App catalog outgrows one Launcher page, so the static registry holds
32 Apps including Launcher and App cards continue in registration order across
four-row, two-column pages. This preserves the existing card size and ADR-0004's
Header-only chrome without a page indicator, Footer, or KeyHint. With multiple
pages, a missing adjacent card is an edge: Left and Up turn to the previous page,
Right and Down to the next, wrapping between the first and last pages. Horizontal
turns keep the row (or the target page's last occupied row), selecting its rightmost
card for Left and leftmost for Right; vertical turns keep the column (or fall back
to the left column), selecting its last card for Up and first for Down. A single
page still stops at its edges, and entering Launcher still selects its first card.
