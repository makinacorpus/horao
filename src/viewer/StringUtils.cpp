#include <boost/algorithm/string/replace.hpp>

#include "StringUtils.h"

std::string unescapeXMLString( const std::string& str )
{
    std::string out;
    out = boost::replace_all_copy( str, "&quot;", "\"" );
    boost::replace_all( out, "&#10;", "\n" );
    boost::replace_all( out, "&amp;", "&" );
    return out;
}

std::string escapeXMLString( const std::string& str )
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
