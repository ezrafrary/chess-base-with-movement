#pragma once

#include "Game.h"
#include "Grid.h"
#include "GameState.h"
#include <vector>
 
constexpr int pieceSize = 80;

class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;
    void endTurn() override;

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
    // Helper functions
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    char pieceNotation(int x, int y) const;
    ChessPiece getPieceType(const Bit* bit) const;
    int squareToIndex(int x, int y) const { return y * 8 + x; }
    void indexToSquare(int index, int& x, int& y) const { x = index % 8; y = index / 8; }
    
    // Sync between GameState and visual representation
    void syncGameStateToVisual();
    void syncVisualToGameState();
    void updateVisualFromGameState();
    
    // Move validation using GameState
    bool isValidMove(int fromX, int fromY, int toX, int toY);
    BitMove findMoveInList(int fromX, int fromY, int toX, int toY, const std::vector<BitMove>& moves) const;
    
    // Apply move to both GameState and visual
    void applyMove(const BitMove& move);
    void handleSpecialMoves(const BitMove& move);

    Grid* _grid;
    GameState _gameState;
    std::vector<BitMove> _currentLegalMoves;
};