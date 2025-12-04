#include "Chess.h"
#include <algorithm>
#include <cstring>

Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    
    // Initialize starting position FEN
    const char* startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w";
    char state[64];
    std::memset(state, '0', 64);
    
    // Parse FEN board part
    int rank = 7;
    int file = 0;
    const char* fenPtr = startFEN;
    
    while (*fenPtr && *fenPtr != ' ') {
        char c = *fenPtr;
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            if (file < 8 && rank >= 0) {
                state[rank * 8 + file] = c;
            }
            file++;
        }
        fenPtr++;
    }
    
    // Initialize GameState
    char currentPlayer = WHITE;
    if (*fenPtr == ' ') {
        fenPtr++;
        if (*fenPtr == 'b' || *fenPtr == 'B') {
            currentPlayer = BLACK;
        }
    }
    
    _gameState.init(state, currentPlayer);
    
    // Create visual pieces
    updateVisualFromGameState();
    
    // Ensure GameState.color matches the initial current player
    // startGame() sets currentTurnNo = 0, so player 0 (white) goes first
    _gameState.color = WHITE;
    
    startGame();
    
    // After startGame(), ensure color is still correct
    _gameState.color = WHITE;
}

void Chess::endTurn()
{
    // Call the base class endTurn() - it increments currentTurnNo and handles turn history
    Game::endTurn();
    
    // After endTurn(), currentTurnNo has been incremented, so getCurrentPlayer() returns the next player
    // Sync GameState.color with the new current player
    // This is CRITICAL - the next player should now be able to move
    Player* newCurrentPlayer = getCurrentPlayer();
    if (newCurrentPlayer) {
        _gameState.color = (newCurrentPlayer->playerNumber() == 0) ? WHITE : BLACK;
    }
}

