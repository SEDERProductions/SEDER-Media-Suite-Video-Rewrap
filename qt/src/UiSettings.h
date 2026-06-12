#pragma once

#include <QObject>
#include <QSettings>
#include <QVariant>

// Thin QSettings bridge for QML-side UI persistence (split sizes,
// active tabs, window geometry). Kept separate from AppController so
// layout state never mixes with app logic.
class UiSettings : public QObject
{
    Q_OBJECT

public:
    explicit UiSettings(QObject *parent = nullptr);

    Q_INVOKABLE QVariant value(const QString &key, const QVariant &fallback = QVariant()) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);

private:
    mutable QSettings m_settings;
};
