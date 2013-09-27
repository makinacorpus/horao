#ifndef THREEDSTACK_VIEWER_STRING_UTILS
#define THREEDSTACK_VIEWER_STRING_UTILS

#include <string>
#include <map>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cassert>

#include <boost/algorithm/string/replace.hpp>

// no used yet
//#define NEW_PARSING
#ifdef NEW_PARSING
#include <vector>
#endif


const std::string unescapeXMLString( const std::string& str );

const std::string escapeXMLString( const std::string& str );

struct AttributeMap: private std::map< std::string, std::string >
{
    //! parse a space separated list of key="value" double quote in value are forbidden
    AttributeMap( std::istream & in )
    {
        std::string key, val;
        while (    std::getline( in, key, '=' ) 
                && std::getline( in, val, '"' ) // discarded
                && std::getline( in, val, '"' )){
            // remove spaces in key
            key.erase( remove_if(key.begin(), key.end(), isspace ), key.end());
            (*this)[ key ] = unescapeXMLString( val );
        }
    }

    void setValue( const std::string & key, const std::string & val ){ (*this)[key] = val; }

    const std::string value( const std::string & key ) const
    {
        const const_iterator found = find( key );
        if ( found == end() ) throw std::runtime_error("cannot find attribute '" + key + "'");

        return found->second;
    }

    const std::string optionalValue( const std::string & key ) const
    {
        const const_iterator found = find( key );
        return found == end() ? "" : found->second;
    }
};

inline
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

inline
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

#ifdef NEW_PARSING

//! parse C style
inline
std::istream & operator>>( std::istream & in, AttributeMap & am )
{
    std::string line;
    std::vector<std::string> args;
    bool continued = false;
    while( getline( std::cin, line ) ){
        if (continued) continued = false;
        else args.push_back( std::string() );

        char in='\0';
        bool special = false;
        for ( const char * p = line.c_str(); *p; ++p ){
            if (*p == '\''){
               if( in == '\''  ) {
                   if (!special){
                       // closing string
                       args.push_back( std::string() );
                       in='\0';
                   }
                   else{
                       args.back()+='\'';
                       special = false;
                   }
                   continue;
               }
               else if (in =='\0' && !special){
                   // open string
                   in = '\'';
                   continue;
               }
            }
            if (*p == '\"'){
               if( in == '\"'  ) {
                   if (!special){
                       // closing string
                       args.push_back( std::string() );
                       in='\0';
                   }
                   else{
                       args.back()+='\"';
                       special = false;
                   }
                   continue;
               }
               else if (in =='\0' && !special){
                   // open string
                   in = '\"';
                   continue;
               }
            }

            if ((*p == ' ' || *p == '=') && !special && in == '\0' ){
               args.push_back( std::string() );
               continue;
            }

            if (special) args.back() += '\\';

            if (*p == '\\' && !special) {
                if (!*p) {continued=true; break;}
                else {special=true; continue; }
            }
            else special = false;

            args.back() += *p;
        }

        if (!continued) for (int i=0; i<args.size(); i++) std::cout << args[i] << "\n";
    }
    return 0;
}
#endif
#endif
