#include "board.h"


extern bool onOffSound;

board::board()
{
	bool i = false;
	// create entire board pieces with type onto board.
	// white first then black. the rest is black.
	do
	{
		_board[7 * !i][0] = new Pieces::Rook(i);
		_board[7 * !i][7] = new Pieces::Rook(i);
		_board[7 * !i][1] = new Pieces::Knight(i);
		_board[7 * !i][6] = new Pieces::Knight(i);
		_board[7 * !i][2] = new Pieces::Bishop(i);
		_board[7 * !i][5] = new Pieces::Bishop(i);
		_board[7 * !i][3] = new Pieces::Queen(i);
		_board[7 * !i][4] = new Pieces::King(i);
		for (int j = 0; j < SIZE; j++)
		{
			_board[7 * !i + 1 - 2 * !i][j] = new Pieces::Pawn(i);
		}
		i = !i;
	} while (i);
}


board::~board()
{
	onOffSound = false;
	// delete every single piece on board
	for (int i = 0; i < SIZE*SIZE; i++)
	{
		if (_board[0][i] != nullptr) { delete _board[0][i]; }
	}
}


std::string board::getLastMove() const
{
	return _log.back();
}


/*
* check if a point is inside board
* perimeters.
* input: point to check
* output: if the point exists within the board
*/
bool board::isInboard(const Point& point) const
{
	return ((0 <= point.getX() && point.getX() < SIZE) && (0 <= point.getY() && point.getY() < SIZE));
	
}


/*
* checks if the current player has a piece
* in a certain location.
* input: point to check, player flag
*/
bool board::playerInPos(const Point& pnt, const bool player) const
{
	if ((*this)[pnt]==nullptr) { return false; }
	return ((player) ? 'a' : 'A') <= (*this)[pnt]->getType() && (*this)[pnt]->getType() <= ((player) ? 'z' : 'Z');
}


/*
* checks if a blocking piece exists between two points.
* input: source and destenation points.
* output: if the path is blocked
*/
bool board::isBlockedPath(const Point& src, const Point& dst) const
{
	for (Point i(src+ Point::direction(src, dst)); i!=dst;i+=Point::direction(src,dst)){
		if((*this)[i]!=nullptr){return true;}
	}
	return false;	
}


/*
* checks if a current player piece exists that
* can reach the selected destination point.
*/
bool board::canReach(const Point& dst, const bool player) const
{
	for (int row = 0; row < SIZE; row++)
	{
		for (int col = 0; col < SIZE;col++) {
			if (!subMovePiece(Point(col, row),dst, player))
			{
				return true;
			}
		}
	}
	return false;
}


bool board::checkIfMovesExist(const bool player)
{
	//varibales: rows, cols (row/col src) and rowd, cold(row/col dst)
	for (int rows = 0; rows < SIZE; rows++)
	{
		for (int cols = 0; cols < SIZE;cols++) {
			for (int rowd = 0; rowd < SIZE; rowd++)
			{
				for (int cold = 0; cold < SIZE; cold++)
				{
					Point src(cols, rows), dst(cold, rowd);
					Piece* temp[2] = {nullptr};
					onOffSound = false;
					int t = canMovePiece(src, dst, player, temp);
					onOffSound = true;
					if (!t || t==1)
					{
						movePiece(dst, src);
						((_enPassent) ? _board[src.getY()][dst.getX()] : _board[dst.getY()][dst.getX()]) = temp[0];
						if (_board[src.getY()][src.getX()] != temp[1]) {
							onOffSound = false;
							delete _board[src.getY()][src.getX()];
							onOffSound = true;
							_board[src.getY()][src.getX()] = temp[1];
						}
						return true;
					}
				}
			}
		}
	}
	return false;
}


