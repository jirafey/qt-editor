#ifndef FUZZYSEARCHER_H
#define FUZZYSEARCHER_H

#include <QString>
#include <QStringList>
#include <vector>

class FuzzySearcher {
  public:
	static bool match(const QString& pattern, const QString& text);
	static int score(const QString& pattern, const QString& text);
	static QStringList search(const QString& pattern, const QStringList& candidates, int maxResults = 20);

	static std::vector<size_t> findMatches(const QString& pattern, const char* data, size_t size,
										   size_t maxResults = 1000);
};

#endif