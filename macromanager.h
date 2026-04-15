#ifndef MACROMANAGER_H
#define MACROMANAGER_H

#include <QKeyEvent>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

enum class MacroStepType { KeyEvent, IncrementNumber };

struct MacroStep {
	MacroStepType type = MacroStepType::KeyEvent;

	int key = 0;
	Qt::KeyboardModifiers mods;
	QString text;

	int incrementDelta = 1;
};

class EditorView;

class MacroManager : public QObject {
	Q_OBJECT
  public:
	explicit MacroManager(QObject* parent = nullptr);

	void startRecording();
	void stopRecording(const QString& name);
	void record(QKeyEvent* e);

	void recordIncrementStep(int delta, const QString& macroName = QString());

	void play(const QString& name, EditorView* view);
	bool isRecording() const;
	bool isPlaying() const;

	QStringList macroNames() const;
	void removeMacro(const QString& name);

	QJsonObject toJson() const;
	void fromJson(const QJsonObject& root);
	const QList<MacroStep>& stepsFor(const QString& name) const {
		static const QList<MacroStep> empty;
		auto it = m_library.constFind(name);
		return (it != m_library.constEnd()) ? it.value() : empty;
	}

	void setSteps(const QString& name, const QList<MacroStep>& steps) { m_library[name] = steps; }

  signals:
	void recordingStarted();
	void recordingStopped(const QString& name);

  private:
	bool m_recording = false;
	bool m_playing = false;
	QList<MacroStep> m_currentMacro;
	QMap<QString, QList<MacroStep>> m_library;
};

#endif