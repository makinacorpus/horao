#include "Interpreter.h"

#include <QApplication>

using namespace Stack3d::Viewer;

int main( int argc, char** argv )
{
    QApplication app( argc, argv );

    osg::DisplaySettings::instance()->setNumMultiSamples( 4 ); //antialiasing (normally should be set in ViewerWindow, test without once antialiasing is working
    std::unique_ptr< ViewerWidget > viewer( new ViewerWidget() );
    
    Interpreter interpreter( viewer.get(), argc >= 2 ? argv[1] : "" );
    boost::thread interpreterThread( interpreter );

    const int ret = app.exec();

    return ret;
}
