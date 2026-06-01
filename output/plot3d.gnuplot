set terminal pngcairo size 1200,900
set output 'output/terrain_3d.png'
set title 'Terrain Field (3D)'
set pm3d
splot 'output/terrain_data.txt' with pm3d
