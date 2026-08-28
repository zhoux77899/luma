# DOTS paint is Footer-only so a 60×30 Matrix fills the screen

ADR-0006 keeps Apps on Header plus Footer. DOTS list still does that. The paint view drops the Header so Content is `kContentFooterOnly` (240×120): exactly sixty by thirty LED cells at 4×4 screen pixels, with no pan. A Header would cut the document to 60×22 or force scrolling. Light Theme still paints the Matrix field Kuro; the Footer follows the Theme canvas.
