#ifndef SHORTCUTMANAGER_H
#define SHORTCUTMANAGER_H

#include <QKeyEvent>
#include <QKeySequence>
#include <QMap>
#include <QObject>
#include <functional>

class ShortcutManager : public QObject {
	Q_OBJECT
  public:
	explicit ShortcutManager(QObject* parent = nullptr);

	void registerAction(const QString& id, const QKeySequence& key, std::function<void()> func);
	void updateAction(const QString& id, const QKeySequence& key);
	bool handleEvent(QKeyEvent* event);
	QMap<QString, QKeySequence> allActions() const;

  private:
	struct Action {
		QKeySequence key;
		std::function<void()> callback;
	};
	QMap<QString, Action> m_actions;
};

#endif