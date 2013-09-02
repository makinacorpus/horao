#include "Interpreter.h"
#include <X11/Xlib.h>

using namespace Stack3d::Viewer;

int main( int argc, char** argv )
{
    XInitThreads();
    osg::ref_ptr<ViewerWidget> viewer = new ViewerWidget;
    Interpreter interpreter( viewer.get(), argc >= 2 ? argv[1] : "" );
    interpreter.startThread();
    const int ret = viewer->run();
    // force termination of interpreter thread, if still running
    interpreter.cancel();
    return ret;
}
