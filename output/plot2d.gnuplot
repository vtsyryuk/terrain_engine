set terminal pngcairo size 900,900
set output 'output/terrain_2d.png'
plot 'output/terrain_data_2d.txt' with image
