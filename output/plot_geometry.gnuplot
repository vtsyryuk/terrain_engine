set terminal pngcairo size 1000,1000
set output 'output/my_delaunay_voronoi.png'
set xrange [0:900]
set yrange [0:900]
set size square
plot 'output/delaunay.txt' with lines lc rgb 'blue', \
'output/voronoi.txt' with lines lc rgb 'red' lw 2, 'output/points_centers.txt' with points pt 7 ps 2 lc rgb 'black'
