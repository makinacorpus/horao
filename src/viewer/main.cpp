/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
    interpreter.join();
    return ret;
}
