#include "Interpreter.h"

#include <osgDB/ReadFile>
#include <osg/Material>

#include <QApplication>
#include <QSqlDatabase>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>

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
                load( ss ) || std::cerr << "error: cannot " << cmd << "\n";
            }
            else if ( "unload" == cmd ){
                unload( ss ) || std::cerr << "error: cannot " << cmd << "\n";
            }
            else if ( "list" == cmd ){
                list( ss ) || std::cerr << "error: cannot " << cmd << "\n";
            }
            else if ( "help" == cmd ){
                help( ss ) || std::cerr << "error: cannot " << cmd << "\n";
            }
            else {
                std::cerr << "error: '" << cmd << "' command not found\n";
            }
        }
        std::string dummy;
        if ( ss >> dummy ){
            std::cerr << "error: too many argument ('" << dummy << "' was not parsed)\n";
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
    if ( !( ss >> layerName ) ) {
        std::cerr << "error: not enough arguments\n";
        return false;
    }

    const auto found = _nodeMap.find( layerName );
    if ( found == _nodeMap.end() ){
        std::cerr << "error: layer '" << layerName << "' not found\n";
        return false;
    }

    _viewer->removeNode( found->second.get() );
    return true;
}

bool Interpreter::load( std::stringstream & ss )
{
    std::string layerName;
    std::string sourceType;
    if ( !( ss >> layerName >> sourceType ) ) {
        std::cerr << "error: not enough arguments\n";
        return false;
    }
    if ( _nodeMap.find( layerName ) != _nodeMap.end() ){
        std::cerr << "error: '" << layerName << "' already exists\n";
        return false;
    }

    osg::ref_ptr< osg::Node > scene;

    if ( "file" == sourceType ) {
        scene = loadFile( ss );
    }
    else if ( "postgis" == sourceType ) {
        scene = loadPostgis( ss );
    }
    else {
        std::cerr << "error: '" << sourceType << "' unknown source type\n";
        return false;
    }

    if ( !scene.get() ) {
        std::cerr << "error: cannot create '" << layerName << "'\n";
        return false;
    }

    _viewer->addNode( scene.get() );
    _nodeMap.insert( std::make_pair( layerName, scene.get() ) );
    return true;
}

osg::ref_ptr< osg::Node > Interpreter::loadFile( std::stringstream & ss )
{
    std::string fileName;
    if ( !(ss >> fileName ) ) {
        std::cerr << "error: not enough arguments\n";
        return nullptr;
    }
    osg::ref_ptr< osg::Node > scene = osgDB::readNodeFile( fileName );
    if ( !scene.get() ) {
        std::cerr << "error: cannot load '" << fileName << "'\n";
        return nullptr;
    }
    // create white material
    osg::Material *material = new osg::Material();
    material->setDiffuse(osg::Material::FRONT,  osg::Vec4(0.9, 0.9, 0.9, 1.0));
    material->setSpecular(osg::Material::FRONT, osg::Vec4(0.5, 0.5, 0.5, 1.0));
    material->setAmbient(osg::Material::FRONT,  osg::Vec4(0.5, 0.5, 0.5, 1.0));
    material->setEmission(osg::Material::FRONT, osg::Vec4(0.1, 0.1, 0.1, 1.0));
    material->setShininess(osg::Material::FRONT, 25.0);
     
    // assign the material to the scene
    scene->getOrCreateStateSet()->setAttribute(material);

    return scene;
}

osg::ref_ptr< osg::Node > Interpreter::loadPostgis( std::stringstream & ss ) {
    std::string host, dbname, table, user, passwd;
    if ( !(ss >> host >> dbname >> table >> user >> passwd ) ) {
        std::cerr << "error: not enough arguments\n";
        return nullptr;
    }

    osg::ref_ptr< osg::Node > scene;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", "posgisDB");
        db.setHostName( QString( host.c_str() ) );
        db.setDatabaseName( QString( dbname.c_str() ) );
        db.setUserName( QString( user.c_str() ) );
        db.setPassword( QString( passwd.c_str() ) );
        if ( !db.open() ) {
            std::cerr << "error: cannot open database (" << db.lastError().text().toStdString() << ")\n";
            QSqlDatabase::removeDatabase("posgisDB");
            return nullptr;
        }

        QSqlQuery query("SELECT id, name FROM " + QString( table.c_str() ), db );
        query.exec();
         
        std::cout << "found " << query.size() << " record\n";
         
        //for (int i=0; i<numRows; ++i) {
        //    // Read fields
        //    qlonglong id = model->record(i).value("id").toLongLong();
        //    QByteArray wkb = model->record(i).value("the_geom").toByteArray();
        //    std::cout << "recors " << id << "\n";
        // 
        //    // Process !
        //    //processRecord(id, wkb);
        //}
    
    }
    QSqlDatabase::removeDatabase("posgisDB");
    return scene;
}

}
}
