#include "BitboardElement.h"

// Constructors
BitboardElement::BitboardElement() : _data(0) {
}

BitboardElement::BitboardElement(uint64_t data) : _data(data) {
}

// Getters and Setters
uint64_t BitboardElement::getData() const {
    return _data;
}

void BitboardElement::setData(uint64_t data) {
    _data = data;
}

// Operators
BitboardElement& BitboardElement::operator|=(const uint64_t other) {
    _data |= other;
    return *this;
}

void BitboardElement::printBitboard() {
    std::cout << "\n a b c d e f g h\n";
    for (int rank = 7; rank >= 0; rank--) {
        std::cout << (rank + 1) << " ";
        for (int file = 0; file < 8; file++) {
            int square = rank * 8 + file;
            if (_data & (1ULL << square)) {
                std::cout << "X ";
            } else {
                std::cout << ". ";
            }
        }
        std::cout << (rank + 1) << "\n";
        std::cout << std::flush;
    }
    std::cout << " a b c d e f g h\n";
    std::cout << std::flush;
}

// BitMove Constructors
BitMove::BitMove(int from, int to, ChessPiece piece) 
    : from(from), to(to), piece(piece) {
}

BitMove::BitMove() : from(0), to(0), piece(NoPiece) {
}

// BitMove Operators
bool BitMove::operator==(const BitMove& other) const {
    return from == other.from && to == other.to && piece == other.piece;
}