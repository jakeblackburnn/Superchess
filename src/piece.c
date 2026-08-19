#include "superchess.h"

Piece create_piece(PieceType type, Color color) {
    return (Piece){type, color};
}
