#include "settingsmanager.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>

static QString colorToHex(const QColor& c) { return c.name(QColor::HexArgb); }
static QColor hexToColor(const QString& s, const QColor& def = Qt::black) {
	QColor c(s);
	return c.isValid() ? c : def;
}

QJsonObject ColorScheme::toJson() const {
	QJsonObject o;
	o["background"] = colorToHex(background);
	o["foreground"] = colorToHex(foreground);
	o["lineHighlight"] = colorToHex(lineHighlight);
	o["selection"] = colorToHex(selection);
	o["gutter"] = colorToHex(gutter);
	o["gutterFg"] = colorToHex(gutterFg);
	o["gutterFgCurrent"] = colorToHex(gutterFgCurrent);
	o["cursor"] = colorToHex(cursor);
	o["searchHighlight"] = colorToHex(searchHighlight);
	o["synKeyword"] = colorToHex(synKeyword);
	o["synPreprocessor"] = colorToHex(synPreprocessor);
	o["synString"] = colorToHex(synString);
	o["synComment"] = colorToHex(synComment);
	o["synNumber"] = colorToHex(synNumber);
	return o;
}

void ColorScheme::fromJson(const QJsonObject& o) {
	background = hexToColor(o["background"].toString(), background);
	foreground = hexToColor(o["foreground"].toString(), foreground);
	lineHighlight = hexToColor(o["lineHighlight"].toString(), lineHighlight);
	selection = hexToColor(o["selection"].toString(), selection);
	gutter = hexToColor(o["gutter"].toString(), gutter);
	gutterFg = hexToColor(o["gutterFg"].toString(), gutterFg);
	gutterFgCurrent = hexToColor(o["gutterFgCurrent"].toString(), gutterFgCurrent);
	cursor = hexToColor(o["cursor"].toString(), cursor);
	searchHighlight = hexToColor(o["searchHighlight"].toString(), searchHighlight);
	synKeyword = hexToColor(o["synKeyword"].toString(), synKeyword);
	synPreprocessor = hexToColor(o["synPreprocessor"].toString(), synPreprocessor);
	synString = hexToColor(o["synString"].toString(), synString);
	synComment = hexToColor(o["synComment"].toString(), synComment);
	synNumber = hexToColor(o["synNumber"].toString(), synNumber);
}

void KeyBindings::setDefaults() {
	binds["new"] = QKeySequence::New;
	binds["open"] = QKeySequence::Open;
	binds["save"] = QKeySequence::Save;
	binds["saveAs"] = QKeySequence::SaveAs;
	binds["closeTab"] = QKeySequence::Close;
	binds["quit"] = QKeySequence::Quit;
	binds["undo"] = QKeySequence::Undo;
	binds["redo"] = QKeySequence::Redo;
	binds["cut"] = QKeySequence::Cut;
	binds["copy"] = QKeySequence::Copy;
	binds["paste"] = QKeySequence::Paste;
	binds["selectAll"] = QKeySequence::SelectAll;
	binds["find"] = QKeySequence::Find;
	binds["findNext"] = QKeySequence::FindNext;
	binds["findPrev"] = QKeySequence::FindPrevious;
	binds["goToLine"] = QKeySequence(Qt::CTRL | Qt::Key_G);
	binds["toggleTerminal"] = QKeySequence(Qt::CTRL | Qt::Key_QuoteLeft);
	binds["zoomIn"] = QKeySequence(Qt::CTRL | Qt::Key_Plus);
	binds["zoomOut"] = QKeySequence(Qt::CTRL | Qt::Key_Minus);
	binds["zoomReset"] = QKeySequence(Qt::CTRL | Qt::Key_0);
	binds["runInTerminal"] = QKeySequence(Qt::CTRL | Qt::Key_F5);
	binds["macroStart"] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R);
	binds["macroStop"] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T);
	binds["macroPlay"] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P);
	binds["swapLineUp"] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Up);
	binds["swapLineDown"] = QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Down);
	binds["deleteWordBack"] = QKeySequence(Qt::CTRL | Qt::Key_Backspace);
	binds["deleteWordFwd"] = QKeySequence(Qt::CTRL | Qt::Key_Delete);
	binds["openFile"] = QKeySequence(Qt::CTRL | Qt::Key_E);
}

