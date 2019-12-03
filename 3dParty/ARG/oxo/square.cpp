//  File. . . . : square.cpp
//  Description : 
//  Rev . . . . :
//
//  Classes . . : 
//
//  Modification History
//
//  28-May-99 : Original version                                Alan Griffiths
//

                    /************************************/
                    /* Copyright 1999 Experian limited. */
                    /************************************/
                    
#include "oxo.h"

#include <algorithm>


using namespace oxo;

Square::Square(SquareNo i)
: identity(i),
  contents(square_is_empty)
{
    std::fill(lines+0, lines+max_lines_on_square, (Line*)0);
}
        
// Helper for void Square::enumerateLines(LineEnumerator& enumerator) const
// (could do it as local class but that stretches current compilers)
namespace
{
    class LineEnumerator_ref
    {
    public:
        LineEnumerator_ref(LineEnumerator& enumerator) : e(enumerator) {}
        void operator()(const Line* line)         { if (line) e(*line); }
    private:
        LineEnumerator& e;
    };
}

void Square::enumerateLines(LineEnumerator& enumerator) const
{
    std::for_each(
        lines+0, 
        lines+max_lines_on_square, 
        LineEnumerator_ref(enumerator));
}

SquareNo Square::getIdentity() const
{
    return identity;
}
        

SquareContents Square::getContents() const
{
    return contents;
}


Square& Square::setContents(SquareContents value)
{
    contents = value;
    return *this;
}

        
void Square::LineSetup::addLineToSquare(Line* newLine, Square* s)
{
    *std::find(s->lines+0, s->lines+max_lines_on_square, (Line*)0) = newLine;
}
