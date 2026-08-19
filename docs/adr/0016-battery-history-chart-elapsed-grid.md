# Battery history chart uses elapsed-minute labels and percent gridlines

The Settings Battery detail is about 130px wide, so a labeled vertical axis cannot sit beside the 120px bar field. The chart therefore has no vertical axis: three Ginnezumi gridlines mark 0, 50, and 100 percent under the bars, and the plot is a fixed 50px so one pixel is two percent. Bars are right-aligned to now, with -60, -30, and 0 elapsed-minute labels, so a window that is not yet full does not pretend the left edge is an hour ago of data.
