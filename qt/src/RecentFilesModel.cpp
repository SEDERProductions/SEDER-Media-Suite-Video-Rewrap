#include "RecentFilesModel.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

RecentFilesModel::RecentFilesModel(const QString &settingsKey, QObject *parent)
    : QAbstractListModel(parent)
    , m_settingsKey(settingsKey)
{
    load();
}

int RecentFilesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_paths.size();
}

QVariant RecentFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_paths.size()) {
        return {};
    }
    const QString &path = m_paths.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case DisplayRole:
        return QFileInfo(path).fileName();
    case PathRole:
        return QDir::toNativeSeparators(path);
    default:
        return {};
    }
}

QHash<int, QByteArray> RecentFilesModel::roleNames() const
{
    return {
        { PathRole, "path" },
        { DisplayRole, "display" },
    };
}

QString RecentFilesModel::pathAt(int row) const
{
    if (row < 0 || row >= m_paths.size()) {
        return {};
    }
    return m_paths.at(row);
}

void RecentFilesModel::prepend(const QString &path)
{
    if (path.isEmpty()) return;

    const int existing = m_paths.indexOf(path);
    if (existing == 0) {
        return;
    }
    beginResetModel();
    if (existing > 0) {
        m_paths.removeAt(existing);
    }
    m_paths.prepend(path);
    while (m_paths.size() > kMaxEntries) {
        m_paths.removeLast();
    }
    endResetModel();
    persist();
    emit changed();
}

void RecentFilesModel::remove(const QString &path)
{
    const int idx = m_paths.indexOf(path);
    if (idx < 0) return;
    beginRemoveRows(QModelIndex(), idx, idx);
    m_paths.removeAt(idx);
    endRemoveRows();
    persist();
    emit changed();
}

void RecentFilesModel::clear()
{
    if (m_paths.isEmpty()) return;
    beginResetModel();
    m_paths.clear();
    endResetModel();
    persist();
    emit changed();
}

void RecentFilesModel::load()
{
    QSettings settings;
    m_paths = settings.value(m_settingsKey).toStringList();
    while (m_paths.size() > kMaxEntries) {
        m_paths.removeLast();
    }
}

void RecentFilesModel::persist() const
{
    QSettings settings;
    settings.setValue(m_settingsKey, m_paths);
}
