#pragma once

#include "Game.h"
#include "Grid.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

constexpr int pieceSize = 80;

enum ChessPiece {
    NoPiece = 0,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

struct ChessMove {
    int fromX, fromY, toX, toY;
    ChessSquare* fromSquare;
    ChessSquare* toSquare;
    Bit* piece;
    
    ChessMove() : fromX(0), fromY(0), toX(0), toY(0), fromSquare(nullptr), toSquare(nullptr), piece(nullptr) {}
    ChessMove(int fx, int fy, int tx, int ty, ChessSquare* fs, ChessSquare* ts, Bit* p)
        : fromX(fx), fromY(fy), toX(tx), toY(ty), fromSquare(fs), toSquare(ts), piece(p) {}
};

class Chess : public Game {
public:
    Chess();
    ~Chess();

    void setUpBoard() override;
    void stopGame() override;
    void endTurn() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &start) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &start, BitHolder &end) override;
    bool actionForEmptyHolder(BitHolder &holder) override;
    void bitMovedFromTo(Bit &bit, BitHolder &start, BitHolder &end) override;

    Player* checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return m_grid; }

    void makeRandomMoveForCurrentPlayer() { 
        AIMove(getCurrentPlayer()->playerNumber()); 
    }

private:
    Grid* m_grid;

    bool m_castlingRights[4] = {true, true, true, true};
    bool m_whiteKingMoved = false;
    bool m_blackKingMoved = false;
    bool m_whiteKingRookMoved = false;
    bool m_whiteQueenRookMoved = false;
    bool m_blackKingRookMoved = false;
    bool m_blackQueenRookMoved = false;

    int m_enPassantC = -1;
    int m_enPassantR = -1;
    int m_enPassantR2 = -1;

    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    char pieceNotation(int x, int y) const;

    void FENToBoard(const std::string& fen);

    std::vector<ChessMove> generateAllMoves(int playerNumber);
    std::vector<ChessMove> generatePawnMoves(int x, int y, Bit* piece);
    std::vector<ChessMove> generateKnightMoves(int x, int y, Bit* piece);
    std::vector<ChessMove> generateRookMoves(int x, int y, Bit* piece);
    std::vector<ChessMove> generateBishopMoves(int x, int y, Bit* piece);
    std::vector<ChessMove> generateQueenMoves(int x, int y, Bit* piece);
    std::vector<ChessMove> generateKingMoves(int x, int y, Bit* piece);

    bool canBitMoveFromToOld(Bit &bit, BitHolder &start, BitHolder &end);

    bool isSquareUnderAttack(int x, int y, bool byWhite);
    bool checkAfterMove(int fromX, int fromY, int toX, int toY, int playerNumber);
    bool isInCheck(int playerNumber);

    int evaluateBoard(int playerNumber);
    int negamax(int depth, int alpha, int beta, int playerNumber);
    ChessMove findBestMove(int playerNumber);
    void makeMove(const ChessMove& move);
    void unmakeMove(const ChessMove& move, Bit* capturedPiece, int oldEnPassantCol, int oldEnPassantRow, int oldEnPassantTarget);
    void AIMove(int playerNumber);
};
