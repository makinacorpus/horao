#include <viewer/Interpreter.h>


int main()
{
    {
    const std::string query("SELECT * FROM table /**WHERE TILE && gom*/ /*comment*/");
    std::cout << query << "\n";
    const std::string squery( Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) );
    std::cout << Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) << "\n";
    assert(  squery == "SELECT * FROM table WHERE ST_MakeEnvelope(-1,3,-2,4) && gom /*comment*/");
    }

    {
    const std::string query("SELECT * FROM table WHERE gid=2 /**AND TILE && gom*/ /*comment*/");
    std::cout << query << "\n";
    const std::string squery( Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) );
    std::cout << Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) << "\n";
    assert(  squery == "SELECT * FROM table WHERE gid=2 AND ST_MakeEnvelope(-1,-2,3,4) && gom /*comment*/");
    }
    
    return EXIT_FAILURE;
}
