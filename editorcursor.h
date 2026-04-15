#ifndef EDITORCURSOR_H
#define EDITORCURSOR_H

#include <cstddef>

struct TextPos {
	int line = 0;
	int col = 0;
	bool operator!=(const TextPos& o) const { return line != o.line || col != o.col; }
	bool operator==(const TextPos& o) const { return line == o.line && col == o.col; }
	bool operator<(const TextPos& o) const { return line != o.line ? line < o.line : col < o.col; }
};

class EditorCursor {
  public:
	TextPos current;
	TextPos anchor;
	bool selecting = false;
	void reset() {
		current = {};
		anchor = {};
		selecting = false;
	}
};

#endif