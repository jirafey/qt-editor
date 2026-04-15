#include "macromanager.h"
#include "editorview.h"
#include <QApplication>
#include <QClipboard>
#include <QJsonArray>
#include <QJsonObject>

MacroManager::MacroManager(QObject* parent) : QObject(parent) {}

void MacroManager::startRecording() {
	m_recording = true;
	m_currentMacro.clear();
	emit recordingStarted();
}

void MacroManager::stopRecording(const QString& name) {
	m_recording = false;
	if (!name.isEmpty()) m_library[name] = m_currentMacro;
	emit recordingStopped(name);
}

void MacroManager::record(QKeyEvent* e) {
	if (!m_recording) return;

	int k = e->key();
	if (k == Qt::Key_Shift || k == Qt::Key_Control || k == Qt::Key_Alt || k == Qt::Key_Meta || k == Qt::Key_Super_L ||
		k == Qt::Key_Super_R) {
		return;
	}

	MacroStep step;
	step.type = MacroStepType::KeyEvent;
	step.key = k;
	step.mods = e->modifiers();

	bool hasCtrlOrAlt = e->modifiers() & (Qt::ControlModifier | Qt::AltModifier);
	if (!hasCtrlOrAlt && !e->text().isEmpty() && e->text().at(0).isPrint())
		step.text = e->text();
	else
		step.text = QString();

	m_currentMacro.append(step);
}

void MacroManager::recordIncrementStep(int delta, const QString& macroName) {
	MacroStep step;
	step.type = MacroStepType::IncrementNumber;
	step.incrementDelta = delta;
	if (!macroName.isEmpty() && m_library.contains(macroName)) {
		m_library[macroName].append(step);
	} else if (m_recording) {
		m_currentMacro.append(step);
	}
}

bool MacroManager::isRecording() const { return m_recording; }
bool MacroManager::isPlaying() const { return m_playing; }

void MacroManager::play(const QString& name, EditorView* view) {
	if (!m_library.contains(name) || !view) return;

	m_playing = true;
	view->syncLocalClipboard();

	const QList<MacroStep>& steps = m_library[name];
	for (const auto& step : steps) {
		if (step.type == MacroStepType::IncrementNumber) {
			view->incrementCurrentNumber(step.incrementDelta);
		} else {
			QKeyEvent ev(QEvent::KeyPress, step.key, step.mods, step.text);
			view->injectKeyEvent(&ev);
		}
	}

	m_playing = false;
}

QStringList MacroManager::macroNames() const { return m_library.keys(); }

void MacroManager::removeMacro(const QString& name) { m_library.remove(name); }

QJsonObject MacroManager::toJson() const {
	QJsonObject root;
	for (auto it = m_library.constBegin(); it != m_library.constEnd(); ++it) {
		QJsonArray arr;
		for (const auto& step : it.value()) {
			QJsonObject s;
			s["type"] = (int)step.type;
			s["key"] = step.key;
			s["mods"] = (int)step.mods;
			s["text"] = step.text;
			s["incrementDelta"] = step.incrementDelta;
			arr.append(s);
		}
		root[it.key()] = arr;
	}
	return root;
}

void MacroManager::fromJson(const QJsonObject& root) {
	m_library.clear();
	for (auto it = root.constBegin(); it != root.constEnd(); ++it) {

		const QJsonArray arr = it.value().toArray();
		QList<MacroStep> steps;

		for (const QJsonValue& val : arr) {
			QJsonObject s = val.toObject();
			MacroStep step;
			step.type = (MacroStepType)s["type"].toInt();
			step.key = s["key"].toInt();
			step.mods = (Qt::KeyboardModifiers)s["mods"].toInt();
			step.text = s["text"].toString();
			step.incrementDelta = s["incrementDelta"].toInt();
			steps.append(step);
		}
		m_library[it.key()] = steps;
	}
}