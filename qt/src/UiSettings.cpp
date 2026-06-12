#include "UiSettings.h"

namespace {
QString prefixed(const QString &key)
{
    return QStringLiteral("ui/") + key;
}
}

UiSettings::UiSettings(QObject *parent)
    : QObject(parent)
{
}

QVariant UiSettings::value(const QString &key, const QVariant &fallback) const
{
    return m_settings.value(prefixed(key), fallback);
}

void UiSettings::setValue(const QString &key, const QVariant &value)
{
    m_settings.setValue(prefixed(key), value);
}
