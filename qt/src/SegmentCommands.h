#pragma once

#include "SegmentTableModel.h"

#include <QString>
#include <QUndoCommand>

// Commands operate on SegmentTableModel and emit a refresh signal via the
// owner pointer so AppController can recompute total duration after undo/redo.
class SegmentCommandContext
{
public:
    virtual ~SegmentCommandContext() = default;
    virtual void onSegmentsMutated() = 0;
};

class AddSegmentCommand : public QUndoCommand
{
public:
    AddSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, const SegmentRow &row);
    void undo() override;
    void redo() override;

private:
    SegmentTableModel *m_model;
    SegmentCommandContext *m_ctx;
    SegmentRow m_row;
    int m_insertedAt = -1;
};

class RemoveSegmentCommand : public QUndoCommand
{
public:
    RemoveSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int row);
    void undo() override;
    void redo() override;

private:
    SegmentTableModel *m_model;
    SegmentCommandContext *m_ctx;
    int m_row;
    SegmentRow m_snapshot;
};

class DuplicateSegmentCommand : public QUndoCommand
{
public:
    DuplicateSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int row);
    void undo() override;
    void redo() override;

private:
    SegmentTableModel *m_model;
    SegmentCommandContext *m_ctx;
    int m_row;
};

class MoveSegmentCommand : public QUndoCommand
{
public:
    MoveSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int from, int to);
    void undo() override;
    void redo() override;

private:
    SegmentTableModel *m_model;
    SegmentCommandContext *m_ctx;
    int m_from;
    int m_to;
};

class ToggleSegmentCommand : public QUndoCommand
{
public:
    ToggleSegmentCommand(SegmentTableModel *model, SegmentCommandContext *ctx, int row, bool enabled);
    void undo() override;
    void redo() override;

private:
    SegmentTableModel *m_model;
    SegmentCommandContext *m_ctx;
    int m_row;
    bool m_newEnabled;
    bool m_previousEnabled;
};
