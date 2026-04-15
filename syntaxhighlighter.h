#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QColor>
#include <QHash>
#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QTextLayout>

struct HighlightRule {
	QRegularExpression pattern;
	QColor color;
	bool bold;
};

class SyntaxHighlighter {
  public:
	SyntaxHighlighter();

	void setFilePath(const QString& path);

	void reloadColors();
	bool isEnabled() const { return m_enabled; }

	QList<QTextLayout::FormatRange> getLineFormat(const QString& text);

  private:
	void buildRules();

	QList<HighlightRule> m_rules;
	bool m_enabled = true;

	mutable QHash<size_t, QList<QTextLayout::FormatRange>> m_cache;
};

#endif