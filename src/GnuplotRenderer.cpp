#include "GnuplotRenderer.h"
#include "Logger.h"

#include <cstdlib>
#include <string>

void GnuplotRenderer::executeScript(const std::string& script)
{
#ifdef _WIN32
    std::string cmd = "gnuplot \"" + script + "\" 2>NUL";
#else
    std::string cmd = "gnuplot \"" + script + "\" 2>/dev/null";
#endif
    int code = std::system(cmd.c_str());
    if (code != 0) Logger::error("Gnuplot execution failed: " + script);
}
