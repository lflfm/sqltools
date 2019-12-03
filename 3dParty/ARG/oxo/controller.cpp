//  File. . . . : controller.cpp
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
#include <utility>

using namespace oxo;


Controller::Controller(GameDisplay* d, Player* player1, Player* player2)
:   display(d),
    game(),
    toPlay(side_x)
{
    player[0] = player1;
    player[1] = player2;
}


void Controller::run()
{
    do
    {
        bool inPlay = true;
        game.reset(new Game());
        
        do
        {
            do
            {
                displayBoard();
            }
            while (!game->makeMove(
                        player[0]->selectMove(*game, toPlay),
                        (side_x == toPlay ? square_is_x : square_is_o)));
            
            std::swap(player[0], player[1]);
            toPlay = (side_x != toPlay ? side_x : side_o);
            
            switch (game->getState())
            {
            case game_is_draw:
            case game_is_x_win:
            case game_is_o_win:
                inPlay = false;
                break;
                
            case game_x_to_play:
            case game_o_to_play:
                break;
            }
        }
        while (inPlay);

        displayBoard();
    }
    while (display->wantAnotherGame());
    
}


void Controller::displayBoard() const
{
    if (game.get())
    {
        display->displayGamePosition(*game);
    }
}


void Controller::restartGame(Side firstToPlay)
{
    toPlay = firstToPlay;
    game.reset(0);
}
        
