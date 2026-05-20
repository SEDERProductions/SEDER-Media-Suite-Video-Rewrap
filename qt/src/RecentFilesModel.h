#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>

// Tracks the N most recently used files of a given category (source, output,
// or project). Persists to QSettings under the supplied key so the list
// survives restarts. Newest entries appear first; duplicates are de-duplicated
// on insert.
class RecentFilesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY changed)

public:
    static constexpr int kMaxEntries = 10;

    enum Roles {
        PathRole = Qt::UserRole + 1,
        DisplayRole,
    };

    explicit RecentFilesModel(const QString &settingsKey, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int count() const { return m_paths.size(); }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList paths() const { return m_paths; }

    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE void prepend(const QString &path);
    Q_INVOKABLE void remove(const QString &path);
    Q_INVOKABLE void clear();

signals:
    void changed();

private:
    void persist() const;
    void load();

    QString m_settingsKey;
    QStringList m_paths;
};
