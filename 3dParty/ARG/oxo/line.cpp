//  File. . . . : line.cpp
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

Line::Line(Square* s0, Square* s1, Square* s2)
{
    Square::LineSetup::addLineToSquare(this, s0);
    Square::LineSetup::addLineToSquare(this, s1);
    Square::LineSetup::addLineToSquare(this, s2);
    squares[0] = s0; 
    squares[1] = s1;
    squares[2] = s2;
}
    
// Helper for LineState Line::getState() const
// (could do it as local class but that stretches current compilers)
namespace
{
    struct CountContents : public SquareEnumerator
    {
        int x;
        int o;
        
        CountContents() : x(0), o(0) {}
        
        void operator()(const Square& square)
        {
            switch (square.getContents())
            {
            case square_is_empty:
                break;
                
            case square_is_x:
                ++x;
                break;
                
            case square_is_o:
                ++o;
                break;
            }
        }
    };
}

LineState Line::getState() const
{
    CountContents count;
    
    enumerateSquares(count);
    
    if (0 == count.o)
    {
        return LineState(count.x);
    }
    else if (0 == count.x)
    {
        return LineState(count.o + line_has_1o - 1);
    }
    else
    {
        return line_is_dead;
    }
}


// Helper for void Line::enumerateSquares(SquareEnumerator& enumerator) const
// (could do it as local class but that stretches current compilers)
namespace
{
    class SquareEnumerator_ref
    {
    public:
        SquareEnumerator_ref(SquareEnumerator& enumerator) : e(enumerator) {}
        void operator()(const Square* const square)          { e(*square); }
    private:
        SquareEnumerator& e;
    };
}
        
void Line::enumerateSquares(SquareEnumerator& enumerator) const
{
    std::for_each(
        squares+0, 
        squares+squares_on_line, 
        SquareEnumerator_ref(enumerator));
}
