#ifndef PIECE_H
#define PIECE_H

#include <cstddef>

enum class BufferType { Original, Add };

struct Piece {
	BufferType bufferType;
	size_t start;
	size_t length;

	Piece();
	Piece(BufferType type, size_t s, size_t len);
};

#endif