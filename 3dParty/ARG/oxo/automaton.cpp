//  File. . . . : automaton.cpp
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

#include <map>
#include <functional>


using namespace oxo;

// Helper classes for SquareNo Automaton::selectMove()
namespace
{
    // The square gets a score of 10 for each line that would be
    // completed, 2 for each line that would have two, and one if
    // it is unoccupied.
    struct ScoreSquare : LineEnumerator
    {
        int x;
        int o;
        
        ScoreSquare() 
        : x(0), o(0)
        {
        }
        
        void operator()(const Line& line)
        {
            switch (line.getState())
            {
            case line_is_empty:
                x += 1;
                o += 1;
                break;
                
            case line_has_1x:
                x += 2;
                break;
                
            case line_has_2x: 
                x += 10;
                break;

            case line_has_1o:
                o += 2;
                break;
                
            case line_has_2o:
                o += 10;
                break;

            case line_is_dead:
                break;

            case line_has_3x:
                break;
                
            case line_has_3o:
                break;
            }
        }
    };
    
    struct CollateMoves : public SquareEnumerator
    {
        // score and square
        std::multimap<int, int, std::greater<int> > x_moves;
        std::multimap<int, int, std::greater<int> > o_moves;
        
        void operator()(const Square& square)
        {
            if (square_is_empty == square.getContents())
            {
                typedef std::multimap<int, int, std::greater<int> >::value_type val;
                
                ScoreSquare scorer;
                
                square.enumerateLines(scorer);
                x_moves.insert(val(scorer.x, square.getIdentity()));
                o_moves.insert(val(scorer.o, square.getIdentity()));
            }
        }
    };
}

SquareNo Automaton::selectMove(const Game& game, Side side)
{
    typedef std::multimap<int, int, std::greater<int> >::iterator Iter;
    CollateMoves movemarker;
    
    game.getBoard().enumerateSquares(movemarker);
    
    // Pretend we are alway "O"...
    Iter beginAttack = movemarker.o_moves.begin();
    Iter endAttack = movemarker.o_moves.end();
    
    Iter beginDefence = movemarker.x_moves.begin();
    Iter endDefence = movemarker.x_moves.end();
    
    // ...if not then correct
    if (side_x == side)
    {
        std::swap(beginAttack, beginDefence);
        std::swap(endAttack, endDefence);
    }
    
    int rval;
    
    if (beginAttack == endAttack)
    {
        rval = beginDefence->second;
    }
    else if (beginDefence == endDefence)
    {
        rval = beginAttack->second;
    }
    else if (beginAttack->first > beginDefence->first)
    {
        rval = beginAttack->second;
    }
    else
    {
        rval = beginDefence->second;
    }
    
    return SquareNo(rval);
}
