#ifndef STACK3D_VIEWER_LOG_H
#define STACK3D_VIEWER_LOG_H

#include <iostream>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>

#define DEBUG_TRACE std::cerr << __PRETTY_FUNCTION__ << "\n";
#define ERROR  (Stack3d::Viewer::Log::instance() << __FILE__ << ":" << __LINE__ << ": " )
#define WARNING (std::cerr << "warning: " << __FILE__ << ":" << __LINE__  << ": " )

namespace Stack3d {
namespace Viewer {

//! @note no '"' are alowed in error messages
struct Log: std::stringstream
{
    static Log & instance(){ static Log log; return log;}
};

}
}
#endif
