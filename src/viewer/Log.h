#ifndef STACK3D_VIEWER_LOG_H
#define STACK3D_VIEWER_LOG_H

#include <iostream>
#include <string>

#define DEBUG_TRACE ( std::cerr << __FILE__ << ":" << __LINE__ << ": debug: ")
#define ERROR       ( std::cerr << __FILE__ << ":" << __LINE__ << ": error: " )
#define WARNING     ( std::cerr << __FILE__ << ":" << __LINE__ << ": warning: " )

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define Exception( WHAT )  Stack3d::Viewer::Error( std::string(__FILE__ ":" STRINGIZE(__LINE__) ": error: ") + WHAT )

namespace Stack3d {
namespace Viewer {

struct Error: std::exception
{
    Error( const std::string & msg ):_msg(msg){}
    const char * what() const noexcept { return _msg.c_str(); }
private:
    const std::string _msg;
};

}
}

#endif
