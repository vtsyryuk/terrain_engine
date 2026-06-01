set terminal pngcairo size 900,900
set output 'output/em_overlay.png'
unset key
set size square
set xrange [0:900]
set yrange [0:900]
plot 'output/terrain_data_2d.txt' with image, \
'output/em_clusters_plot.txt' using 1:2:(int($3)+1) with points pt 7 ps 1 lc palette notitle