/*
* Checks if any possible valid move exists
* for current player.
* input:
*		src - source point
*		dst - destination point
*		player - true for white, false for black 
*		(for current player)
*		movementPieces - contains eaten and eater pieces.
* 
* output: if any player piece on the board can be moved.
*/
int board::canMovePiece(const Point& src, const Point& dst, const bool player, Piece * movementPieces[2]) // NOTE: the function does way more then check if can move.
{
	Piece*& deleted = *movementPieces; 
	_enPassent = false;
	// check if any clear error exist
	int ret = subMovePiece(src,dst,player);

	if (!ret){
		Piece* temp[2] = { (_enPassent) ? _board[src.getY()][dst.getX()] : _board[dst.getY()][dst.getX()],_board[src.getY()][src.getX()] };
		movePiece(src, dst);
		if (dst.getY() == (player ? 7 : 0) && (*this)[dst]->getType() == (player ? 'p' : 'P')) { // NOTE: Not commented!!!!
			temp[1] = _board[dst.getY()][dst.getX()];
			_board[dst.getY()][dst.getX()] = new Pieces::Queen(player);
			
		}
		if (_enPassent) { _board[src.getY()][dst.getX()] = nullptr; }

		// check if move will check current player
		if (canReach(find(player ? 'k' : 'K'), !player)) {  // NOTE: STP- depend on the very specific pieces class, if changed not work.
			movePiece(dst, src);
			(_enPassent) ? _board[src.getY()][dst.getX()] : _board[dst.getY()][dst.getX()] = temp[0];
			if ((*this)[src]!=temp[1]){
				onOffSound = false;
				delete _board[dst.getY()][dst.getX()];
				onOffSound = true;
				_board[dst.getY()][dst.getX()] = temp[1];
			}
			return 4;
		}
		// check if moved checked opponent
		else if (canReach(find(!player ? 'k' : 'K'), player))  
		{
			ret = 1;
		}

		// save existing movement pieces
		if (&deleted != nullptr) { deleted = temp[0]; movementPieces[1] = temp[1]; } 

		// deletes eaten pieces if they are not saved
		else if (temp[0] != nullptr) { delete temp[0]; if ((*this)[dst] != temp[1]) 
		{
			onOffSound = false;
			delete temp[1];
			onOffSound = true;
		} }

		// plays sound for uneaten pieces (regular step)
		else if (ret != 1) { playSound(move); }
		// play development sound
		if ((*this)[dst] != temp[1]) { playSound(proneToQueen); }
		_board[dst.getY()][dst.getX()]->setMoved(true);
		_log.push_back(convertPositionType(src) + convertPositionType(dst));
	}
	return ret;
}


/*
* checks if movement causes any simple
* errors.
* input: source and destination points, current player 
* tag.
* output: error code.
*/
int board::subMovePiece(const Point& src, const Point& dst, const bool player) const
{

	if (src == dst)
	{
		return dst_is_src;
	}
	else if (!isInboard(dst))
	{
		return invalid_index;
	}
	else if (!playerInPos(src, player))
	{
		return player_not_src;
	}
	else if (playerInPos(dst, player))
	{
		return player_in_dst;
	}
	else if (!(*this)[src]->isValidMove(src, dst, *this))
	{
		return invalid_movment;
	}
	return 0;
}


void board::movePiece(const Point& src, const Point& dst)
{
	_board[dst.getY()][dst.getX()] = _board[src.getY()][src.getX()];
	_board[src.getY()][src.getX()] = nullptr;
}


/*
* convertes string of point into point object
* input: sting value
* output: point object
*/
Point board::convertPositionType(const std::string& pos)
{
	return Point(pos[0] - 'a', 8 - (pos[1] - '0'));
}


/*
* convertes point object into string 
* input: point object
* output: sting value
*/
std::string board::convertPositionType(const Point& pos)
{
	return std::string() + (char)(pos.getX() + 'a') + (char)(-pos.getY() + 8 + '0');
}


/*
* searchs for the location of a piece.
* input: targer piece
* output: the point where the piece was found.
*/
Point board::find(const char target) const
{
	for (int i = 0; i < SIZE; i++)
	{
		for (int j = 0; j < SIZE; j++)
		{
			if ((*this)[Point(j, i)]) {
				if ((*this)[Point(j, i)]->getType() == target) {
					return Point(j, i);
				}
			}
		}
	}
	throw("Not found");
}


Piece* board::operator[](const Point& index) const
{
	return _board[index.getY()][index.getX()];
}


Piece* board::operator[](const int index) const
{
	return _board[0][index];
}


/*
* convert board to string.
* input: none
* output: board as string.
*/
std::string board::convertToString() const
{
	char boardStr[SIZE * SIZE + 1] = { 0 };
	for (int i = 0; i < SIZE * SIZE; i++)
	{
		boardStr[i] = ((*this)[i]!=nullptr) ? (*this)[i]->getType() : '#';
	}
	return  std::string(boardStr);
}
