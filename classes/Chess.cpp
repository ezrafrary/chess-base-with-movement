#include "Chess.h"
#include <limits>
#include <cmath>

Chess::Chess()
{
    _grid = new Grid(8, 8);
    _whiteKingMoved = false;
    _blackKingMoved = false;
    _whiteRookAMoved = false;
    _whiteRookHMoved = false;
    _blackRookAMoved = false;
    _blackRookHMoved = false;
    _enPassantColumn = -1;
    _enPassantRow = -1;
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
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;
    
    // Check if player is in check and this piece must respond
    if (isKingInCheck(getCurrentPlayer())) {
        ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
        int srcX = srcSquare->getColumn();
        int srcY = srcSquare->getRow();
        
        // Must have at least one legal move to be movable
        bool hasLegalMove = false;
        _grid->forEachSquare([&](ChessSquare* dstSquare, int dstX, int dstY) {
            if (!hasLegalMove && canBitMoveFromTo(bit, src, *dstSquare)) {
                hasLegalMove = true;
            }
        });
        return hasLegalMove;
    }
    
    return true;
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
    
    Bit* dstBit = dstSquare->bit();
    if (dstBit && dstBit->getOwner() == bit.getOwner()) return false;
    
    ChessPiece pieceType = getPieceType(&bit);
    bool isValidBasicMove = false;
    
    switch (pieceType) {
        case Pawn:
            isValidBasicMove = isValidPawnMove(srcX, srcY, dstX, dstY, bit.getOwner());
            break;
        case Knight:
            isValidBasicMove = isValidKnightMove(srcX, srcY, dstX, dstY);
            break;
        case Bishop:
            isValidBasicMove = isValidBishopMove(srcX, srcY, dstX, dstY);
            break;
        case Rook:
            isValidBasicMove = isValidRookMove(srcX, srcY, dstX, dstY);
            break;
        case Queen:
            isValidBasicMove = isValidQueenMove(srcX, srcY, dstX, dstY);
            break;
        case King:
            isValidBasicMove = isValidKingMove(srcX, srcY, dstX, dstY);
            // Check for castling
            if (!isValidBasicMove && canCastle(srcX, srcY, dstX, dstY, bit.getOwner())) {
                isValidBasicMove = true;
            }
            break;
        default:
            return false;
    }
    
    if (!isValidBasicMove) return false;
    
    // Check if move would leave king in check
    if (wouldMoveLeaveKingInCheck(srcX, srcY, dstX, dstY, bit.getOwner())) {
        return false;
    }
    
    return true;
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);
    
    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();
    
    ChessPiece pieceType = getPieceType(&bit);
    Player* player = bit.getOwner();
    
    // Reset en passant
    int oldEnPassantCol = _enPassantColumn;
    int oldEnPassantRow = _enPassantRow;
    _enPassantColumn = -1;
    _enPassantRow = -1;
    
    // Handle special moves BEFORE updating bitboards
    if (pieceType == King) {
        // Check for castling
        if (abs(dstX - srcX) == 2) {
            performCastling(srcX, srcY, dstX, dstY);
        }
        
        if (player->playerNumber() == 0) {
            _whiteKingMoved = true;
        } else {
            _blackKingMoved = true;
        }
    }
    
    if (pieceType == Rook) {
        if (player->playerNumber() == 0) {
            if (srcX == 0 && srcY == 0) _whiteRookAMoved = true;
            if (srcX == 7 && srcY == 0) _whiteRookHMoved = true;
        } else {
            if (srcX == 0 && srcY == 7) _blackRookAMoved = true;
            if (srcX == 7 && srcY == 7) _blackRookHMoved = true;
        }
    }
    
    if (pieceType == Pawn) {
        // Check for two-square advance (enables en passant)
        if (abs(dstY - srcY) == 2) {
            _enPassantColumn = dstX;
            _enPassantRow = (srcY + dstY) / 2;
        }
        
        // Check for en passant capture
        if (dstX != srcX && oldEnPassantCol != -1 && dstX == oldEnPassantCol && dstY == oldEnPassantRow) {
            int captureY = (player->playerNumber() == 0) ? dstY - 1 : dstY + 1;
            ChessSquare* captureSquare = _grid->getSquare(dstX, captureY);
            if (captureSquare && captureSquare->bit()) {
                captureSquare->destroyBit();
            }
        }
        
        // Check for promotion
        if ((player->playerNumber() == 0 && dstY == 7) || (player->playerNumber() == 1 && dstY == 0)) {
            // The bit is already at the destination, we need to get it from there
            Bit* pawnAtDest = dstSquare->bit();
            if (pawnAtDest) {
                handlePawnPromotion(pawnAtDest, dstX, dstY);
            }
        }
    }
    
    updateBitboards();
    endTurn();
}

