#include "Chess.h"
#include <limits>
#include <cmath>
#include <cctype>
#include <iostream>
#include <algorithm>
#include <cstring>

Chess::Chess() {
    m_grid = new Grid(8, 8);
}

Chess::~Chess() {
    delete m_grid;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    m_grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENToBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    
    startGame();
}

void Chess::stopGame() {
    m_grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

void Chess::endTurn() {
    Game::endTurn();
}


Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece) {
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };
    Bit* bit = new Bit();
    const char* pieceName = pieces[piece - 1];
    std::string spritePath;
    if (playerNumber == 0) {
        spritePath = "w_";
    } else {
        spritePath = "b_";
    }
    spritePath += std::string(pieceName);
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);
    return bit;
}

char Chess::pieceNotation(int x, int y) const {
    const char *wpieces = "0PNBRQK";
    const char *bpieces = "0pnbrqk";
    Bit *bit = m_grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        if (bit->gameTag() < 128) {
            notation = wpieces[bit->gameTag()];
        } else {
            notation = bpieces[bit->gameTag() - 128];
        }
    }
    return notation;
}

void Chess::FENToBoard(const std::string& fen) {
    m_grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    std::string placement = fen.substr(0, fen.find(' '));
    
    int rank = 7;  
    int file = 0;  
    
    for (char c : placement) {
        if (c == '/') {
            rank--;
            file = 0;
            continue;
        }
        
        if (std::isdigit(c)) {
            file += (c - '0');
            continue;
        }
        
        // Convert character to piece type
        char lower = std::tolower(c);
        ChessPiece pieceType = NoPiece;
        switch (lower) {
            case 'p': pieceType = Pawn; break;
            case 'n': pieceType = Knight; break;
            case 'b': pieceType = Bishop; break;
            case 'r': pieceType = Rook; break;
            case 'q': pieceType = Queen; break;
            case 'k': pieceType = King; break;
        }
        
        if (pieceType == NoPiece) continue;
        
        bool isWhite = std::isupper(c);
        int playerNum = isWhite ? 0 : 1;
        int tag = (isWhite ? 0 : 128) + static_cast<int>(pieceType);
        
        ChessSquare* square = m_grid->getSquare(file, rank);
        if (square) {
            Bit* bit = PieceForPlayer(playerNum, pieceType);
            bit->setGameTag(tag);
            bit->setPosition(square->getPosition());
            square->setBit(bit);
        }
        
        file++;
    }
}

std::string Chess::initialStateString() {
    return stateString();
}

std::string Chess::stateString() {
    std::string s;
    s.reserve(64);
    m_grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        s += pieceNotation(x, y);
    });
    return s;
}

