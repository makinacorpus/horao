#ifndef STACK3D_VIEWER_LOG_H
#define STACK3D_VIEWER_LOG_H

#include <iostream>

// little trick to close error message tags, note that character '"' is not allowed en error message
struct EndErr { ~EndErr(){std::cerr << "\"/>"<< std::endl; } };
#define DEBUG_TRACE std::cerr << __PRETTY_FUNCTION__ << "\n";
#define ERROR   (EndErr(), (std::cerr << "<error   msg=\"" << __FILE__ << ":" << __LINE__ << " " ))
#define WARNING (EndErr(), (std::cerr << "<warning msg=\"" << __FILE__ << ":" << __LINE__ << " " ))

#endif
