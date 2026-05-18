#include "ProcessUtil.h"

#include <QProcess>

bool ProcessUtil::programExists(const QString &program)
{
    QProcess process;
    process.start(program, { "-version" });
    if (!process.waitForStarted(1000)) {
        return false;
    }
    if (!process.waitForFinished(1500)) {
        process.kill();
        process.waitForFinished();
        return false;
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

QString ProcessUtil::programVersionOutput(const QString &program)
{
    QProcess process;
    process.start(program, { "-version" });
    if (!process.waitForStarted(1000)) {
        return {};
    }
    if (!process.waitForFinished(1500)) {
        process.kill();
        process.waitForFinished();
        return {};
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        return {};
    }
    return QString::fromUtf8(process.readAllStandardOutput());
}

ProcessResult ProcessUtil::runCommand(const QString &program, const QStringList &args, std::atomic_bool *cancel)
{
    ProcessResult result;
    if (program.isEmpty()) {
        result.stderrText = "Empty process command.";
        return result;
    }

    QProcess process;
    process.start(program, args);
    if (!process.waitForStarted(10'000)) {
        result.stderrText = QStringLiteral("Unable to start %1").arg(program);
        return result;
    }

    QByteArray stdoutBuf;
    QByteArray stderrBuf;
    while (!process.waitForFinished(100)) {
        stdoutBuf += process.readAllStandardOutput();
        stderrBuf += process.readAllStandardError();
        if (cancel && *cancel) {
            process.kill();
            process.waitForFinished();
            result.stderrText = "Process cancelled.";
            return result;
        }
    }
    stdoutBuf += process.readAllStandardOutput();
    stderrBuf += process.readAllStandardError();

    result.stdoutText = QString::fromUtf8(stdoutBuf);
    result.stderrText = QString::fromUtf8(stderrBuf).trimmed();
    result.exitCode = process.exitCode();
    result.ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (!result.ok && result.stderrText.isEmpty()) {
        result.stderrText = QStringLiteral("%1 exited with code %2").arg(program).arg(process.exitCode());
    }
    return result;
}
