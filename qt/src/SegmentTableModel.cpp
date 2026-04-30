#include "SegmentTableModel.h"

#include "RustBridge.h"

#include <QJsonObject>

SegmentTableModel::SegmentTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int SegmentTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_segments.size();
}

int SegmentTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 6;
}

QVariant SegmentTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_segments.size()) {
        return {};
    }

    const SegmentRow &segment = m_segments.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
        return cellDisplay(segment, index.column());
    case EnabledRole:
        return segment.enabled;
    case NameRole:
        return segment.name;
    case InMsRole:
        return segment.inMs;
    case OutMsRole:
        return segment.outMs;
    case InTextRole:
        return timeText(segment.inMs);
    case OutTextRole:
        return timeText(segment.outMs);
    case DurationTextRole:
        return timeText((segment.outMs - segment.inMs) > 0 ? segment.outMs - segment.inMs : 0);
    case NotesRole:
        return segment.notes;
    default:
        return {};
    }
}

QVariant SegmentTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
    case 0:
        return "On";
    case 1:
        return "Name";
    case 2:
        return "In";
    case 3:
        return "Out";
    case 4:
        return "Duration";
    case 5:
        return "Notes";
    default:
        return {};
    }
}

QHash<int, QByteArray> SegmentTableModel::roleNames() const
{
    return {
        { EnabledRole, "enabled" },
        { NameRole, "name" },
        { InMsRole, "inMs" },
        { OutMsRole, "outMs" },
        { InTextRole, "inText" },
        { OutTextRole, "outText" },
        { DurationTextRole, "durationText" },
        { NotesRole, "notes" },
    };
}

void SegmentTableModel::clear()
{
    if (m_segments.isEmpty()) {
        return;
    }
    beginResetModel();
    m_segments.clear();
    endResetModel();
}

void SegmentTableModel::remove(int row)
{
    if (row < 0 || row >= m_segments.size()) {
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    m_segments.removeAt(row);
    endRemoveRows();
}

void SegmentTableModel::duplicate(int row)
{
    if (row < 0 || row >= m_segments.size()) {
        return;
    }
    SegmentRow copy = m_segments.at(row);
    copy.name = QStringLiteral("%1 copy").arg(copy.name);
    beginInsertRows(QModelIndex(), row + 1, row + 1);
    m_segments.insert(row + 1, copy);
    endInsertRows();
}

void SegmentTableModel::moveUp(int row)
{
    if (row <= 0 || row >= m_segments.size()) {
        return;
    }
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
    m_segments.move(row, row - 1);
    endMoveRows();
}

void SegmentTableModel::moveDown(int row)
{
    if (row < 0 || row + 1 >= m_segments.size()) {
        return;
    }
    beginMoveRows(QModelIndex(), row, row, QModelIndex(), row + 2);
    m_segments.move(row, row + 1);
    endMoveRows();
}

void SegmentTableModel::setEnabled(int row, bool enabled)
{
    if (row < 0 || row >= m_segments.size() || m_segments[row].enabled == enabled) {
        return;
    }
    m_segments[row].enabled = enabled;
    const QModelIndex first = index(row, 0);
    const QModelIndex last = index(row, columnCount() - 1);
    emit dataChanged(first, last, { Qt::DisplayRole, EnabledRole, DurationTextRole });
}

void SegmentTableModel::append(const SegmentRow &segment)
{
    const int row = m_segments.size();
    beginInsertRows(QModelIndex(), row, row);
    m_segments.push_back(segment);
    endInsertRows();
}

void SegmentTableModel::setSegments(const QVector<SegmentRow> &segments)
{
    beginResetModel();
    m_segments = segments;
    endResetModel();
}

QVector<SegmentRow> SegmentTableModel::segments() const
{
    return m_segments;
}

QJsonArray SegmentTableModel::toJsonArray() const
{
    QJsonArray array;
    for (const SegmentRow &segment : m_segments) {
        array.push_back(QJsonObject {
            { "name", segment.name },
            { "in_ms", static_cast<double>(segment.inMs) },
            { "out_ms", static_cast<double>(segment.outMs) },
            { "notes", segment.notes },
            { "enabled", segment.enabled },
        });
    }
    return array;
}

void SegmentTableModel::fromJsonArray(const QJsonArray &array)
{
    QVector<SegmentRow> segments;
    segments.reserve(array.size());
    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        segments.push_back(SegmentRow {
            object.value("name").toString(QStringLiteral("Segment")),
            static_cast<qint64>(object.value("in_ms").toDouble()),
            static_cast<qint64>(object.value("out_ms").toDouble()),
            object.value("notes").toString(),
            object.value("enabled").toBool(true),
        });
    }
    setSegments(segments);
}

QString SegmentTableModel::timeText(qint64 milliseconds) const
{
    const qint64 safe = std::max<qint64>(0, milliseconds);
    const qint64 hours = safe / 3600000;
    const qint64 minutes = (safe % 3600000) / 60000;
    const qint64 seconds = (safe % 60000) / 1000;
    const qint64 millis = safe % 1000;
    return QStringLiteral("%1:%2:%3.%4")
        .arg(hours, 2, 10, QLatin1Char('0'))
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'))
        .arg(millis, 3, 10, QLatin1Char('0'));
}

QString SegmentTableModel::cellDisplay(const SegmentRow &segment, int column) const
{
    switch (column) {
    case 0:
        return segment.enabled ? QStringLiteral("ON") : QStringLiteral("OFF");
    case 1:
        return segment.name;
    case 2:
        return timeText(segment.inMs);
    case 3:
        return timeText(segment.outMs);
    case 4:
        return timeText((segment.outMs - segment.inMs) > 0 ? segment.outMs - segment.inMs : 0);
    case 5:
        return segment.notes;
    default:
        return {};
    }
}