void Chess::setStateString(const std::string &s) {
    m_grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

bool Chess::actionForEmptyHolder(BitHolder &holder) {
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &start) {
    int currentPlayer = getCurrentPlayer()->playerNumber();
    int pieceColor = bit.gameTag() & 128;
    if (currentPlayer == 0 && pieceColor == 0) return true;
    if (currentPlayer == 1 && pieceColor == 128) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &start, BitHolder &end) {
    if (!canBitMoveFromToOld(bit, start, end)) {
        return false;
    }
    ChessSquare* startSquare = dynamic_cast<ChessSquare*>(&start);
    ChessSquare* endSquare = dynamic_cast<ChessSquare*>(&end);
    if (!startSquare || !endSquare) return false;
    int playerNumber;
    if (bit.gameTag() & 128) {
        playerNumber = 1;
    } else {
        playerNumber = 0;
    }
    if (checkAfterMove(
            startSquare->getColumn(), startSquare->getRow(),
            endSquare->getColumn(), endSquare->getRow(), playerNumber)) {
    return false;
    }
    return true;
}

bool Chess::canBitMoveFromToOld(Bit &bit, BitHolder &start, BitHolder &end) {
    ChessSquare* startSquare = dynamic_cast<ChessSquare*>(&start);
    ChessSquare* endSquare = dynamic_cast<ChessSquare*>(&end);
    if (!startSquare || !endSquare) return false;
    
    int startX = startSquare->getColumn();
    int startY = startSquare->getRow();
    int endX = endSquare->getColumn();
    int endY = endSquare->getRow();
    int dx = endX - startX;
    int dy = endY - startY;
    int adx = abs(dx);
    int ady = abs(dy);
    
    int pieceType = bit.gameTag() & 0x7F;
    bool isWhite = (bit.gameTag() & 0x80) == 0;
    int direction = isWhite ? 1 : -1;
    
    if (end.bit() && end.bit()->getOwner() == bit.getOwner()) {
        return false;
    }
    
    switch (pieceType) {
        case Pawn: {
            int startRank = isWhite ? 1 : 6;
            int forwardOne = startY + direction;
            int forwardTwo = startY + 2 * direction;
            if (startX == endX && !end.bit()) {
                if (endY == forwardOne) return true;
                if (startY == startRank && endY == forwardTwo && 
                    !m_grid->getSquare(endX, forwardOne)->bit()) {
                    return true;
                }
            } else if (adx == 1 && endY == forwardOne) {
                if (end.bit() && end.bit()->getOwner() != bit.getOwner()) {
                    return true;
                }
                if (m_enPassantC != -1 && endX == m_enPassantC && endY == m_enPassantR2) {
                    return true;
                }
            }
            return false;
        }
        case Knight: {
            return (adx == 2 && ady == 1) || (adx == 1 && ady == 2);
        }
        case King: {
            if (adx <= 1 && ady <= 1) return true;
            if (adx == 2 && dy == 0) {
                bool kingSide = (dx > 0);
                int row = isWhite ? 0 : 7;
                int playerIndex = isWhite ? 0 : 2;
                int castlingIndex = playerIndex + (kingSide ? 0 : 1);
                if (!m_castlingRights[castlingIndex]) return false;
                int checkStartX = kingSide ? startX + 1 : 1;
                int checkEndX = kingSide ? endX : startX;
                for (int x = checkStartX; x < checkEndX; x++) {
                    ChessSquare* square = m_grid->getSquare(x, row);
                    if (!square || square->bit()) return false;
                }
                return true;
            }
            return false;
        }
        case Rook: {
            if (startX != endX && startY != endY) return false;
            int stepX = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
            int stepY = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;
            int x = startX + stepX;
            int y = startY + stepY;
            while (x != endX || y != endY) {
                if (m_grid->getSquare(x, y)->bit()) return false;
                x += stepX;
                y += stepY;
            }
            return true;
        }
        case Bishop: {
            if (adx != ady) return false;
            int stepX = (dx > 0) ? 1 : -1;
            int stepY = (dy > 0) ? 1 : -1;
            int x = startX + stepX;
            int y = startY + stepY;
            while (x != endX && y != endY) {
                if (m_grid->getSquare(x, y)->bit()) return false;
                x += stepX;
                y += stepY;
            }
            return true;
        }
        case Queen: {
            if (adx != 0 && ady != 0 && adx != ady) return false;
            int stepX = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
            int stepY = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;
            int x = startX + stepX;
            int y = startY + stepY;
            while (x != endX || y != endY) {
                if (m_grid->getSquare(x, y)->bit()) return false;
                x += stepX;
                y += stepY;
            }
            return true;
        }
        default:
            return false;
    }
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &start, BitHolder &end) {
    int pieceType = bit.gameTag() & 0x7F;
    bool isWhite = (bit.gameTag() & 0x80) == 0;
    if (pieceType == Pawn) {
        ChessSquare* endSquare = dynamic_cast<ChessSquare*>(&end);
        if (endSquare && m_enPassantC != -1 && 
            endSquare->getColumn() == m_enPassantC && 
            endSquare->getRow() == m_enPassantR2) {
            ChessSquare* capturedSquare = m_grid->getSquare(m_enPassantC, m_enPassantR);
            if (capturedSquare) {
                capturedSquare->destroyBit();
            }
        }
    }
    int nextEnPassantCol = -1;
    int nextEnPassantRow = -1;
    int nextEnPassantTargetRow = -1;
    if (pieceType == Pawn) {
        ChessSquare* startSquare = dynamic_cast<ChessSquare*>(&start);
        ChessSquare* endSquare = dynamic_cast<ChessSquare*>(&end);
        if (startSquare && endSquare && abs(startSquare->getRow() - endSquare->getRow()) == 2) {
            nextEnPassantCol = endSquare->getColumn();
            nextEnPassantRow = endSquare->getRow();
            nextEnPassantTargetRow = (startSquare->getRow() + endSquare->getRow()) / 2;
        }
    }
    m_enPassantC = nextEnPassantCol;
    m_enPassantR = nextEnPassantRow;
    m_enPassantR2 = nextEnPassantTargetRow;
    if (pieceType == King) {
        if (isWhite) {
            m_whiteKingMoved = true;
            m_castlingRights[0] = false;
            m_castlingRights[1] = false;
        } else {
            m_blackKingMoved = true;
            m_castlingRights[2] = false;
            m_castlingRights[3] = false;
        }
    } else if (pieceType == Rook) {
        ChessSquare* startSquare = dynamic_cast<ChessSquare*>(&start);
        if (startSquare) {
            int x = startSquare->getColumn();
            int y = startSquare->getRow();
            if (isWhite) {
                if (x == 0 && y == 0) m_whiteQueenRookMoved = true;
                else if (x == 7 && y == 0) m_whiteKingRookMoved = true;
            } else {
                if (x == 0 && y == 7) m_blackQueenRookMoved = true;
                else if (x == 7 && y == 7) m_blackKingRookMoved = true;
            }
            int expectedRow;
            if (isWhite) {
                expectedRow = 0;
            } else {
                expectedRow = 7;
            }
            if (y == expectedRow) {
                if (x == 0) {
                    if (isWhite) {
                        m_castlingRights[1] = false;
                    } else {
                        m_castlingRights[3] = false;
                    }
                } else if (x == 7) {
                    if (isWhite) {
                        m_castlingRights[0] = false;
                    } else {
                        m_castlingRights[2] = false;
                    }
                }
            }
        }
    }
    if (pieceType == Pawn) {
        ChessSquare* endSquare = dynamic_cast<ChessSquare*>(&end);
        if (endSquare) {
            int row = endSquare->getRow();
            if ((isWhite && row == 7) || (!isWhite && row == 0)) {
                int newTag;
                if (isWhite) {
                    newTag = 0;
                } else {
                    newTag = 128;
                }
                newTag += Queen;
                bit.setGameTag(newTag);
                std::string spritePath;
                if (isWhite) {
                    spritePath = "w_";
                } else {
                    spritePath = "b_";
                }
                spritePath += std::string("queen.png");
                bit.LoadTextureFromFile(spritePath.c_str());
                bit.setSize(pieceSize, pieceSize);
            }
        }
    }
    ChessSquare* startSquare = dynamic_cast<ChessSquare*>(&start);
    ChessSquare* endSquare = dynamic_cast<ChessSquare*>(&end);
    if (!startSquare || !endSquare) {
        endTurn();
        return;
    }
    if (pieceType == King && abs(startSquare->getColumn() - endSquare->getColumn()) == 2) {
        bool king = (endSquare->getColumn() > startSquare->getColumn());
        int row;
        if (isWhite) {
            row = 0;
        } else {
            row = 7;
        }
        int rookFromCol;
        if (king) {
            rookFromCol = 7;
        } else {
            rookFromCol = 0;
        }
        int rookToCol;
        if (king) {
            rookToCol = 5;
        } else {
            rookToCol = 3;
        }
        ChessSquare* rookSquare = m_grid->getSquare(rookFromCol, row);
        ChessSquare* newRookSquare = m_grid->getSquare(rookToCol, row);
        if (rookSquare && newRookSquare && rookSquare->bit()) {
            Bit* rook = rookSquare->bit();
            int rookType = rook->gameTag() & 0x7F;
            if (rookType == Rook) {
                rookSquare->releaseBit();
                newRookSquare->setBit(rook);
                rook->setPosition(newRookSquare->getPosition());
            }
        }
    }
    endTurn();
}

bool Chess::isSquareUnderAttack(int x, int y, bool byWhite) {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            ChessSquare* square = m_grid->getSquare(i, j);
            if (!square || !square->bit()) continue;
            bool pieceIsWhite = (square->bit()->gameTag() & 0x80) == 0;
            if (pieceIsWhite != byWhite) continue;
            int pieceType = square->bit()->gameTag() & 0x7F;
            if (pieceType == King) continue;
            ChessSquare* targetSquare = m_grid->getSquare(x, y);
            if (targetSquare && canBitMoveFromToOld(*square->bit(), *square, *targetSquare)) {
                return true;
            }
        }
    }
    return false;
}

