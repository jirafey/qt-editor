#include "piece.h"

Piece::Piece() : bufferType(BufferType::Original), start(0), length(0) {}

Piece::Piece(BufferType type, size_t s, size_t len) : bufferType(type), start(s), length(len) {}