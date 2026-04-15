#include "fuzzysearcher.h"
#include <algorithm>

bool FuzzySearcher::match(const QString& pattern, const QString& text) {
	if (pattern.isEmpty()) return true;
	int pi = 0;
	for (int ti = 0; ti < text.length() && pi < pattern.length(); ++ti) {
		if (pattern[pi].toLower() == text[ti].toLower()) pi++;
	}
	return pi == pattern.length();
}

int FuzzySearcher::score(const QString& pattern, const QString& text) {
	if (pattern.isEmpty()) return 0;
	int sc = 0, pi = 0;
	bool prevMatch = false;
	for (int ti = 0; ti < text.length() && pi < pattern.length(); ++ti) {
		if (pattern[pi].toLower() == text[ti].toLower()) {
			sc += prevMatch ? 2 : 1;
			if (pattern[pi] == text[ti]) sc++;
			if (ti == 0) sc += 3;
			prevMatch = true;
			pi++;
		} else {
			prevMatch = false;
		}
	}
	if (pi != pattern.length()) return -1;

	sc -= text.length() / 5;

	int lastSlash = text.lastIndexOf('/');
	QString baseName = (lastSlash >= 0) ? text.mid(lastSlash + 1) : text;
	if (baseName.startsWith(pattern, Qt::CaseInsensitive)) { sc += 100; }
	return sc;
}

QStringList FuzzySearcher::search(const QString& pattern, const QStringList& candidates, int maxResults) {
	if (pattern.isEmpty()) return candidates.mid(0, maxResults);

	struct Scored {
		QString text;
		int score;
	};
	QList<Scored> results;

	for (const QString& candidate : candidates) {
		int s = score(pattern, candidate);
		if (s >= 0) results.append({candidate, s});
	}

	std::sort(results.begin(), results.end(), [](const Scored& a, const Scored& b) { return a.score > b.score; });

	QStringList out;
	int limit = qMin((int)results.size(), maxResults);
	out.reserve(limit);
	for (int i = 0; i < limit; ++i) out << results[i].text;
	return out;
}

std::vector<size_t> FuzzySearcher::findMatches(const QString& pattern, const char* data, size_t size,
											   size_t maxResults) {
	std::vector<size_t> results;
	if (pattern.isEmpty() || !data) return results;

	QByteArray p = pattern.toUtf8();
	char first = p[0];
	size_t qLen = (size_t)p.size();

	for (size_t i = 0; i + qLen <= size; ++i) {
		if (data[i] == first) {

			bool ok = true;
			for (size_t j = 1; j < qLen; ++j) {
				if (data[i + j] != p[(int)j]) {
					ok = false;
					break;
				}
			}
			if (ok) {
				results.push_back(i);
				if (results.size() >= maxResults) break;
			}
		}
	}
	return results;
}