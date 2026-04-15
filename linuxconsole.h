#ifndef LINUXCONSOLE_H
#define LINUXCONSOLE_H

#include <QLineEdit>
#include <QProcess>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

class LinuxConsole : public QWidget {
	Q_OBJECT
  public:
	void runCommand(const QString& cmd);
	explicit LinuxConsole(QWidget* parent = nullptr);
  private slots:
	void executeCommand();

  private:
	QTextEdit* m_output;
	QLineEdit* m_input;
};
#endif