QJsonObject KeyBindings::toJson() const {
	QJsonObject o;
	for (auto it = binds.constBegin(); it != binds.constEnd(); ++it) o[it.key()] = it.value().toString();
	return o;
}

void KeyBindings::fromJson(const QJsonObject& o) {
	for (auto it = o.constBegin(); it != o.constEnd(); ++it) binds[it.key()] = QKeySequence(it.value().toString());
}

QJsonObject EditorOptions::toJson() const {
	QJsonObject o;
	o["tabWidth"] = tabWidth;
	o["useSpaces"] = useSpaces;
	o["wordWrap"] = wordWrap;
	o["showLineNumbers"] = showLineNumbers;
	o["enableSyntaxHighlighting"] = enableSyntaxHighlighting;
	o["fontSize"] = fontSize;
	o["fontFamily"] = fontFamily;
	return o;
}

void EditorOptions::fromJson(const QJsonObject& o) {
	if (o.contains("tabWidth")) tabWidth = o["tabWidth"].toInt(tabWidth);
	if (o.contains("useSpaces")) useSpaces = o["useSpaces"].toBool(useSpaces);
	if (o.contains("wordWrap")) wordWrap = o["wordWrap"].toBool(wordWrap);
	if (o.contains("showLineNumbers")) showLineNumbers = o["showLineNumbers"].toBool(showLineNumbers);
	if (o.contains("enableSyntaxHighlighting"))
		enableSyntaxHighlighting = o["enableSyntaxHighlighting"].toBool(enableSyntaxHighlighting);
	if (o.contains("fontSize")) fontSize = o["fontSize"].toInt(fontSize);
	if (o.contains("fontFamily")) fontFamily = o["fontFamily"].toString(fontFamily);
}

SettingsManager::SettingsManager() {
	m_keys.setDefaults();
	load();
}

SettingsManager& SettingsManager::instance() {
	static SettingsManager inst;
	return inst;
}

QString SettingsManager::defaultPath() const {
	QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
	QDir().mkpath(dir);
	return dir + "/settings.json";
}

bool SettingsManager::load(const QString& path) {
	QString p = path.isEmpty() ? defaultPath() : path;
	QFile f(p);
	if (!f.open(QIODevice::ReadOnly)) return false;

	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
	if (err.error != QJsonParseError::NoError) return false;

	QJsonObject root = doc.object();
	if (root.contains("colors")) m_colors.fromJson(root["colors"].toObject());
	if (root.contains("keys")) m_keys.fromJson(root["keys"].toObject());
	if (root.contains("options")) m_options.fromJson(root["options"].toObject());
	if (root.contains("macros")) macros = root["macros"].toObject();
	if (root.contains("recentFiles")) {
		QJsonArray arr = root["recentFiles"].toArray();
		for (const auto& v : arr) recentFiles.append(v.toString());
	}

	return true;
}

bool SettingsManager::save(const QString& path) {
	QString p = path.isEmpty() ? defaultPath() : path;
	QFile f(p);
	if (!f.open(QIODevice::WriteOnly)) return false;

	QJsonObject root;
	root["colors"] = m_colors.toJson();
	root["keys"] = m_keys.toJson();
	root["options"] = m_options.toJson();
	QJsonArray arr;
	for (const QString& file : recentFiles) arr.append(file);
	root["recentFiles"] = arr;

	f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
	return true;
}

QFont SettingsManager::getFont() const {
	QFont f(m_options.fontFamily, m_options.fontSize);
	f.setStyleHint(QFont::TypeWriter);
	return f;
}

void SettingsManager::setFont(const QFont& f) {
	m_options.fontFamily = f.family();
	m_options.fontSize = f.pointSize();
}