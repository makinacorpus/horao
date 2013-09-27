/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <viewer/Interpreter.h>


int main()
{
    {
        const std::string query( "SELECT * FROM table /**WHERE TILE && gom*/ /*comment*/" );
        std::cout << query << "\n";
        const std::string squery( Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) );
        std::cout << Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) << "\n";
        assert(  squery == "SELECT * FROM table WHERE ST_MakeEnvelope(-1,-2,3,4) && gom /*comment*/" );
    }

    {
        const std::string query( "SELECT * FROM table WHERE gid=2 /**AND TILE && gom*/ /*comment*/" );
        std::cout << query << "\n";
        const std::string squery( Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) );
        std::cout << Stack3d::Viewer::tileQuery( query, -1, -2, 3, 4 ) << "\n";
        assert(  squery == "SELECT * FROM table WHERE gid=2 AND ST_MakeEnvelope(-1,-2,3,4) && gom /*comment*/" );
    }

    return EXIT_SUCCESS;
}
