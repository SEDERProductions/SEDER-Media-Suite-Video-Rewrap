#pragma once

#include <QString>
#include <QStringList>
#include <atomic>

struct ProcessResult {
    bool ok = false;
    QString stdoutText;
    QString stderrText;
    int exitCode = -1;
};

namespace ProcessUtil {
    bool programExists(const QString &program);

    // Probes a program with `-version` and returns its first-line banner via
    // stdout. Returns an empty string if the program cannot be started.
    QString programVersionOutput(const QString &program);

    ProcessResult runCommand(const QString &program, const QStringList &args, std::atomic_bool *cancel = nullptr);
}
