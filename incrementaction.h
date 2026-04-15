#ifndef INCREMENTACTION_H
#define INCREMENTACTION_H

#include "editorcursor.h"
#include "piecetable.h"
#include <QString>

class IncrementAction {
  public:
	static bool run(PieceTable& table, EditorCursor& cursor, int delta);

	struct NumberInfo {
		int col;
		int length;
		long long value;
	};
	static NumberInfo findFirstNumber(const QString& text, int startCol = 0);
};

#endif