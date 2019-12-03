//  File. . . . : game.cpp
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



Game::Game()
:   current(stack)
{
}


const Board& Game::getBoard() const
{
    return board;
}


GameState Game::getState() const
{
    return current->state;
}


// Helper class for bool  makeMove(SquareNo index, SquareContents contents)
namespace 
{

    struct FindNewState : LineEnumerator
    {
        GameState state;
        int o;
        
        FindNewState(SquareContents contents, bool finished) 
        : state(square_is_o == contents ? game_x_to_play : game_o_to_play) 
        {
            // At the end of the game
            if (finished)
            {
                state = game_is_draw;
            }
        }
        
        void operator()(const Line& line)
        {
            switch (line.getState())
            {
            case line_is_empty:
            case line_has_1x:
            case line_has_2x: 
            case line_has_1o:
            case line_has_2o:
            case line_is_dead:
                break;

            case line_has_3x:
                state = game_is_x_win;
                break;
                
            case line_has_3o:
                state = game_is_o_win;
                break;
            }
        }
    };
    
}

bool  Game::makeMove(SquareNo index, SquareContents contents)
{
    bool rval = false;
    
    if (square_is_empty != contents &&
        square_is_empty == board.getSquare(index).getContents())
    {
        FindNewState update(contents, stack+max_moves_in_game-1 == current);

        board.setContents(index, contents).enumerateLines(update);
        
        // Record result...
        ++current;
        current->square = index;
        current->piece = contents;
        current->state = update.state;
        
        
        rval = true;
    }
    
    return rval;
}