char Chess::pieceNotation(int x, int y) const
{
    int index = squareToIndex(x, y);
    return _gameState.state[index];
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

ChessPiece Chess::getPieceType(const Bit* bit) const
{
    if (!bit) return NoPiece;
    int gameTag = bit->gameTag();
    return static_cast<ChessPiece>(gameTag & 0x7F);
}

void Chess::updateVisualFromGameState()
{
    // Clear all squares
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    
    // Create pieces based on GameState
    for (int i = 0; i < 64; i++) {
        char piece = _gameState.state[i];
        if (piece == '0') continue;
        
        int x, y;
        indexToSquare(i, x, y);
        ChessSquare* square = _grid->getSquare(x, y);
        if (!square) continue;
        
        bool isWhite = (piece >= 'A' && piece <= 'Z');
        int playerNumber = isWhite ? 0 : 1;
        char pieceLower = isWhite ? (piece + 32) : piece;
        
        ChessPiece pieceType = NoPiece;
        switch (pieceLower) {
            case 'p': pieceType = Pawn; break;
            case 'n': pieceType = Knight; break;
            case 'b': pieceType = Bishop; break;
            case 'r': pieceType = Rook; break;
            case 'q': pieceType = Queen; break;
            case 'k': pieceType = King; break;
        }
        
        if (pieceType != NoPiece) {
            Bit* bit = PieceForPlayer(playerNumber, pieceType);
            int gameTag = pieceType + (playerNumber == 1 ? 128 : 0);
            bit->setGameTag(gameTag);
            bit->setPosition(square->getPosition());
            square->setBit(bit);
        }
    }
}

void Chess::syncGameStateToVisual()
{
    // CRITICAL: Always sync GameState.color with the current player FIRST
    // This ensures we're always generating moves for the correct player
    Player* currentPlayer = getCurrentPlayer();
    if (currentPlayer) {
        char expectedColor = (currentPlayer->playerNumber() == 0) ? WHITE : BLACK;
        _gameState.color = expectedColor;
    }
    
    // Update GameState board state from visual representation
    for (int i = 0; i < 64; i++) {
        int x, y;
        indexToSquare(i, x, y);
        ChessSquare* square = _grid->getSquare(x, y);
        if (square && square->bit()) {
            Bit* bit = square->bit();
            ChessPiece pieceType = getPieceType(bit);
            int playerNumber = bit->getOwner() ? bit->getOwner()->playerNumber() : 0;
            
            char pieceChar = '0';
            switch (pieceType) {
                case Pawn: pieceChar = playerNumber == 0 ? 'P' : 'p'; break;
                case Knight: pieceChar = playerNumber == 0 ? 'N' : 'n'; break;
                case Bishop: pieceChar = playerNumber == 0 ? 'B' : 'b'; break;
                case Rook: pieceChar = playerNumber == 0 ? 'R' : 'r'; break;
                case Queen: pieceChar = playerNumber == 0 ? 'Q' : 'q'; break;
                case King: pieceChar = playerNumber == 0 ? 'K' : 'k'; break;
            }
            _gameState.state[i] = pieceChar;
        } else {
            _gameState.state[i] = '0';
        }
    }
}

void Chess::syncVisualToGameState()
{
    // Alias for syncGameStateToVisual - same functionality
    syncGameStateToVisual();
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // CRITICAL: First check if it's the current player's piece
    // This is the primary gate - only the current player can move their pieces
    Player* currentPlayer = getCurrentPlayer();
    if (!bit.getOwner() || bit.getOwner() != currentPlayer) {
        return false;
    }
    
    // Sync GameState to match current visual state and current player
    // This ensures GameState.color matches getCurrentPlayer()
    syncGameStateToVisual();
    
    // Generate legal moves for the current player
    _currentLegalMoves = _gameState.generateAllMoves();
    
    // Check if this specific piece has any legal moves
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int srcIndex = squareToIndex(srcX, srcY);
    
    for (const auto& move : _currentLegalMoves) {
        if (move.from == srcIndex) {
            return true;
        }
    }
    
    return false;
}

BitMove Chess::findMoveInList(int fromX, int fromY, int toX, int toY, const std::vector<BitMove>& moves) const
{
    int fromIndex = squareToIndex(fromX, fromY);
    int toIndex = squareToIndex(toX, toY);
    
    for (const auto& move : moves) {
        if (move.from == fromIndex && move.to == toIndex) {
            return move;
        }
    }
    
    // Return invalid move if not found
    return BitMove(0, 0, NoPiece, 0);
}

bool Chess::isValidMove(int fromX, int fromY, int toX, int toY)
{
    // Sync current state and get legal moves
    syncGameStateToVisual();
    std::vector<BitMove> legalMoves = _gameState.generateAllMoves();
    
    BitMove move = findMoveInList(fromX, fromY, toX, toY, legalMoves);
    return move.piece != NoPiece;
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
    
    // Check if destination has friendly piece
    Bit* dstBit = dstSquare->bit();
    if (dstBit && dstBit->getOwner() == bit.getOwner()) return false;
    
    // Use GameState to validate the move
    // syncGameStateToVisual() will set the color correctly based on current player
    syncGameStateToVisual();
    _currentLegalMoves = _gameState.generateAllMoves();
    
    BitMove move = findMoveInList(srcX, srcY, dstX, dstY, _currentLegalMoves);
    return move.piece != NoPiece;
}

void Chess::applyMove(const BitMove& move)
{
    // Apply move to GameState first (this handles all logic including special moves)
    // pushMove() will flip GameState.color to the next player
    _gameState.pushMove(move);
    
    // Update visual representation to match GameState
    // This ensures castling, en passant, promotion, etc. are all handled correctly
    updateVisualFromGameState();
    
    // Note: GameState.color has been flipped by pushMove() to the next player
    // But we haven't incremented currentTurnNo yet, so getCurrentPlayer() still returns the old player
    // This will be fixed when endTurn() is called, which increments currentTurnNo and syncs the color
}

void Chess::handleSpecialMoves(const BitMove& move)
{
    // This function is a placeholder for visual-only special move effects
    // Most special moves are handled by GameState::pushMove() and updateVisualFromGameState()
    // This can be used for animations, highlights, or other visual effects if needed
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcSquare = static_cast<ChessSquare*>(&src);
    ChessSquare* dstSquare = static_cast<ChessSquare*>(&dst);
    
    int srcX = srcSquare->getColumn();
    int srcY = srcSquare->getRow();
    int dstX = dstSquare->getColumn();
    int dstY = dstSquare->getRow();
    
    // Sync GameState to current visual state and current player
    // This ensures we're validating the move for the correct player
    syncGameStateToVisual();
    _currentLegalMoves = _gameState.generateAllMoves();
    
    // Find the move in the legal moves list
    BitMove move = findMoveInList(srcX, srcY, dstX, dstY, _currentLegalMoves);
    
    if (move.piece != NoPiece) {
        // Apply the move to GameState (this flips GameState.color internally)
        applyMove(move);
        
        // CRITICAL: endTurn() increments currentTurnNo and syncs GameState.color
        // This switches to the next player's turn
        endTurn();
        
        // Final sync to ensure everything is consistent for the next player
        syncGameStateToVisual();
    }
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    
    // Reset GameState
    const char* startFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w";
    char state[64];
    std::memset(state, '0', 64);
    
    int rank = 7;
    int file = 0;
    const char* fenPtr = startFEN;
    
    while (*fenPtr && *fenPtr != ' ') {
        char c = *fenPtr;
        if (c == '/') {
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += (c - '0');
        } else {
            if (file < 8 && rank >= 0) {
                state[rank * 8 + file] = c;
            }
            file++;
        }
        fenPtr++;
    }
    
    _gameState.init(state, WHITE);
    _currentLegalMoves.clear();
}

Player* Chess::checkForWinner()
{
    // Sync GameState to current player before checking for winner
    syncGameStateToVisual();
    _currentLegalMoves = _gameState.generateAllMoves();
    
    // If current player has no legal moves, check if they're in check
    if (_currentLegalMoves.empty()) {
        // Check if king is in check by generating opponent's moves
        char currentColor = _gameState.color;
        char opponentColor = (currentColor == WHITE) ? BLACK : WHITE;
        
        // Temporarily switch to opponent's perspective to generate their moves
        _gameState.color = opponentColor;
        std::vector<BitMove> opponentMoves = _gameState.generateAllMoves();
        _gameState.color = currentColor; // Restore current player's color
        
        // Find current player's king
        int kingIndex = -1;
        char kingChar = (currentColor == WHITE) ? 'K' : 'k';
        for (int i = 0; i < 64; i++) {
            if (_gameState.state[i] == kingChar) {
                kingIndex = i;
                break;
            }
        }
        
        // Check if any opponent move attacks the king
        if (kingIndex != -1) {
            for (const auto& oppMove : opponentMoves) {
                if (oppMove.to == kingIndex) {
                    // Checkmate! Return the opponent as winner
                    return currentColor == WHITE ? getPlayerAt(1) : getPlayerAt(0);
                }
            }
        }
    }
    
    return nullptr;
}

bool Chess::checkForDraw()
{
    // Sync GameState to current player before checking for draw
    syncGameStateToVisual();
    _currentLegalMoves = _gameState.generateAllMoves();
    
    // Stalemate: no legal moves and not in check
    if (_currentLegalMoves.empty()) {
        char currentColor = _gameState.color;
        char opponentColor = (currentColor == WHITE) ? BLACK : WHITE;
        
        // Check if king is in check by generating opponent's moves
        _gameState.color = opponentColor;
        std::vector<BitMove> opponentMoves = _gameState.generateAllMoves();
        _gameState.color = currentColor; // Restore current player's color
        
        int kingIndex = -1;
        char kingChar = (currentColor == WHITE) ? 'K' : 'k';
        for (int i = 0; i < 64; i++) {
            if (_gameState.state[i] == kingChar) {
                kingIndex = i;
                break;
            }
        }
        
        if (kingIndex != -1) {
            bool inCheck = false;
            for (const auto& oppMove : opponentMoves) {
                if (oppMove.to == kingIndex) {
                    inCheck = true;
                    break;
                }
            }
            
            if (!inCheck) {
                return true; // Stalemate - no legal moves and not in check
            }
        }
    }
    
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
    for (int i = 0; i < 64; i++) {
        s += _gameState.state[i];
    }
    return s;
}

void Chess::setStateString(const std::string &s)
{
    if (s.length() < 64) return;
    
    for (int i = 0; i < 64; i++) {
        _gameState.state[i] = s[i];
    }
    
    // Determine current player from turn number
    char currentPlayer = (_gameOptions.currentTurnNo % 2 == 0) ? WHITE : BLACK;
    _gameState.color = currentPlayer;
    
    updateVisualFromGameState();
}
