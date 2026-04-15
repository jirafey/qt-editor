#ifndef LINUXCOMMANDRUNNER_H
#define LINUXCOMMANDRUNNER_H

#include <QObject>
#include <QProcess>

class LinuxCommandRunner : public QObject {
	Q_OBJECT
  public:
	explicit LinuxCommandRunner(QObject* parent = nullptr);
	void runShell(const QString& cmd, const QString& input);
	void kill();

  signals:
	void outputReady(const QString& out);
	void errorReady(const QString& err);
	void finished(int exitCode);
};

#endif