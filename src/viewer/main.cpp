#include "Interpreter.h"

#include <QApplication>

using namespace Stack3d::Viewer;

int main( int argc, char** argv )
{
    osg::DisplaySettings::instance()->setNumMultiSamples( 4 );
    QApplication app( argc, argv );
    ViewerWidget* viewWidget = new ViewerWidget();
    viewWidget->show();
    
    // start interpretter
    Interpreter interpreter( viewWidget, argc >= 2 ? argv[1] : "" );
    boost::thread interpreterThread( interpreter );

    const int ret = app.exec();

    interpreterThread.join(); // interpretter has a pointer on the viewer, it's safer to join it befaore the viwer gets destroyed

    return ret;
}
