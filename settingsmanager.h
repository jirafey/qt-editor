#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QColor>
#include <QFont>
#include <QJsonObject>
#include <QKeySequence>
#include <QMap>
#include <QString>

struct ColorScheme {
	QColor background = QColor(30, 30, 30);
	QColor foreground = QColor(212, 212, 212);
	QColor lineHighlight = QColor(255, 255, 255, 18);
	QColor selection = QColor(38, 79, 120, 180);
	QColor gutter = QColor(37, 37, 37);
	QColor gutterFg = QColor(100, 100, 100);
	QColor gutterFgCurrent = QColor(200, 200, 200);
	QColor cursor = QColor(220, 220, 220);
	QColor searchHighlight = QColor(255, 200, 0, 80);

	QColor synKeyword = QColor(86, 156, 214);
	QColor synPreprocessor = QColor(189, 99, 197);
	QColor synString = QColor(206, 145, 120);
	QColor synComment = QColor(106, 153, 85);
	QColor synNumber = QColor(181, 206, 168);

	QJsonObject toJson() const;
	void fromJson(const QJsonObject& o);
};

struct KeyBindings {
	QMap<QString, QKeySequence> binds;

	void setDefaults();
	QJsonObject toJson() const;
	void fromJson(const QJsonObject& o);

	QKeySequence get(const QString& action) const { return binds.value(action); }
};

struct EditorOptions {
	int tabWidth = 4;
	bool useSpaces = true;
	bool wordWrap = false;
	bool showLineNumbers = true;
	bool enableSyntaxHighlighting = true;
	int fontSize = 11;
	QString fontFamily = "Monospace";

	QJsonObject toJson() const;
	void fromJson(const QJsonObject& o);
};

class SettingsManager {
  public:
	static SettingsManager& instance();

	bool load(const QString& path = QString());
	bool save(const QString& path = QString());
	QString defaultPath() const;

	ColorScheme& colors() { return m_colors; }
	KeyBindings& keys() { return m_keys; }
	EditorOptions& options() { return m_options; }

	const ColorScheme& colors() const { return m_colors; }
	const KeyBindings& keys() const { return m_keys; }
	const EditorOptions& options() const { return m_options; }

	QFont getFont() const;
	void setFont(const QFont& f);
	int getTabWidth() const { return m_options.tabWidth; }
	void setTabWidth(int w) { m_options.tabWidth = w; }

	QJsonObject macros;
	QStringList recentFiles;

  private:
	SettingsManager();
	ColorScheme m_colors;
	KeyBindings m_keys;
	EditorOptions m_options;
};

#endif