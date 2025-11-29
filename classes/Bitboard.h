#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <iostream>

enum ChessPiece {
    NoPiece,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

class BitboardElement {
public:
    // Constructors
    BitboardElement();
    BitboardElement(uint64_t data);

    // Getters and Setters
    uint64_t getData() const;
    void setData(uint64_t data);

    // Method to loop through each bit in the element and perform an operation on it.
    template <typename Func>
    void forEachBit(Func func) const {
        if (_data != 0) {
            uint64_t tempData = _data;
            while (tempData) {
                int index = bitScanForward(tempData);
                func(index);
                tempData &= tempData - 1;
            }
        }
    }

    BitboardElement& operator|=(const uint64_t other);
    void printBitboard();

private:
    uint64_t _data;
    
    inline int bitScanForward(uint64_t bb) const {
#if defined(_MSC_VER) && !defined(__clang__)
        unsigned long index;
        _BitScanForward64(&index, bb);
        return index;
#else
        return __builtin_ffsll(bb) - 1;
#endif
    }
};

struct BitMove {
    uint8_t from;
    uint8_t to;
    uint8_t piece;

    BitMove(int from, int to, ChessPiece piece);
    BitMove();

    bool operator==(const BitMove& other) const;
};