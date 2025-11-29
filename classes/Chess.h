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
    
    bool isValidPawnMove(int srcX, int srcY, int dstX, int dstY, Player* player) const;
    bool isValidKnightMove(int srcX, int srcY, int dstX, int dstY) const;
    bool isValidKingMove(int srcX, int srcY, int dstX, int dstY) const;
    
    void initializeBitboards();
    void updateBitboards();
    BitboardElement getKnightMoves(int square) const;
    BitboardElement getKingMoves(int square) const;
    uint64_t squareToBit(int x, int y) const;
    
    ChessPiece getPieceType(const Bit* bit) const;

    Grid* _grid;
    
    BitboardElement _whitePieces;
    BitboardElement _blackPieces;
    BitboardElement _allPieces;
    
    BitboardElement _knightMoves[64];
    BitboardElement _kingMoves[64];
};