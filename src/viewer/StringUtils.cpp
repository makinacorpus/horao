#include <boost/algorithm/string/replace.hpp>

#include "StringUtils.h"
#include <cassert>

const std::string unescapeXMLString( const std::string & str )
{
    std::string out(str);
    boost::replace_all( out, "&quot;", "\"" );
    boost::replace_all( out, "&#10;", "\n" );
    boost::replace_all( out, "&amp;", "&" );
    assert( out.find("&quot;") == std::string::npos );
    assert( out.find("&#10;") == std::string::npos );
    assert( out.find("&amp;") == std::string::npos );
    return out;
}

const std::string escapeXMLString( const std::string& str )
{
    std::string out;
    for ( size_t i = 0; i < str.size(); ++i ) {
        if ( str[i] == '"' ) {
            out += "&quot;";
        }
        else if ( str[i] == '\n' ) {
            out += "&#10;";
        }
        else if ( str[i] == '&' ) {
            out += "&amp;";
        }
        else {
            out.push_back( str[i] );
        }
    }
    return out;
}
