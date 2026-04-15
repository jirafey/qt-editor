#include "shortcutmanager.h"

ShortcutManager::ShortcutManager(QObject* parent) : QObject(parent) {}

void ShortcutManager::registerAction(const QString& id, const QKeySequence& key, std::function<void()> func) {
	m_actions[id] = {key, func};
}

void ShortcutManager::updateAction(const QString& id, const QKeySequence& key) {
	if (m_actions.contains(id)) m_actions[id].key = key;
}

bool ShortcutManager::handleEvent(QKeyEvent* event) {
	QKeySequence seq(event->modifiers() | event->key());
	for (auto it = m_actions.begin(); it != m_actions.end(); ++it) {
		if (it.value().key == seq) {
			if (it.value().callback) it.value().callback();
			return true;
		}
	}
	return false;
}

QMap<QString, QKeySequence> ShortcutManager::allActions() const {
	QMap<QString, QKeySequence> result;
	for (auto it = m_actions.constBegin(); it != m_actions.constEnd(); ++it) result[it.key()] = it.value().key;
	return result;
}