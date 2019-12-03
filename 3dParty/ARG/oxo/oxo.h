//  File. . . . : oxo.h
//  Description : header for classes in oxo package
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

// Avoid duff auto_ptr<> implementations                    
#include "arg_auto_ptr.h"   
                    
#ifndef MOXO_H
#define MOXO_H

namespace oxo
{
    enum constants 
    {
        squares_on_line     = 3, 
        max_lines_on_square = 4, 
        squares_on_board    = 9, 
        lines_on_board      = 8,
        max_moves_in_game   = squares_on_board
    };


    // Wrapper class to avoid typos:
    // (range [0-9) isn't checked)
    class SquareNo
    {
    public:
        explicit SquareNo(int number) : no(number)  {}
        operator int()                      { return no; }
    private:
        int no;
    };
    
    class Line;
    class Square;
    class LineEnumerator;
    class SquareEnumerator;


    // States a line may be in:    
    enum LineState
    {
         line_is_empty, 
         line_has_1x, line_has_2x, line_has_3x, 
         line_has_1o, line_has_2o, line_has_3o, 
         line_is_dead
    };

    // possible contents of a square
    enum SquareContents
    {
         square_is_empty, square_is_x, square_is_o
    };
    
    // states a game may be in
    enum GameState
    {
        game_is_draw, game_is_x_win, game_is_o_win,
        game_x_to_play, game_o_to_play
    };

    // side identitifiers
    enum Side
    {
        side_x, side_o
    };

    
    /**
    *   Keeps track of the relationship of squares.
    **/    
    class Line
    {
    public:
        ///
        Line(Square* s0, Square* s1, Square* s2);
        
        ///
        LineState getState() const;
        
        ///
        void enumerateSquares(SquareEnumerator& enumerator) const;
        
    private:
        // No copy semantics needed
        Line(const Line&);
        Line& operator=(const Line&);
        
        Square* squares[squares_on_line];
    };
    

    /**
    *   Manage a single square.  It has identity, contents, and
    *   knows which lines are associated with it.
    **/    
    class Square
    {
    public:
        ///
        Square(SquareNo identity);
        
        ///
        void enumerateLines(LineEnumerator& enumerator) const;
        
        ///
        SquareNo getIdentity() const;
        
        ///
        SquareContents getContents() const;

        ///
        Square& setContents(SquareContents value);

        // Allows lines to set up associations with squares        
        class LineSetup
        {
            friend Line::Line(Square* s0, Square* s1, Square* s2);
            static void addLineToSquare(Line* newLine, Square* s);
        };
        
    private:
        // No copy semantics needed
        Square(const Square&);
        Square& operator=(const Square&);

        friend LineSetup;
        
        SquareNo       identity;
        SquareContents contents;
        Line* lines[max_lines_on_square];
    };

    
    /**
    *   Allows client code to process a collection of lines.
    **/    
    class LineEnumerator 
    {
    public:
        ///
        virtual void operator()(const Line& line) = 0;
    
    protected:
        virtual ~LineEnumerator() throw() {} 
    };
    
    
    /**
    *   Allows client code to process a collection of squares.
    **/    
    class SquareEnumerator 
    {
    public:
        ///
        virtual void operator()(const Square& square) = 0;
    
    protected:
        virtual ~SquareEnumerator() throw() {} 
    };


    /**
    *   Manage the board state (setting up board, lines etc.)
    **/
    class Board
    {
    public:
        ///
        Board();
        
        /// Explicit copy semantics needed to set up line/square associations
        Board(const Board& board);
        
        /// Explicit copy semantics needed to set up line/square associations
        Board& operator=(const Board& board);
        
        ///
        const Square& getSquare(SquareNo index) const;
        
        ///
        void enumerateSquares(SquareEnumerator& enumerator) const;
        
        ///
        void enumerateLines(LineEnumerator& enumerator) const;
        
        ///
        const Square& setContents(SquareNo index, SquareContents content);
        
    private:
        // We can't provide constructor parameters to array members
        // so this is a little messy...
        Square square0;
        Square square1;
        Square square2;
        Square square3;
        Square square4;
        Square square5;
        Square square6;
        Square square7;
        Square square8;

        Line   line0;
        Line   line1;
        Line   line2;
        Line   line3;
        Line   line4;
        Line   line5;
        Line   line6;
        Line   line7;
    };
    
    
    /**
    *   Control the gameplay
    **/
    class Game
    {
    public:
        ///
        Game();
        
        ///
        const Board& getBoard() const;
        
        ///
        GameState getState() const;
        
        ///
        bool  makeMove(SquareNo index, SquareContents contents);
        
    private:
    
        struct MoveDetails
        {
            MoveDetails() : square(0), piece(square_is_empty), state(game_x_to_play){}
            SquareNo        square;
            SquareContents  piece;
            GameState       state;
        };
        
        // May as well do this as a "stack" now, as munge it later.
        MoveDetails  stack[max_moves_in_game+1];
        MoveDetails* current;
        
        Board board;
    };
    
    
    /**
    *   Interface for displaying progress of the game
    **/
    class GameDisplay
    {
    public:
        ///
        virtual void displayGamePosition(const Game& game) = 0;
        
        ///
        virtual bool wantAnotherGame() = 0;
        
    protected:
        virtual ~GameDisplay() throw() {}
    };
    
    /**
    *   Interface to player
    **/
    class Player
    {
    public:
        ///
        virtual SquareNo selectMove(const Game& game, Side side) = 0;
    };
    
    /**
    *   Co-ordinate the game with the players and display
    **/
    class Controller
    {
    public:
        /**
        * @param display the display, must remain valid for life
        * @param player1 first to play, must remain valid for life
        * @param player2 second to play, must remain valid for life
        **/
        Controller(GameDisplay* display, Player* player1, Player* player2);
        
        /// 
        void run();
        
        ///
        void displayBoard() const;
        
        ///
        void restartGame(Side firstToPlay);
        
    private:
        // No copy semantics needed
        Controller(const Controller&);
        Controller& operator=(const Controller&);
        
        GameDisplay*           display;
        arg::auto_ptr<Game>    game;
        Player*                player[2];
        Side                   toPlay;
    };


    /**
    *   Automaton player
    **/
    class Automaton : public Player
    {
    public:
        Automaton(int playerLevel) : level(playerLevel) {}
        ///
        virtual SquareNo selectMove(const Game& game, Side side);
        
    private:
        int level;
    };
    
}

#endif