void Chess::handlePawnPromotion(Bit* pawn, int x, int y)
{
    if (!pawn) return;
    
    Player* player = pawn->getOwner();
    ChessSquare* square = _grid->getSquare(x, y);
    
    if (!square) return;
    
    // Destroy the pawn
    square->destroyBit();
    
    // Create a queen
    Bit* queen = PieceForPlayer(player->playerNumber(), Queen);
    int gameTag = Queen + (player->playerNumber() == 1 ? 128 : 0);
    queen->setGameTag(gameTag);
    queen->setPosition(square->getPosition());
    square->setBit(queen);
}

bool Chess::canCastle(int srcX, int srcY, int dstX, int dstY, Player* player) const
{
    ChessPiece pieceType = getPieceType(_grid->getSquare(srcX, srcY)->bit());
    if (pieceType != King) return false;
    
    // Must move exactly 2 squares horizontally
    if (abs(dstX - srcX) != 2 || dstY != srcY) return false;
    
    bool isWhite = (player->playerNumber() == 0);
    int baseRank = isWhite ? 0 : 7;
    
    if (srcY != baseRank || dstY != baseRank) return false;
    if (srcX != 4) return false;
    
    // Check if king has moved
    if (isWhite && _whiteKingMoved) return false;
    if (!isWhite && _blackKingMoved) return false;
    
    // Check if king is in check
    if (isKingInCheck(player)) return false;
    
    bool isKingSide = (dstX == 6);
    
    // Check if rook has moved
    if (isWhite) {
        if (isKingSide && _whiteRookHMoved) return false;
        if (!isKingSide && _whiteRookAMoved) return false;
    } else {
        if (isKingSide && _blackRookHMoved) return false;
        if (!isKingSide && _blackRookAMoved) return false;
    }
    
    // Check if rook is present
    int rookX = isKingSide ? 7 : 0;
    ChessSquare* rookSquare = _grid->getSquare(rookX, baseRank);
    if (!rookSquare->bit() || getPieceType(rookSquare->bit()) != Rook) return false;
    
    // Check if path is clear
    int direction = isKingSide ? 1 : -1;
    int endX = isKingSide ? 6 : 2;
    
    for (int x = srcX + direction; x != rookX; x += direction) {
        if (_grid->getSquare(x, baseRank)->bit()) return false;
    }
    
    // Check if king passes through or ends in check
    for (int x = srcX; x != endX + direction; x += direction) {
        if (isSquareUnderAttack(x, baseRank, player->playerNumber() == 0 ? getPlayerAt(1) : getPlayerAt(0))) {
            return false;
        }
    }
    
    return true;
}

void Chess::performCastling(int srcX, int srcY, int dstX, int dstY)
{
    bool isKingSide = (dstX == 6);
    int rookSrcX = isKingSide ? 7 : 0;
    int rookDstX = isKingSide ? 5 : 3;
    
    ChessSquare* rookSrc = _grid->getSquare(rookSrcX, srcY);
    ChessSquare* rookDst = _grid->getSquare(rookDstX, dstY);
    
    if (rookSrc->bit()) {
        Bit* rook = rookSrc->bit();
        rookSrc->setBit(nullptr);
        rook->setPosition(rookDst->getPosition());
        rook->moveTo(rookDst->getPosition());
        rookDst->setBit(rook);
    }
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    
    _whiteKingMoved = false;
    _blackKingMoved = false;
    _whiteRookAMoved = false;
    _whiteRookHMoved = false;
    _blackRookAMoved = false;
    _blackRookHMoved = false;
    _enPassantColumn = -1;
    _enPassantRow = -1;
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
    Player* currentPlayer = getCurrentPlayer();
    
    if (isKingInCheck(currentPlayer)) {
        if (!hasLegalMoves(currentPlayer)) {
            // Checkmate!
            return currentPlayer->playerNumber() == 0 ? getPlayerAt(1) : getPlayerAt(0);
        }
    }
    
    return nullptr;
}

bool Chess::checkForDraw()
{
    Player* currentPlayer = getCurrentPlayer();
    
    // Stalemate: not in check but no legal moves
    if (!isKingInCheck(currentPlayer) && !hasLegalMoves(currentPlayer)) {
        return true;
    }
    
    return false;
}

