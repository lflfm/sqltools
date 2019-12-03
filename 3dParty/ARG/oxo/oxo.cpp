//  File. . . . : shell.cpp
//  Description : Initial version of oxo shell
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
#include <iostream>
#include <string>

#ifdef _MSC_VER
#pragma warning(disable : 4355 4127)
#define for if (false);else for
#endif

using namespace oxo;

namespace
{
    class Shell : public GameDisplay, public Player
    {
    public:
        Shell();
        
        /// from GameDisplay
        virtual void displayGamePosition(const Game& game);
        
        /// from GameDisplay
        virtual bool wantAnotherGame();
        
        /// from Player
        virtual SquareNo selectMove(const Game& game, Side side);
        
    private:
        Automaton  automaton;
        Controller controller;
    };
}


Shell::Shell()
:   automaton(0),
    controller(this, this, &automaton)
{
    controller.run();
}
    
void Shell::displayGamePosition(const Game& game)
{
    static char contentText[] = "_XO";
    
    const Board board = game.getBoard();
    
    for (int row = 0; row != 3; ++row)
    {
        for (int col = 0; col != 3; ++col)
        {
            std::cout << 3*row+1+col;
        }
        
        std::cout << "  ";

        for (int col = 0; col != 3; ++col)
        {
            SquareNo sq(3*row+col);
            std::cout << contentText[board.getSquare(sq).getContents()];
        }
        std::cout << '\n';
    }
    
    switch (game.getState())
    {
        case game_is_draw:
            std::cout << " *** game drawn *** ";
            break;
            
        case game_is_x_win:
            std::cout << " *** X wins *** ";
            break;
            
        case game_is_o_win:
            std::cout << " *** O wins *** ";
            break;
            
        case game_x_to_play:
            std::cout << " (X to play) ";
            break;
            
        case game_o_to_play:
            std::cout << " (O to play) ";
            break;
    }
    std::cout << std::endl;
}
    
bool Shell::wantAnotherGame()
{
    std::string answer;

    std::cout << "Another game? [yn]";
    std::cin >> std::ws >> answer;
    std::cin.ignore(9999, '\n');
    
    return 'y' == answer[0];
}


SquareNo Shell::selectMove(const Game&, Side side)
{
    std::string answer;
    int value;

    do
    {
        std::cout << "Move [1-9] for " 
                    << (side_x == side ? "X" : "O") << " :";
    
        std::cin >> std::ws >> answer;
        std::cin.ignore(9999, '\n');
        
        value = atoi(answer.c_str());
        
        if ('?' == answer[0])
        {
            controller.displayBoard();
        }
    }
    while (0 >= value || value > 9);
    
    return SquareNo(--value);
}


int main()
{
    Shell x;
    return 0;
}
