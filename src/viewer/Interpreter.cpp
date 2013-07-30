#include "Interpreter.h"

#include <osgDB/ReadFile>
#include <osg/Material>

#include <QApplication>

#include <iostream>
#include <cassert>

namespace Stack3d {
namespace Viewer {

void Interpreter::operator()()
{
    std::ifstream ifs( _inputFile.c_str() );
    if ( !_inputFile.empty() && !ifs ) std::cout << "error: cannot open '" << _inputFile <<"'\n";
    std::string line;
    while (std::getline(ifs, line) || std::getline(std::cin, line)) {
        if (ifs) std::cout << line << "\n";
        if ( line.empty() || line[0] == '#' ) continue;
        std::stringstream ss( line );
        std::string cmd;
        if ( ss >> cmd ){
            if ( "load" == cmd ){
                load( ss ) || std::cerr << "error: cannot load\n";
            }
            else if ( "unload" == cmd ){
                unload( ss ) || std::cerr << "error: cannot unloaload\n";
            }
            else if ( "list" == cmd ){
                list( ss ) || std::cerr << "error: cannot list\n";
            }
            else {
                std::cerr << "error: '" << cmd << "' command not found\n";
            }
        }
    }
    if (QApplication::instance()) QApplication::instance()->quit();
}

bool Interpreter::list( std::stringstream & ) const
{
    for ( auto l : _nodeMap ) {
        std::cout << "    " << l.first << "\n";
    }
    return true;
}

bool Interpreter::unload( std::stringstream & ss )
{
    std::string layerName;
    if ( ss >> layerName )
    {
        const auto found = _nodeMap.find( layerName );
        if ( found != _nodeMap.end() ){
            _viewer->removeNode( found->second.get() );
        }
        else {
            std::cerr << "error: layer '" << layerName << "' not found\n";
            return false;
        }
    }
    else
    {
        std::cerr << "error: not enough arguments\n";
        return false;
    }
    return true;
}

bool Interpreter::load( std::stringstream & ss )
{
    std::string layerName;
    std::string fileName;
    if ( ss >> layerName >> fileName )
    {
        const auto found = _nodeMap.find( layerName );
        if ( found != _nodeMap.end() ){
            std::cerr << "error: '" << layerName << "' already exists\n";
            return false;
        }
        osg::ref_ptr< osg::Node > scene = osgDB::readNodeFile( fileName );
        if ( !scene.get() ) {
            std::cerr << "error: cannot load '" << fileName << "'\n";
            return false;
        }
        // create white material
        osg::Material *material = new osg::Material();
        material->setDiffuse(osg::Material::FRONT,  osg::Vec4(0.97, 0.97, 0.97, 1.0));
        material->setSpecular(osg::Material::FRONT, osg::Vec4(0.5, 0.5, 0.5, 1.0));
        material->setAmbient(osg::Material::FRONT,  osg::Vec4(0.3, 0.3, 0.3, 1.0));
        material->setEmission(osg::Material::FRONT, osg::Vec4(0.0, 0.0, 0.0, 1.0));
        material->setShininess(osg::Material::FRONT, 25.0);
         
        // assign the material to the scene
        scene->getOrCreateStateSet()->setAttribute(material);

        _viewer->addNode( scene.get() );
        _nodeMap.insert( std::make_pair( layerName, scene.get() ) );
        return true;
    }
    else
    {
        std::cerr << "error: not enough arguments\n";
        return false;
    }
}

}
}