bool Chess::checkAfterMove(int fromX, int fromY, int toX, int toY, int playerNumber) {
    ChessSquare* fromSquare = m_grid->getSquare(fromX, fromY);
    ChessSquare* toSquare = m_grid->getSquare(toX, toY);
    if (!fromSquare || !toSquare || !fromSquare->bit()) return true;

    Bit* movingPiece = fromSquare->releaseBit();
    Bit* capturedPiece = toSquare->releaseBit();
    toSquare->setBit(movingPiece);

    Bit* enPassantCapturedPiece = nullptr;
    ChessSquare* enPassantSquare = nullptr;
    bool isPawnMove = (movingPiece->gameTag() & 0x7F) == Pawn;
    bool isEnPassant = isPawnMove && m_enPassantC != -1 && 
                       toX == m_enPassantC && toY == m_enPassantR2;
    if (isEnPassant) {
        enPassantSquare = m_grid->getSquare(m_enPassantC, m_enPassantR);
        if (enPassantSquare) {
            enPassantCapturedPiece = enPassantSquare->releaseBit();
        }
    }
    int kingX = -1, kingY = -1;
    bool isWhite = (playerNumber == 0);
    m_grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        if (kingX != -1) return;
        Bit* piece = square->bit();
        if (piece) {
            int pieceType = piece->gameTag() & 0x7F;
            bool pieceIsWhite = (piece->gameTag() & 0x80) == 0;
            if (pieceType == King && pieceIsWhite == isWhite) {
                kingX = x;
                kingY = y;
            }
        }
    });
    bool inCheck = (kingX != -1) ? isSquareUnderAttack(kingX, kingY, !isWhite) : false;

    toSquare->releaseBit();
    fromSquare->setBit(movingPiece);
    toSquare->setBit(capturedPiece);
    if (enPassantSquare && enPassantCapturedPiece) {
        enPassantSquare->setBit(enPassantCapturedPiece);
    }

    return inCheck;
}

