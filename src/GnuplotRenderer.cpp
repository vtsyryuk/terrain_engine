#include "GnuplotRenderer.h"
#include <cstdlib>
#include "Logger.h"

void GnuplotRenderer::executeScript(const std::string& script)
{
    std::string cmd = "gnuplot \"" + script + "\" 2>/dev/null";
    int code = std::system(cmd.c_str());
    if (code != 0) Logger::error("Gnuplot execution failed: " + script);
}
