#include "ClusterVisualizer.h"
#include "GnuplotRenderer.h"
#include <fstream>
#include <sstream>

namespace ClusterVisualizer {

void visualize(const std::string& outFilename)
{
    // Expect 'terrain_data_2d.txt' (image data) and 'em_clusters.txt' (x y label)
    std::ifstream fin("output/em_clusters.txt");
    if (!fin.is_open()) return;

    // Create gnuplot datafile with cluster colors
    std::ofstream pdata("output/em_clusters_plot.txt");
    int label; double x,y;
    int maxLabel = 0;
    while (fin >> x >> y >> label) { pdata << x << " " << y << " " << label << "\n"; if (label>maxLabel) maxLabel=label; }
    pdata.close(); fin.close();

    std::ofstream gp("output/plot_clusters.gnuplot");
    gp << "set terminal pngcairo size 900,900\n";
    gp << "set output '" << outFilename << "'\n";
    gp << "unset key\nset size square\nset xrange [0:900]\nset yrange [0:900]\n";
    gp << "plot 'output/terrain_data_2d.txt' with image, \\\n'output/em_clusters_plot.txt' using 1:2:(int($3)+1) with points pt 7 ps 1 lc palette notitle\n";
    gp.close();

    GnuplotRenderer::executeScript("output/plot_clusters.gnuplot");
}

} // namespace