std::pair<int, int> Chess::findKing(Player* player) const
{
    std::pair<int, int> kingPos = {-1, -1};
    
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* bit = square->bit();
        if (bit && bit->getOwner() == player && getPieceType(bit) == King) {
            kingPos = {x, y};
        }
    });
    
    return kingPos;
}

bool Chess::isKingInCheck(Player* player) const
{
    auto [kingX, kingY] = findKing(player);
    if (kingX == -1) return false;
    
    Player* opponent = player->playerNumber() == 0 ? getPlayerAt(1) : getPlayerAt(0);
    return isSquareUnderAttack(kingX, kingY, opponent);
}

bool Chess::isSquareUnderAttack(int x, int y, Player* byPlayer) const
{
    // Check all opponent pieces to see if they can attack this square
    bool underAttack = false;
    
    _grid->forEachSquare([&](ChessSquare* square, int srcX, int srcY) {
        if (underAttack) return;
        
        Bit* bit = square->bit();
        if (!bit || bit->getOwner() != byPlayer) return;
        
        ChessPiece pieceType = getPieceType(bit);
        
        switch (pieceType) {
            case Pawn: {
                int direction = (byPlayer->playerNumber() == 0) ? 1 : -1;
                if (y == srcY + direction && (x == srcX - 1 || x == srcX + 1)) {
                    underAttack = true;
                }
                break;
            }
            case Knight:
                if (isValidKnightMove(srcX, srcY, x, y)) {
                    underAttack = true;
                }
                break;
            case Bishop:
                if (isValidBishopMove(srcX, srcY, x, y)) {
                    underAttack = true;
                }
                break;
            case Rook:
                if (isValidRookMove(srcX, srcY, x, y)) {
                    underAttack = true;
                }
                break;
            case Queen:
                if (isValidQueenMove(srcX, srcY, x, y)) {
                    underAttack = true;
                }
                break;
            case King:
                if (abs(x - srcX) <= 1 && abs(y - srcY) <= 1 && !(x == srcX && y == srcY)) {
                    underAttack = true;
                }
                break;
        }
    });
    
    return underAttack;
}

bool Chess::wouldMoveLeaveKingInCheck(int srcX, int srcY, int dstX, int dstY, Player* player) const
{
    // We need to actually simulate the move by temporarily modifying the board
    // Save the current state
    ChessSquare* srcSquare = _grid->getSquare(srcX, srcY);
    ChessSquare* dstSquare = _grid->getSquare(dstX, dstY);
    
    if (!srcSquare || !dstSquare) return true;
    
    Bit* movingBit = srcSquare->bit();
    Bit* capturedBit = dstSquare->bit();
    
    if (!movingBit) return true;
    
    // Get mutable pointers
    Chess* mutableThis = const_cast<Chess*>(this);
    BitHolder* mutableSrc = const_cast<BitHolder*>(static_cast<const BitHolder*>(srcSquare));
    BitHolder* mutableDst = const_cast<BitHolder*>(static_cast<const BitHolder*>(dstSquare));
    
    // Temporarily make the move by directly manipulating the board
    // We'll use a careful approach to avoid calling destructors
    
    // Save state
    Bit* srcBitBackup = srcSquare->bit();
    Bit* dstBitBackup = dstSquare->bit();
    
    // Make the move (access private member through casting)
    ChessSquare* mutableSrcSquare = const_cast<ChessSquare*>(srcSquare);
    ChessSquare* mutableDstSquare = const_cast<ChessSquare*>(dstSquare);
    
    // Directly set the internal _bit pointers without triggering setBit's delete
    struct BitHolderAccess {
        void* vtable;
        Entity* parent;
        ImVec2 location;
        ImVec2 size;
        float rotation;
        float scale;
        ImVec4 color;
        int localZOrder;
        void* texture;
        bool highlighted;
        Bit* _bit;
        int _gameTag;
        int _entityType;
    };
    
    BitHolderAccess* srcAccess = reinterpret_cast<BitHolderAccess*>(mutableSrcSquare);
    BitHolderAccess* dstAccess = reinterpret_cast<BitHolderAccess*>(mutableDstSquare);
    
    // Temporarily move the piece
    Bit* tempBit = srcAccess->_bit;
    srcAccess->_bit = nullptr;
    dstAccess->_bit = tempBit;
    
    // Now check if king is in check
    bool inCheck = mutableThis->isKingInCheck(player);
    
    // Restore the original state
    srcAccess->_bit = srcBitBackup;
    dstAccess->_bit = dstBitBackup;
    
    return inCheck;
}

