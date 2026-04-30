#pragma once

#include <QAbstractTableModel>
#include <QJsonArray>
#include <QVector>

struct SegmentRow {
    QString name;
    qint64 inMs = 0;
    qint64 outMs = 0;
    QString notes;
    bool enabled = true;
};

class SegmentTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Roles {
        EnabledRole = Qt::UserRole + 1,
        NameRole,
        InMsRole,
        OutMsRole,
        InTextRole,
        OutTextRole,
        DurationTextRole,
        NotesRole,
    };

    explicit SegmentTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void duplicate(int row);
    Q_INVOKABLE void moveUp(int row);
    Q_INVOKABLE void moveDown(int row);
    Q_INVOKABLE void setEnabled(int row, bool enabled);

    void append(const SegmentRow &segment);
    void setSegments(const QVector<SegmentRow> &segments);
    QVector<SegmentRow> segments() const;
    QJsonArray toJsonArray() const;
    void fromJsonArray(const QJsonArray &array);

private:
    QString timeText(qint64 milliseconds) const;
    QString cellDisplay(const SegmentRow &segment, int column) const;

    QVector<SegmentRow> m_segments;
};
