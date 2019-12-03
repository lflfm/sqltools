//  File. . . . : board.cpp
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



// This makes more sense if using this diagram that
// shows the relationship between lines (around edge)
// and squares (in the middle):
//
//    0 1 2 3 4
//     \| | |/
//    5-0 1 2
//    6-3 4 5
//    7-6 7 8
Board::Board()
:   square0(SquareNo(0)),
    square1(SquareNo(1)),
    square2(SquareNo(2)),
    square3(SquareNo(3)),
    square4(SquareNo(4)),
    square5(SquareNo(5)),
    square6(SquareNo(6)),
    square7(SquareNo(7)),
    square8(SquareNo(8)),
    line0(&square0, &square4, &square8),
    line1(&square0, &square3, &square6),
    line2(&square1, &square4, &square7),
    line3(&square2, &square5, &square8),
    line4(&square2, &square4, &square6),
    line5(&square0, &square1, &square2),
    line6(&square3, &square4, &square5),
    line7(&square6, &square7, &square8)
{
}

/// Explicit copy semantics needed to set up line/square associations
Board::Board(const Board& board)
:   square0(SquareNo(0)),
    square1(SquareNo(1)),
    square2(SquareNo(2)),
    square3(SquareNo(3)),
    square4(SquareNo(4)),
    square5(SquareNo(5)),
    square6(SquareNo(6)),
    square7(SquareNo(7)),
    square8(SquareNo(8)),
    line0(&square0, &square4, &square8),
    line1(&square0, &square3, &square6),
    line2(&square1, &square4, &square7),
    line3(&square2, &square5, &square8),
    line4(&square2, &square4, &square6),
    line5(&square0, &square1, &square2),
    line6(&square3, &square4, &square5),
    line7(&square6, &square7, &square8)
{
    *this = board;
}     


/// Explicit copy semantics needed to set up line/square associations
Board& Board::operator=(const Board& board)
{
    square0.setContents(board.square0.getContents());
    square1.setContents(board.square1.getContents());
    square2.setContents(board.square2.getContents());
    square3.setContents(board.square3.getContents());
    square4.setContents(board.square4.getContents());
    square5.setContents(board.square5.getContents());
    square6.setContents(board.square6.getContents());
    square7.setContents(board.square7.getContents());
    square8.setContents(board.square8.getContents());
    
    return *this;
}

///
const Square& Board::getSquare(SquareNo index) const
{
    switch (index)
    {
    default:
    case 0:
        return square0;

    case 1:
        return square1;

    case 2:
        return square2;

    case 3:
        return square3;

    case 4:
        return square4;

    case 5:
        return square5;

    case 6:
        return square6;

    case 7:
        return square7;

    case 8:
        return square8;
    }
}

///
void Board::enumerateSquares(SquareEnumerator& enumerator) const
{
    enumerator(square0);
    enumerator(square1);
    enumerator(square2);
    enumerator(square3);
    enumerator(square4);
    enumerator(square5);
    enumerator(square6);
    enumerator(square7);
    enumerator(square8);
}

///
void Board::enumerateLines(LineEnumerator& enumerator) const
{
    enumerator(line0);
    enumerator(line1);
    enumerator(line2);
    enumerator(line3);
    enumerator(line4);
    enumerator(line5);
    enumerator(line6);
    enumerator(line7);
}

///
const Square& Board::setContents(SquareNo index, SquareContents content)
{
    switch (index)
    {
    default:
    case 0:
        return square0.setContents(content);

    case 1:
        return square1.setContents(content);

    case 2:
        return square2.setContents(content);

    case 3:
        return square3.setContents(content);

    case 4:
        return square4.setContents(content);

    case 5:
        return square5.setContents(content);

    case 6:
        return square6.setContents(content);

    case 7:
        return square7.setContents(content);

    case 8:
        return square8.setContents(content);
    }
}

