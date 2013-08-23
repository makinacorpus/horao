#include <osgDB/FileNameUtils>
#include <osgDB/ReaderWriter>
#include <osgDB/Registry>

#include <iostream>

struct ReaderWriterPOSTGIS : osgDB::ReaderWriter
{
    ReaderWriterPOSTGIS()
    {
        supportsExtension( "postgis", "PostGIS feature driver for osgEarth" );
        supportsExtension( "postgisd", "PostGIS feature driver for osgEarth" );
    }

    virtual const char* className()
    {
        return "PostGIS Feature Reader";
    }

    virtual ReadResult readNode(const std::string& file_name, const Options* ) const
    {
        if ( !acceptsExtension(osgDB::getLowerCaseFileExtension( file_name )))
            return ReadResult::FILE_NOT_HANDLED;

        std::cerr << "loaded plugin postgis\n";

        return ReadResult::NOT_IMPLEMENTED;
    }
};

REGISTER_OSGPLUGIN(postgis, ReaderWriterPOSTGIS)