bool Chess::isInCheck(int playerNumber) {
    int kingX = -1, kingY = -1;
    bool kingFound = false;
    for (int y = 0; y < 8 && !kingFound; y++) {
        for (int x = 0; x < 8; x++) {
            ChessSquare* square = m_grid->getSquare(x, y);
            if (square && square->bit()) {
                int pieceType = square->bit()->gameTag() & 0x7F;
                bool pieceIsWhite = (square->bit()->gameTag() & 0x80) == 0;
                if (pieceType == King && pieceIsWhite == (playerNumber == 0)) {
                    kingX = x;
                    kingY = y;
                    kingFound = true;
                    break;
                }
            }
        }
    }
    if (kingFound) {
        return isSquareUnderAttack(kingX, kingY, playerNumber != 0);
    }
    return false;
}

Player* Chess::checkForWinner() {
    bool whiteKingExists = false;
    bool blackKingExists = false;
    m_grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* piece = square->bit();
        if (piece && (piece->gameTag() & 0x7F) == King) {
            bool isWhite = (piece->gameTag() & 0x80) == 0;
            if (isWhite) {
                whiteKingExists = true;
            } else {
                blackKingExists = true;
            }
        }
    });
    if (!whiteKingExists) return getPlayerAt(1);
    if (!blackKingExists) return getPlayerAt(0);
    int currentPlayerNum = getCurrentPlayer()->playerNumber();
    auto moves = generateAllMoves(currentPlayerNum);
    if (moves.empty()) {
        if (isInCheck(currentPlayerNum)) {
            int winnerPlayerNum;
            if (currentPlayerNum == 0) {
                winnerPlayerNum = 1;
        } else {
                winnerPlayerNum = 0;
            }
            return getPlayerAt(winnerPlayerNum);
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

bool Chess::checkForDraw() {
    int currentPlayerNum = getCurrentPlayer()->playerNumber();
    auto moves = generateAllMoves(currentPlayerNum);
    if (moves.empty() && !isInCheck(currentPlayerNum)) {
        return true;
    }
    return false;
}

std::vector<ChessMove> Chess::generateAllMoves(int playerNumber) {
    std::vector<ChessMove> allMoves;
    m_grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* piece = square->bit();
        if (!piece) return;
        int pieceColor = piece->gameTag() & 128;
        bool isWhitePiece = (pieceColor == 0);
        bool isPlayerWhite = (playerNumber == 0);
        if (isWhitePiece == isPlayerWhite) {
            int pieceType = piece->gameTag() & 0x7F;
            switch (pieceType) {
                case Pawn:
                    for (auto move : generatePawnMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Knight:
                    for (auto move : generateKnightMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case King:
                    for (auto move : generateKingMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Rook:
                    for (auto move : generateRookMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Bishop:
                    for (auto move : generateBishopMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                case Queen:
                    for (auto move : generateQueenMoves(x, y, piece)) {
                        allMoves.push_back(move);
                    }
                    break;
                default:
                break;
            }
        }
    });
    std::vector<ChessMove> validMoves;
    for (const auto& move : allMoves) {
        if (!checkAfterMove(move.fromX, move.fromY, move.toX, move.toY, playerNumber)) {
            validMoves.push_back(move);
        }
    }
    return validMoves;
}

std::vector<ChessMove> Chess::generatePawnMoves(int x, int y, Bit* piece) {
    std::vector<ChessMove> moves;
    bool isWhite = (piece->gameTag() & 128) == 0;
    int direction;
    if (isWhite) {
        direction = 1;
    } else {
        direction = -1;
    }
    int startRank;
    if (isWhite) {
        startRank = 1;
    } else {
        startRank = 6;
    }
    int newY = y + direction;
    if (newY >= 0 && newY < 8) {
        ChessSquare* targetSquare = m_grid->getSquare(x, newY);
        if (targetSquare && !targetSquare->bit()) {
            moves.push_back(ChessMove(x, y, x, newY, m_grid->getSquare(x, y), targetSquare, piece));
        }
    }
    if (y == startRank) {
        newY = y + 2 * direction;
        int middleY = y + direction;
        if (newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = m_grid->getSquare(x, newY);
            ChessSquare* middleSquare = m_grid->getSquare(x, middleY);
            if (targetSquare && middleSquare && !targetSquare->bit() && !middleSquare->bit()) {
                moves.push_back(ChessMove(x, y, x, newY, m_grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    for (int dx = -1; dx <= 1; dx += 2) {
        int newX = x + dx;
        newY = y + direction;
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = m_grid->getSquare(newX, newY);
            if (targetSquare && targetSquare->bit() && targetSquare->bit()->getOwner() != piece->getOwner()) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    if (m_enPassantC != -1) {
        if (abs(x - m_enPassantC) == 1) {
            int forwardOne = y + direction;
            if (forwardOne == m_enPassantR2) {
                ChessSquare* targetSquare = m_grid->getSquare(m_enPassantC, m_enPassantR2);
                if (targetSquare && !targetSquare->bit()) {
                    moves.push_back(ChessMove(x, y, m_enPassantC, m_enPassantR2, 
                                             m_grid->getSquare(x, y), targetSquare, piece));
                }
            }
        }
    }
    return moves;
}

std::vector<ChessMove> Chess::generateKnightMoves(int x, int y, Bit* piece) {
    std::vector<ChessMove> moves;
    int knightMoves[8][2] = {{2,1}, {2,-1}, {-2,1}, {-2,-1}, {1,2}, {1,-2}, {-1,2}, {-1,-2}};
    for (auto& move : knightMoves) {
        int newX = x + move[0];
        int newY = y + move[1];
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = m_grid->getSquare(newX, newY);
            if (targetSquare && (!targetSquare->bit() || targetSquare->bit()->getOwner() != piece->getOwner())) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    return moves;
}

std::vector<ChessMove> Chess::generateRookMoves(int x, int y, Bit* piece) {
    std::vector<ChessMove> moves;
    int directions[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (auto& dir : directions) {
        int dx = dir[0];
        int dy = dir[1];
        int newX = x + dx;
        int newY = y + dy;
        while (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = m_grid->getSquare(newX, newY);
            if (!targetSquare) break;
            Bit* targetPiece = targetSquare->bit();
            if (!targetPiece) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
            } else if (targetPiece->getOwner() != piece->getOwner()) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
                break;
            } else {
                break;
            }
            newX += dx;
            newY += dy;
        }
    }
    return moves;
}

std::vector<ChessMove> Chess::generateBishopMoves(int x, int y, Bit* piece) {
    std::vector<ChessMove> moves;
    int directions[4][2] = {{1, 1}, {-1, 1}, {1, -1}, {-1, -1}};
    for (auto& dir : directions) {
        int dx = dir[0];
        int dy = dir[1];
        int newX = x + dx;
        int newY = y + dy;
        while (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = m_grid->getSquare(newX, newY);
            if (!targetSquare) break;
            Bit* targetPiece = targetSquare->bit();
            if (!targetPiece) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
            } else if (targetPiece->getOwner() != piece->getOwner()) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
                break;
            } else {
                    break;
            }
            newX += dx;
            newY += dy;
        }
    }
    return moves;
}

std::vector<ChessMove> Chess::generateQueenMoves(int x, int y, Bit* piece) {
    std::vector<ChessMove> moves;
    auto rookMoves = generateRookMoves(x, y, piece);
    auto bishopMoves = generateBishopMoves(x, y, piece);
    moves.insert(moves.end(), rookMoves.begin(), rookMoves.end());
    moves.insert(moves.end(), bishopMoves.begin(), bishopMoves.end());
    return moves;
}

std::vector<ChessMove> Chess::generateKingMoves(int x, int y, Bit* piece) {
    std::vector<ChessMove> moves;
    int directions[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
    for (auto& dir : directions) {
        int newX = x + dir[0];
        int newY = y + dir[1];
        if (newX >= 0 && newX < 8 && newY >= 0 && newY < 8) {
            ChessSquare* targetSquare = m_grid->getSquare(newX, newY);
            if (targetSquare && (!targetSquare->bit() || targetSquare->bit()->getOwner() != piece->getOwner())) {
                moves.push_back(ChessMove(x, y, newX, newY, m_grid->getSquare(x, y), targetSquare, piece));
            }
        }
    }
    bool isWhite = (piece->gameTag() & 0x80) == 0;
    int row = isWhite ? 0 : 7;
    int playerIndex = isWhite ? 0 : 2;
    bool enemyCanAttack = !isWhite;
    ChessSquare* fromSquare = m_grid->getSquare(x, y);
    if (m_castlingRights[playerIndex] && y == row) {
        bool pathClear = true;
        for (int i = x + 1; i < 7; i++) {
            if (m_grid->getSquare(i, row)->bit()) {
                pathClear = false;
                break;
            }
        }
        if (pathClear && !isSquareUnderAttack(x, y, enemyCanAttack) && 
            !isSquareUnderAttack(x + 1, row, enemyCanAttack) && 
            !isSquareUnderAttack(x + 2, row, enemyCanAttack)) {
            moves.push_back(ChessMove(x, y, x + 2, row, fromSquare, m_grid->getSquare(x + 2, row), piece));
        }
    }
    if (m_castlingRights[playerIndex + 1] && y == row) {
        bool pathClear = true;
        for (int i = x - 1; i > 0; i--) {
            if (m_grid->getSquare(i, row)->bit()) {
                pathClear = false;
                break;
            }
        }
        if (pathClear && !isSquareUnderAttack(x, y, enemyCanAttack) && 
            !isSquareUnderAttack(x - 1, row, enemyCanAttack) && 
            !isSquareUnderAttack(x - 2, row, enemyCanAttack)) {
            moves.push_back(ChessMove(x, y, x - 2, row, fromSquare, m_grid->getSquare(x - 2, row), piece));
        }
    }
    return moves;
}

int Chess::evaluateBoard(int playerNumber) {
    static const int PIECE_VALUES[] = {0, 100, 310, 330, 500, 900, 20000};
    int score = 0;
    m_grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        Bit* piece = square->bit();
        if (!piece) return;
        int pieceType = piece->gameTag() & 0x7F;
        bool isWhite = (piece->gameTag() & 0x80) == 0;
        int pieceValue = PIECE_VALUES[pieceType];
        bool isPlayerPiece = (isWhite && playerNumber == 0) || (!isWhite && playerNumber == 1);
        score += isPlayerPiece ? pieceValue : -pieceValue;
    });
    return score;
}

void Chess::makeMove(const ChessMove& move) {
    if (move.toSquare->bit()) {
        move.toSquare->releaseBit();
    }
    move.fromSquare->releaseBit();
    move.toSquare->setBit(move.piece);
}

void Chess::unmakeMove(const ChessMove& move, Bit* capturedPiece, int oldEnPassantCol, int oldEnPassantRow, int oldEnPassantTarget) {
    move.toSquare->releaseBit();
    move.fromSquare->setBit(move.piece);
    move.toSquare->setBit(capturedPiece);
    m_enPassantC = oldEnPassantCol;
    m_enPassantR = oldEnPassantRow;
    m_enPassantR2 = oldEnPassantTarget;
}

int Chess::negamax(int depth, int alpha, int beta, int playerNumber) {
    if (depth == 0) {
        return evaluateBoard(playerNumber);
    }
    std::vector<ChessMove> moves = generateAllMoves(playerNumber);
    if (moves.empty()) {
        if (isInCheck(playerNumber)) {
            return -50000;
        }
        return 0;
    }
    std::sort(moves.begin(), moves.end(), [](const ChessMove& a, const ChessMove& b) {
        return (a.toSquare->bit() != nullptr) > (b.toSquare->bit() != nullptr);
    });
    int maxScore = INT_MIN;
    int opponentPlayer = 1 - playerNumber;
    for (const auto& move : moves) {
        Bit* capturedPiece = move.toSquare->bit();
        int oldEnPassantCol = m_enPassantC;
        int oldEnPassantRow = m_enPassantR;
        int oldEnPassantTarget = m_enPassantR2;
        makeMove(move);
        int score = -negamax(depth - 1, -beta, -alpha, opponentPlayer);
        unmakeMove(move, capturedPiece, oldEnPassantCol, oldEnPassantRow, oldEnPassantTarget);
        maxScore = std::max(maxScore, score);
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            break;
        }
    }
    return maxScore;
}

ChessMove Chess::findBestMove(int playerNumber) {
    std::vector<ChessMove> moves = generateAllMoves(playerNumber);
    if (moves.empty()) {
        return ChessMove(0, 0, 0, 0, nullptr, nullptr, nullptr);
    }
    std::sort(moves.begin(), moves.end(), [](const ChessMove& a, const ChessMove& b) {
        return (a.toSquare->bit() != nullptr) > (b.toSquare->bit() != nullptr);
    });
    
    ChessMove bestMove = moves[0];
    int bestScore = INT_MIN;
    int opponentPlayer = 1 - playerNumber;
    
    for (const auto& move : moves) {
        Bit* capturedPiece = move.toSquare->bit();
        int oldEnPassantCol = m_enPassantC;
        int oldEnPassantRow = m_enPassantR;
        int oldEnPassantTarget = m_enPassantR2;
        
        makeMove(move);
        int score = -negamax(3, INT_MIN, INT_MAX, opponentPlayer);
        unmakeMove(move, capturedPiece, oldEnPassantCol, oldEnPassantRow, oldEnPassantTarget);
        
        if (score > bestScore) {
            bestScore = score;
            bestMove = move;
        }
    }
    return bestMove;
}

void Chess::AIMove(int playerNumber) {
    ChessMove bestMove = findBestMove(playerNumber);
    if (bestMove.fromSquare && bestMove.toSquare && bestMove.piece) {
        if (bestMove.toSquare->bit()) {
            bestMove.toSquare->destroyBit();
        }
        bestMove.toSquare->setBit(bestMove.piece);
        bestMove.fromSquare->setBit(nullptr);
        bestMove.piece->setPosition(bestMove.toSquare->getPosition());
        endTurn();
    }
}