bool Chess::hasLegalMoves(Player* player) const
{
    bool hasMove = false;
    
    _grid->forEachSquare([&](ChessSquare* srcSquare, int srcX, int srcY) {
        if (hasMove) return;
        
        Bit* bit = srcSquare->bit();
        if (!bit || bit->getOwner() != player) return;
        
        ChessPiece pieceType = getPieceType(bit);
        
        _grid->forEachSquare([&](ChessSquare* dstSquare, int dstX_inner, int dstY_inner) {
            if (hasMove) return;
            
            // Check if same square
            if (srcX == dstX_inner && srcY == dstY_inner) return;
            
            // Check if destination has friendly piece
            Bit* dstBit = dstSquare->bit();
            if (dstBit && dstBit->getOwner() == player) return;
            
            // Check basic move validity
            bool validMove = false;
            switch (pieceType) {
                case Pawn:
                    validMove = isValidPawnMove(srcX, srcY, dstX_inner, dstY_inner, player);
                    break;
                case Knight:
                    validMove = isValidKnightMove(srcX, srcY, dstX_inner, dstY_inner);
                    break;
                case Bishop:
                    validMove = isValidBishopMove(srcX, srcY, dstX_inner, dstY_inner);
                    break;
                case Rook:
                    validMove = isValidRookMove(srcX, srcY, dstX_inner, dstY_inner);
                    break;
                case Queen:
                    validMove = isValidQueenMove(srcX, srcY, dstX_inner, dstY_inner);
                    break;
                case King:
                    validMove = isValidKingMove(srcX, srcY, dstX_inner, dstY_inner);
                    if (!validMove) {
                        validMove = canCastle(srcX, srcY, dstX_inner, dstY_inner, player);
                    }
                    break;
            }
            
            if (!validMove) return;
            
            // Check if move would leave king in check
            if (!wouldMoveLeaveKingInCheck(srcX, srcY, dstX_inner, dstY_inner, player)) {
                hasMove = true;
            }
        });
    });
    
    return hasMove;
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
            s += pieceNotation(x, y);
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

// Movement validation functions

bool Chess::isValidPawnMove(int srcX, int srcY, int dstX, int dstY, Player* player) const
{
    int direction = (player->playerNumber() == 0) ? 1 : -1;
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
        Bit* middleBit = _grid->getSquare(srcX, srcY + direction)->bit();
        if (!middleBit) {
            return true;
        }
    }
    
    // Capture diagonally
    if (abs(deltaX) == 1 && deltaY == direction) {
        if (dstBit && dstBit->getOwner() != player) {
            return true;
        }
        
        // En passant
        if (!dstBit && dstX == _enPassantColumn && dstY == _enPassantRow) {
            return true;
        }
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

bool Chess::isValidBishopMove(int srcX, int srcY, int dstX, int dstY) const
{
    int deltaX = abs(dstX - srcX);
    int deltaY = abs(dstY - srcY);
    
    if (deltaX != deltaY || deltaX == 0) return false;
    
    return isPathClear(srcX, srcY, dstX, dstY);
}

bool Chess::isValidRookMove(int srcX, int srcY, int dstX, int dstY) const
{
    if (srcX != dstX && srcY != dstY) return false;
    
    return isPathClear(srcX, srcY, dstX, dstY);
}

bool Chess::isValidQueenMove(int srcX, int srcY, int dstX, int dstY) const
{
    return isValidBishopMove(srcX, srcY, dstX, dstY) || isValidRookMove(srcX, srcY, dstX, dstY);
}

bool Chess::isValidKingMove(int srcX, int srcY, int dstX, int dstY) const
{
    int srcSquare = srcY * 8 + srcX;
    int dstSquare = dstY * 8 + dstX;
    
    BitboardElement moves = getKingMoves(srcSquare);
    uint64_t dstBit = 1ULL << dstSquare;
    
    return (moves.getData() & dstBit) != 0;
}

bool Chess::isPathClear(int srcX, int srcY, int dstX, int dstY) const
{
    int deltaX = (dstX > srcX) ? 1 : (dstX < srcX) ? -1 : 0;
    int deltaY = (dstY > srcY) ? 1 : (dstY < srcY) ? -1 : 0;
    
    int x = srcX + deltaX;
    int y = srcY + deltaY;
    
    while (x != dstX || y != dstY) {
        if (_grid->getSquare(x, y)->bit()) {
            return false;
        }
        x += deltaX;
        y += deltaY;
    }
    
    return true;
}

// Bitboard helper methods

void Chess::initializeBitboards()
{
    // Initialize knight move table
    for (int square = 0; square < 64; square++) {
        int rank = square / 8;
        int file = square % 8;
        uint64_t moves = 0;
        
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