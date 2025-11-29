#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
    initializeBitboards();
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
    
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    
    size_t fenLength = fen.length();
    
    size_t boardPartEnd = fenLength;
    for (size_t i = 0; i < fenLength; i++) {
        if (fen[i] == ' ') {
            boardPartEnd = i;
            break;
        }
    }
    
    int rank = 7; 
    int file = 0;
    
    for (size_t i = 0; i < boardPartEnd; i++) {
        char c = fen[i];
        
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            ChessPiece pieceType = NoPiece;
            
            bool isLowerCase = (c >= 'a' && c <= 'z');
            int playerNumber = isLowerCase ? 1 : 0;
            
            char pieceChar = isLowerCase ? c : (c + 32);
            
            switch (pieceChar) {
                case 'p': pieceType = Pawn; break;
                case 'n': pieceType = Knight; break;
                case 'b': pieceType = Bishop; break;
                case 'r': pieceType = Rook; break;
                case 'q': pieceType = Queen; break;
                case 'k': pieceType = King; break;
            }
            
            if (pieceType != NoPiece && file < 8 && rank >= 0) {
                Bit* piece = PieceForPlayer(playerNumber, pieceType);
                int gameTag = pieceType + (playerNumber == 1 ? 128 : 0);
                piece->setGameTag(gameTag);
                
                ChessSquare* square = _grid->getSquare(file, rank);
                piece->setPosition(square->getPosition());
                square->setBit(piece);
            }
            
            file++;
        }
    }
    
    updateBitboards();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);
    
    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();
    
    if (srcX == dstX && srcY == dstY) return false;
    
    // Can't capture your own piece
    Bit* dstBit = dstSquare->bit();
    if (dstBit && dstBit->getOwner() == bit.getOwner()) return false;
    
    // Get piece type
    ChessPiece pieceType = getPieceType(&bit);
    
    switch (pieceType) {
        case Pawn:
            return isValidPawnMove(srcX, srcY, dstX, dstY, bit.getOwner());
        case Knight:
            return isValidKnightMove(srcX, srcY, dstX, dstY);
        case King:
            return isValidKingMove(srcX, srcY, dstX, dstY);
        default:
            return false; 
    }
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation( x, y );
        }
    );
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

bool Chess::isValidPawnMove(int srcX, int srcY, int dstX, int dstY, Player* player) const
{
    int direction = (player->playerNumber() == 0) ? 1 : -1; // White moves up, Black moves down
    int startRank = (player->playerNumber() == 0) ? 1 : 6;
    
    int deltaX = dstX - srcX;
    int deltaY = dstY - srcY;
    
    Bit* dstBit = _grid->getSquare(dstX, dstY)->bit();
    
    // Move forward one square
    if (deltaX == 0 && deltaY == direction && !dstBit) {
        return true;
    }
    
    // Move forward two squares from starting position
    if (deltaX == 0 && deltaY == 2 * direction && srcY == startRank && !dstBit) {
        // Check if square in between is empty
        Bit* middleBit = _grid->getSquare(srcX, srcY + direction)->bit();
        if (!middleBit) {
            return true;
        }
    }
    
    // Capture diagonally
    if (abs(deltaX) == 1 && deltaY == direction && dstBit && dstBit->getOwner() != player) {
        return true;
    }
    
    return false;
}

bool Chess::isValidKnightMove(int srcX, int srcY, int dstX, int dstY) const
{
    int srcSquare = srcY * 8 + srcX;
    int dstSquare = dstY * 8 + dstX;
    
    BitboardElement moves = getKnightMoves(srcSquare);
    uint64_t dstBit = 1ULL << dstSquare;
    
    return (moves.getData() & dstBit) != 0;
}

bool Chess::isValidKingMove(int srcX, int srcY, int dstX, int dstY) const
{
    int srcSquare = srcY * 8 + srcX;
    int dstSquare = dstY * 8 + dstX;
    
    BitboardElement moves = getKingMoves(srcSquare);
    uint64_t dstBit = 1ULL << dstSquare;
    
    return (moves.getData() & dstBit) != 0;
}

// Bitboard helper methods

void Chess::initializeBitboards()
{
    // Initialize knight move table
    for (int square = 0; square < 64; square++) {
        int rank = square / 8;
        int file = square % 8;
        uint64_t moves = 0;
        
        // All 8 possible knight moves
        int knightMoves[8][2] = {
            {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
            {1, -2}, {1, 2}, {2, -1}, {2, 1}
        };
        
        for (int i = 0; i < 8; i++) {
            int newFile = file + knightMoves[i][0];
            int newRank = rank + knightMoves[i][1];
            
            if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
                int targetSquare = newRank * 8 + newFile;
                moves |= (1ULL << targetSquare);
            }
        }
        
        _knightMoves[square] = BitboardElement(moves);
    }
    
    // Initialize king move table
    for (int square = 0; square < 64; square++) {
        int rank = square / 8;
        int file = square % 8;
        uint64_t moves = 0;
        
        // All 8 possible king moves
        int kingMoves[8][2] = {
            {-1, -1}, {-1, 0}, {-1, 1}, {0, -1},
            {0, 1}, {1, -1}, {1, 0}, {1, 1}
        };
        
        for (int i = 0; i < 8; i++) {
            int newFile = file + kingMoves[i][0];
            int newRank = rank + kingMoves[i][1];
            
            if (newFile >= 0 && newFile < 8 && newRank >= 0 && newRank < 8) {
                int targetSquare = newRank * 8 + newFile;
                moves |= (1ULL << targetSquare);
            }
        }
        
        _kingMoves[square] = BitboardElement(moves);
    }
}

void Chess::updateBitboards()
{
    uint64_t white = 0;
    uint64_t black = 0;
    
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (bit) {
            int squareIndex = y * 8 + x;
            uint64_t squareBit = 1ULL << squareIndex;
            
            if (bit->gameTag() < 128) {
                white |= squareBit;
            } else {
                black |= squareBit;
            }
        }
    });
    
    _whitePieces.setData(white);
    _blackPieces.setData(black);
    _allPieces.setData(white | black);
}

BitboardElement Chess::getKnightMoves(int square) const
{
    return _knightMoves[square];
}

BitboardElement Chess::getKingMoves(int square) const
{
    return _kingMoves[square];
}

uint64_t Chess::squareToBit(int x, int y) const
{
    return 1ULL << (y * 8 + x);
}

ChessPiece Chess::getPieceType(const Bit* bit) const
{
    if (!bit) return NoPiece;
    int gameTag = bit->gameTag();
    return static_cast<ChessPiece>(gameTag & 0x7F);
}