#include "Interpreter.h"

using namespace Stack3d::Viewer;

struct InterpreterThread : public OpenThreads::Thread
{
    InterpreterThread( Interpreter & i ): _interpreter( i ) {}

private:
    bool _done;
};

int main( int argc, char** argv )
{
    ViewerWidget viewer;
    
    Interpreter interpreter( &viewer, argc >= 2 ? argv[1] : "" );
    boost::thread interpreterThread( interpreter );

    return viewer.run();
}
