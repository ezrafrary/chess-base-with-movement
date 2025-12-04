#pragma once

#include "Game.h"
#include "Grid.h"
#include "Bitboard.h"
#include <vector>

constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    void stopGame() override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;
    
    // Move validation for each piece type
    bool isValidPawnMove(int srcX, int srcY, int dstX, int dstY, Player* player) const;
    bool isValidKnightMove(int srcX, int srcY, int dstX, int dstY) const;
    bool isValidBishopMove(int srcX, int srcY, int dstX, int dstY) const;
    bool isValidRookMove(int srcX, int srcY, int dstX, int dstY) const;
    bool isValidQueenMove(int srcX, int srcY, int dstX, int dstY) const;
    bool isValidKingMove(int srcX, int srcY, int dstX, int dstY) const;
    
    // Path checking
    bool isPathClear(int srcX, int srcY, int dstX, int dstY) const;
    
    // Check detection
    bool isSquareUnderAttack(int x, int y, Player* byPlayer) const;
    bool isKingInCheck(Player* player) const;
    bool wouldMoveLeaveKingInCheck(int srcX, int srcY, int dstX, int dstY, Player* player) const;
    bool hasLegalMoves(Player* player) const;
    std::pair<int, int> findKing(Player* player) const;
    
    // Special moves
    bool canCastle(int srcX, int srcY, int dstX, int dstY, Player* player) const;
    void performCastling(int srcX, int srcY, int dstX, int dstY);
    void handlePawnPromotion(Bit* pawn, int x, int y);
    
    // Bitboard operations
    void initializeBitboards();
    void updateBitboards();
    BitboardElement getKnightMoves(int square) const;
    BitboardElement getKingMoves(int square) const;
    uint64_t squareToBit(int x, int y) const;
    
    ChessPiece getPieceType(const Bit* bit) const;

    Grid* _grid;
    
    // Game state
    bool _whiteKingMoved;
    bool _blackKingMoved;
    bool _whiteRookAMoved;
    bool _whiteRookHMoved;
    bool _blackRookAMoved;
    bool _blackRookHMoved;
    int _enPassantColumn;
    int _enPassantRow;
    
    // Bitboards
    BitboardElement _whitePieces;
    BitboardElement _blackPieces;
    BitboardElement _allPieces;
    
    BitboardElement _knightMoves[64];
    BitboardElement _kingMoves[64];
};