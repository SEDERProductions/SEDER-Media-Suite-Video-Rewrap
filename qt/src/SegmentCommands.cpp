#include "SegmentCommands.h"

AddSegmentCommand::AddSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, const SegmentRow &row)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
{
    setText(QObject::tr("Add segment"));
}

void AddSegmentCommand::redo()
{
    m_insertedAt = m_model->rowCount();
    m_model->append(m_row);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void AddSegmentCommand::undo()
{
    if (m_insertedAt < 0) return;
    m_model->remove(m_insertedAt);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

RemoveSegmentCommand::RemoveSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int row)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
{
    setText(QObject::tr("Remove segment"));
    if (m_row >= 0 && m_row < m_model->rowCount()) {
        m_snapshot = m_model->segments().at(m_row);
    }
}

void RemoveSegmentCommand::redo()
{
    m_model->remove(m_row);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void RemoveSegmentCommand::undo()
{
    QVector<SegmentRow> segments = m_model->segments();
    if (m_row < 0 || m_row > segments.size()) return;
    segments.insert(m_row, m_snapshot);
    m_model->setSegments(segments);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

DuplicateSegmentCommand::DuplicateSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int row)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
{
    setText(QObject::tr("Duplicate segment"));
}

void DuplicateSegmentCommand::redo()
{
    m_model->duplicate(m_row);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void DuplicateSegmentCommand::undo()
{
    if (m_row < 0) return;
    m_model->remove(m_row + 1);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

MoveSegmentCommand::MoveSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int from, int to)
    : m_model(model)
    , m_ctx(ctx)
    , m_from(from)
    , m_to(to)
{
    setText(QObject::tr("Move segment"));
}

void MoveSegmentCommand::redo()
{
    if (m_to > m_from) {
        m_model->moveDown(m_from);
    } else if (m_to < m_from) {
        m_model->moveUp(m_from);
    }
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void MoveSegmentCommand::undo()
{
    if (m_to > m_from) {
        m_model->moveUp(m_to);
    } else if (m_to < m_from) {
        m_model->moveDown(m_to);
    }
    if (m_ctx) m_ctx->onSegmentsMutated();
}

ToggleSegmentCommand::ToggleSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int row, bool enabled)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
    , m_newEnabled(enabled)
    , m_previousEnabled(!enabled)
{
    setText(QObject::tr("Toggle segment"));
    if (m_row >= 0 && m_row < m_model->rowCount()) {
        m_previousEnabled = m_model->segments().at(m_row).enabled;
    }
}

void ToggleSegmentCommand::redo()
{
    m_model->setEnabled(m_row, m_newEnabled);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void ToggleSegmentCommand::undo()
{
    m_model->setEnabled(m_row, m_previousEnabled);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

RenameSegmentCommand::RenameSegmentCommand(
    SegmentTableModel *model, SegmentCommandContext *ctx, int row, const QString &name)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
    , m_newName(name)
{
    setText(QObject::tr("Rename segment"));
    if (m_row >= 0 && m_row < m_model->rowCount()) {
        m_previousName = m_model->segments().at(m_row).name;
    }
}

void RenameSegmentCommand::redo()
{
    m_model->setName(m_row, m_newName);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void RenameSegmentCommand::undo()
{
    m_model->setName(m_row, m_previousName);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

SetSegmentNotesCommand::SetSegmentNotesCommand(
    SegmentTableModel *model, SegmentCommandContext *ctx, int row, const QString &notes)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
    , m_newNotes(notes)
{
    setText(QObject::tr("Edit segment notes"));
    if (m_row >= 0 && m_row < m_model->rowCount()) {
        m_previousNotes = m_model->segments().at(m_row).notes;
    }
}

void SetSegmentNotesCommand::redo()
{
    m_model->setNotes(m_row, m_newNotes);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void SetSegmentNotesCommand::undo()
{
    m_model->setNotes(m_row, m_previousNotes);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

SetSegmentBoundsCommand::SetSegmentBoundsCommand(
    SegmentTableModel *model, SegmentCommandContext *ctx, int row, qint64 inMs, qint64 outMs)
    : m_model(model)
    , m_ctx(ctx)
    , m_row(row)
    , m_newInMs(inMs)
    , m_newOutMs(outMs)
{
    setText(QObject::tr("Trim segment"));
    if (m_row >= 0 && m_row < m_model->rowCount()) {
        const SegmentRow &segment = m_model->segments().at(m_row);
        m_previousInMs = segment.inMs;
        m_previousOutMs = segment.outMs;
    }
}

void SetSegmentBoundsCommand::redo()
{
    m_model->setBounds(m_row, m_newInMs, m_newOutMs);
    if (m_ctx) m_ctx->onSegmentsMutated();
}

void SetSegmentBoundsCommand::undo()
{
    m_model->setBounds(m_row, m_previousInMs, m_previousOutMs);
    if (m_ctx) m_ctx->onSegmentsMutated();
}
