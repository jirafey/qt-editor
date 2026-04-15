#include "linuxcommandrunner.h"
#include <QProcess>

LinuxCommandRunner::LinuxCommandRunner(QObject* parent) : QObject(parent) {}

void LinuxCommandRunner::runShell(const QString& cmd, const QString& input) {
	QProcess* proc = new QProcess(this);

	connect(proc, &QProcess::finished, this, [this, proc](int exitCode, QProcess::ExitStatus status) {
		Q_UNUSED(exitCode);
		Q_UNUSED(status);
		emit outputReady(QString::fromUtf8(proc->readAllStandardOutput()));
		proc->deleteLater();
	});

	proc->start("sh", QStringList() << "-c" << cmd);
	proc->write(input.toUtf8());
	proc->closeWriteChannel();
}