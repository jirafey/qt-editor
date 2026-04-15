#include "syntaxhighlighter.h"
#include "settingsmanager.h"
#include <QFileInfo>
#include <QFont>
#include <functional>

SyntaxHighlighter::SyntaxHighlighter() { buildRules(); }

void SyntaxHighlighter::setFilePath(const QString& path) {
	static const QStringList hlExts = {"cpp", "c", "h", "hpp", "cxx", "cc", "hxx"};
	const QString ext = QFileInfo(path).suffix().toLower();
	m_enabled = path.isEmpty() || hlExts.contains(ext);
	m_cache.clear();
}

void SyntaxHighlighter::reloadColors() {
	m_cache.clear();
	buildRules();
}

void SyntaxHighlighter::buildRules() {
	m_rules.clear();
	const auto& c = SettingsManager::instance().colors();

	QStringList syntaxWords = {
#include "syntax_words.inc"
	};

	for (const QString& word : syntaxWords) {
		m_rules.append({QRegularExpression("\\b" + word + "\\b"), c.synKeyword, true});
	}

	m_rules.append({QRegularExpression("#[ "
									   "\\t]*(?:include|define|undef|if|ifdef|ifndef|else|elif|elifdef|elifndef|endif|"
									   "line|error|warning|pragma|embed)\\b"),
					c.synPreprocessor, false});

	m_rules.append({QRegularExpression("\"(?:[^\"\\\\]|\\\\.)*\""), c.synString, false});
	m_rules.append({QRegularExpression("'(?:[^'\\\\]|\\\\.)*'"), c.synString, false});
	m_rules.append({QRegularExpression("//[^\n]*"), c.synComment, false});
	m_rules.append({QRegularExpression("/\\*.*\\*/"), c.synComment, false});
	m_rules.append({QRegularExpression("\\b(0x[0-9a-fA-F]+|\\d+\\.?\\d*)\\b"), c.synNumber, false});
}

QList<QTextLayout::FormatRange> SyntaxHighlighter::getLineFormat(const QString& text) {
	if (!m_enabled || !SettingsManager::instance().options().enableSyntaxHighlighting || text.isEmpty()) return {};

	const size_t h = qHash(text);
	auto it = m_cache.find(h);
	if (it != m_cache.end()) return it.value();

	QList<QTextLayout::FormatRange> formats;
	for (const auto& rule : m_rules) {
		QRegularExpressionMatchIterator mi = rule.pattern.globalMatch(text);
		while (mi.hasNext()) {
			const QRegularExpressionMatch match = mi.next();
			QTextLayout::FormatRange range;
			range.start = match.capturedStart();
			range.length = match.capturedLength();
			range.format.setForeground(rule.color);
			if (rule.bold) range.format.setFontWeight(QFont::Bold);
			formats.append(range);
		}
	}

	if (m_cache.size() > 4096) m_cache.clear();
	m_cache[h] = formats;
	return formats;
}