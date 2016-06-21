#include "dfileitemdelegate.h"
#include "fileitem.h"
#include "dfilesystemmodel.h"

#include "../app/global.h"

#include <QLabel>
#include <QPainter>
#include <QTextEdit>
#include <QLineEdit>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>

#define ICON_SPACING 10
#define COLUMU_PADDING 10
#define LEFT_PADDING 10
#define RIGHT_PADDING 10
#define ICON_MODE_RECT_RADIUS 4
#define LIST_MODE_RECT_RADIUS 2
#define LIST_MODE_LEFT_MARGIN 20
#define LIST_MODE_RIGHT_MARGIN 20
#define SELECTED_BACKGROUND_COLOR "#2da6f7"

DFileItemDelegate::DFileItemDelegate(DFileView *parent) :
    QStyledItemDelegate(parent)
{
    focus_item = new FileIconItem(parent->viewport());
    focus_item->setAttribute(Qt::WA_TransparentForMouseEvents);
    focus_item->setProperty("showBackground", true);
    focus_item->edit->setReadOnly(true);
    focus_item->canDeferredDelete = false;
    focus_item->icon->setFixedSize(parent->iconSize());
    /// prevent flash when first call show()
    focus_item->setFixedWidth(0);

    connect(parent, &DListView::triggerEdit,
            this, [this, parent](const QModelIndex &index) {
        if(index == focus_index) {
            parent->setIndexWidget(index, 0);
            focus_item->hide();
            focus_index = QModelIndex();
            parent->edit(index);
        }
    });

    connect(parent, &DListView::iconSizeChanged,
            this, [this] {
        m_elideMap.clear();
        m_wordWrapMap.clear();
        m_textHeightMap.clear();
    });
}

DFileItemDelegate::~DFileItemDelegate()
{
    focus_item->canDeferredDelete = true;
}

void DFileItemDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    /// judgment way of the whether drag model(another way is: painter.devType() != 1)
    bool isDragMode = ((QPaintDevice*)parent()->viewport() != painter->device());

    if(parent()->isIconViewMode()) {
        paintIconItem(isDragMode, painter, option, index);
    } else {
        paintListItem(isDragMode, painter, option, index);
    }
}

QSize DFileItemDelegate::sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const
{
    return parent()->itemSizeHint();
}

QWidget *DFileItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    editing_index = index;

    if (this->parent()->isIconViewMode()) {
       FileIconItem *item = new FileIconItem(parent);


       connect(item, &FileIconItem::destroyed, this, [this] {
           editing_index = QModelIndex();
       });

       return item;
   } else {
       QLineEdit *edit = new QLineEdit(parent);

       connect(edit, &QLineEdit::destroyed, this, [this] {
           editing_index = QModelIndex();
       });

       edit->setFrame(false);
       edit->setAttribute(Qt::WA_TranslucentBackground);
       edit->setContentsMargins(-3, 0, 0, 0);

       return edit;
   }
}

void DFileItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(parent()->isIconViewMode()) {
        editor->move(option.rect.topLeft());
        editor->setFixedWidth(option.rect.width());
        editor->setMinimumHeight(option.rect.height());

        FileIconItem *item = qobject_cast<FileIconItem*>(editor);

        if(!item)
            return;

        if(parent()->iconSize().width() != item->icon->size().width()) {
            QStyleOptionViewItem opt;

            initStyleOption(&opt, index);

            item->icon->setFixedSize(parent()->iconSize());
            item->icon->setPixmap(opt.icon.pixmap(parent()->iconSize(), QIcon::Selected));
        }
    } else {
        const QList<int> &columnRoleList = parent()->columnRoleList();

        int column_x = 0;

        /// draw icon

        const QRect &opt_rect = option.rect + QMargins(-LIST_MODE_LEFT_MARGIN, 0, -LIST_MODE_RIGHT_MARGIN, 0);

        QRect icon_rect = opt_rect;

        icon_rect.setSize(parent()->iconSize());

        column_x = icon_rect.right() + ICON_SPACING;

        QRect rect = opt_rect;

        rect.setLeft(column_x + LEFT_PADDING);

        column_x = parent()->columnWidth(0) - parent()->viewportMargins().left();

        rect.setRight(column_x);

        editor->setGeometry(rect);

        column_x += 5;

        for(int i = 1; i < columnRoleList.count(); ++i) {
            QRect rect = opt_rect;

            rect.setLeft(column_x);

            column_x += parent()->columnWidth(i);

            rect.setRight(column_x);
        }
    }
}

void DFileItemDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if(parent()->isIconViewMode()) {
        FileIconItem *item = qobject_cast<FileIconItem*>(editor);

        if(!item)
            return;

        QStyleOptionViewItem opt;

        initStyleOption(&opt, index);

        item->icon->setPixmap(opt.icon.pixmap(parent()->iconSize(), QIcon::Selected));
        item->edit->setPlainText(index.data().toString());
        item->edit->setAlignment(Qt::AlignHCenter);
        item->edit->document()->setTextWidth(parent()->iconSize().width() * 1.8);
        item->edit->setFixedSize(item->edit->document()->size().toSize());

        if(item->edit->isReadOnly())
            return;

        int endPos = item->edit->toPlainText().lastIndexOf('.');

        if(endPos == -1) {
            item->edit->selectAll();
        } else {
            QTextCursor cursor = item->edit->textCursor();

            cursor.setPosition(0);
            cursor.setPosition(endPos, QTextCursor::KeepAnchor);

            item->edit->setTextCursor(cursor);
        }
    } else {
        QLineEdit *edit = qobject_cast<QLineEdit*>(editor);

        if(!edit)
            return;

        const QString &text = index.data(DFileSystemModel::FileNameRole).toString();

        edit->setText(text);
    }
}

void DFileItemDelegate::paintIconItem(bool isDragMode, QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    if((index == focus_index || index == editing_index) && !isDragMode)
        return;

    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    /// init icon geomerty

    QRect icon_rect = opt.rect;

    icon_rect.setSize(parent()->iconSize());
    icon_rect.moveLeft(opt.rect.left() + (opt.rect.width() - icon_rect.width()) / 2.0);
    icon_rect.moveTop(opt.rect.top());

    QString str = opt.text;

    if(str.isEmpty()) {
        /// draw icon

        opt.icon.paint(painter, icon_rect);

        return;
    }

    /// init file name geometry

    QRect label_rect = opt.rect;

    label_rect.setTop(icon_rect.bottom() + TEXT_PADDING);
    label_rect.setWidth(opt.rect.width() - 2 * TEXT_PADDING);
    label_rect.moveLeft(label_rect.left() + TEXT_PADDING);

    /// if has focus show all file name else show elide file name.

    if((opt.state & QStyle::State_HasFocus) && parent()->selectedIndexCount() < 2) {
        if(focus_index.isValid()) {
            parent()->setIndexWidget(focus_index, 0);
            focus_item->hide();
            focus_index = QModelIndex();
        }

        int height = 0;

        /// init file name text

        if(m_wordWrapMap.contains(str)) {
            str = m_wordWrapMap.value(str);
            height = m_textHeightMap.value(str);
        } else {
            QString wordWrap_str = Global::wordWrapText(str, label_rect.width(),
                                                        QTextOption::WrapAtWordBoundaryOrAnywhere,
                                                        &height);

            m_wordWrapMap[str] = wordWrap_str;
            m_textHeightMap[wordWrap_str] = height;
            str = wordWrap_str;
        }

        if(height > label_rect.height()) {
            /// use widget(FileIconItem) show file icon and file name label.

            focus_index = index;

            setEditorData(focus_item, index);
            parent()->setIndexWidget(index, focus_item);

            return;
        }
    } else {
        /// init file name text

        if(m_elideMap.contains(str)) {
            str = m_elideMap.value(str);
        } else {
            QString elide_str = Global::elideText(str, label_rect.size(),
                                                  opt.fontMetrics,
                                                  QTextOption::WrapAtWordBoundaryOrAnywhere,
                                                  opt.textElideMode);

            m_elideMap[str] = elide_str;

            str = elide_str;
        }
    }

    /// draw background

    bool drawBackground = !isDragMode && (opt.state & QStyle::State_Selected) && opt.showDecorationSelected;

    if (drawBackground) {
        painter->setPen(Qt::white);
        opt.icon.paint(painter, icon_rect, Qt::AlignCenter, QIcon::Selected);
    } else {
        painter->setPen(Qt::black);
        opt.icon.paint(painter, icon_rect);
    }

    /// draw file name label

    if(str.indexOf("\n") >=0 && !str.endsWith("\n")) {
        QTextDocument *doc = m_documentMap.value(str);

        if(!doc) {
            doc = new QTextDocument(str, const_cast<DFileItemDelegate*>(this));

            QTextCursor cursor(doc);
            QTextOption text_option(Qt::AlignHCenter);

            text_option.setWrapMode(QTextOption::NoWrap);
//            doc->setDefaultFont(painter->font());
            doc->setDefaultTextOption(text_option);
            doc->setTextWidth(label_rect.width());
            cursor.movePosition(QTextCursor::Start);

            do {
                QTextBlockFormat format = cursor.blockFormat();

                format.setLineHeight(TEXT_LINE_HEIGHT, QTextBlockFormat::FixedHeight);
                cursor.setBlockFormat(format);
            } while (cursor.movePosition(QTextCursor::NextBlock));

            m_documentMap[str] = doc;
        }

        QAbstractTextDocumentLayout::PaintContext ctx;

        ctx.palette.setColor(QPalette::Text, painter->pen().color());

        if (drawBackground) {
            QRect rect = opt.rect;

            rect.moveTop(label_rect.top() - TEXT_PADDING);
            rect.setHeight(doc->size().height());

            QPainterPath path;

            path.addRoundedRect(rect, ICON_MODE_RECT_RADIUS, ICON_MODE_RECT_RADIUS);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->fillPath(path, QColor(SELECTED_BACKGROUND_COLOR));
            painter->restore();
        } else {
            painter->fillRect(label_rect, Qt::transparent);
        }

        painter->save();
        painter->translate(label_rect.left(), label_rect.top() - TEXT_PADDING);
        doc->documentLayout()->draw(painter, ctx);
        painter->restore();
    } else {
        if(drawBackground) {
            QRect rect;

            painter->drawText(label_rect, Qt::AlignHCenter, str, &rect);
            rect += QMargins(TEXT_PADDING, TEXT_PADDING, TEXT_PADDING, TEXT_PADDING);

            QPainterPath path;

            path.addRoundedRect(rect, ICON_MODE_RECT_RADIUS, ICON_MODE_RECT_RADIUS);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->fillPath(path, QColor(SELECTED_BACKGROUND_COLOR));
            painter->restore();
        } else {
            painter->fillRect(label_rect, Qt::transparent);
        }

        painter->drawText(label_rect, Qt::AlignHCenter, str);
    }
}

void DFileItemDelegate::paintListItem(bool isDragMode, QPainter *painter,
                                      const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    const QList<int> &columnRoleList = parent()->columnRoleList();

    int column_x = 0;

    QStyleOptionViewItem opt = option;

    initStyleOption(&opt, index);

    opt.rect += QMargins(-LIST_MODE_LEFT_MARGIN, 0, -LIST_MODE_RIGHT_MARGIN, 0);

    /// draw background

    bool drawBackground = !isDragMode && (opt.state & QStyle::State_Selected) && opt.showDecorationSelected;

    if (drawBackground) {
        QPainterPath path;

        path.addRoundedRect(opt.rect, LIST_MODE_RECT_RADIUS, LIST_MODE_RECT_RADIUS);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(path, QColor(SELECTED_BACKGROUND_COLOR));
        painter->restore();
        painter->setPen(Qt::white);
    } else {
        painter->setPen(Qt::black);
    }

    opt.rect.setLeft(opt.rect.left() + LEFT_PADDING);
    opt.rect.setRight(opt.rect.right() + RIGHT_PADDING);

    /// draw icon

    QRect icon_rect = opt.rect;

    icon_rect.setSize(parent()->iconSize());
    icon_rect.moveTop(icon_rect.top() + (opt.rect.bottom() - icon_rect.bottom()) / 2);
    opt.icon.paint(painter, icon_rect);

    column_x = icon_rect.right() + ICON_SPACING;

    QRect rect = opt.rect;

    rect.setLeft(column_x);

    column_x = parent()->columnWidth(0) - parent()->viewportMargins().left();

    rect.setRight(column_x);

    int role = columnRoleList.at(0);

    if(index != editing_index || role != DFileSystemModel::FileNameRole) {
    /// draw file name label

        painter->drawText(rect, Qt::Alignment(index.data(Qt::TextAlignmentRole).toInt()),
                      index.data(role).toString());
    }

    if(isDragMode)
        return;

    const DFileSystemModel *model = qobject_cast<const DFileSystemModel*>(index.model());

    if (drawBackground) {
        painter->setPen("#e9e9e9");
    } else {
        painter->setPen("#797979");
    }

    for(int i = 1; i < columnRoleList.count(); ++i) {
        int column_width = parent()->columnWidth(i);

        if (column_width <= 0)
            continue;

        QRect rect = opt.rect;

        rect.setLeft(column_x + COLUMU_PADDING);

        column_x += parent()->columnWidth(i);

        rect.setRight(column_x - (i < columnRoleList.count() - 1 ? COLUMU_PADDING : 0));

        int role = columnRoleList.at(i);

        if(index != editing_index || role != DFileSystemModel::FileNameRole) {
            /// draw file name label

            QModelIndex tmp_index = model->createIndex(index.row(), model->roleToColumn(role), index.internalId());

            painter->drawText(rect, Qt::Alignment(tmp_index.data(Qt::TextAlignmentRole).toInt()),
                              index.data(role).toString());
        }
    }
}

QList<QRect> DFileItemDelegate::paintGeomertyss(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QList<QRect> geomertys;

    if(parent()->isIconViewMode()) {
        /// init icon geomerty

        QRect icon_rect = option.rect;

        icon_rect.setSize(parent()->iconSize());
        icon_rect.moveCenter(option.rect.center());
        icon_rect.moveTop(option.rect.top());

        geomertys << icon_rect;

        QString str = index.data(Qt::DisplayRole).toString();

        if(str.isEmpty()) {
            return geomertys;
        }

        /// init file name geometry

        QRect label_rect = option.rect;

        label_rect.setTop(icon_rect.bottom() + 10);

        /// if has focus show all file name else show elide file name.
        /// init file name text

        if(m_elideMap.contains(str)) {
            str = m_elideMap.value(str);
        } else {
            QString elide_str = Global::elideText(str, label_rect.size(),
                                                  option.fontMetrics,
                                                  QTextOption::WrapAtWordBoundaryOrAnywhere,
                                                  option.textElideMode);

            m_elideMap[str] = elide_str;

            str = elide_str;
        }

        /// draw icon and file name label

        geomertys << option.fontMetrics.boundingRect(label_rect, Qt::AlignHCenter, str);
    } else {
        const QList<int> &columnRoleList = parent()->columnRoleList();

        int column_x = 0;

        /// draw icon

        QRect icon_rect = option.rect;

        icon_rect.setSize(parent()->iconSize());

        geomertys << icon_rect;

        column_x = icon_rect.right() + ICON_SPACING;

        QRect rect = option.rect;

        rect.setLeft(column_x);

        column_x = parent()->columnWidth(0);

        rect.setRight(column_x);

        int role = columnRoleList.at(0);

        /// draw file name label

        geomertys << option.fontMetrics.boundingRect(rect, Qt::Alignment(index.data(Qt::TextAlignmentRole).toInt()),
                                                     index.data(role).toString());
        for(int i = 1; i < columnRoleList.count(); ++i) {
            QRect rect = option.rect;

            rect.setLeft(column_x);

            column_x += parent()->columnWidth(i);

            rect.setRight(column_x);

            int role = columnRoleList.at(i);

            /// draw file name label

            geomertys << option.fontMetrics.boundingRect(rect, Qt::Alignment(index.data(Qt::TextAlignmentRole).toInt()),
                                                         index.data(role).toString());
        }
    }

    return geomertys;
}

void DFileItemDelegate::hideAllIIndexWidget()
{
    if(focus_index.isValid()) {
        parent()->setIndexWidget(focus_index, 0);
        focus_item->hide();
        focus_index = QModelIndex();
    }

    if(editing_index.isValid()) {
        parent()->setIndexWidget(editing_index, 0);
    }
}

void DFileItemDelegate::commitDataAndCloseActiveEditor()
{
    QWidget *editor = parent()->indexWidget(editing_index);

    QMetaObject::invokeMethod(this, "_q_commitDataAndCloseEditor",
                              Qt::QueuedConnection, Q_ARG(QWidget*, editor));
}

QModelIndex DFileItemDelegate::editingIndex() const
{
    return editing_index;
}

bool DFileItemDelegate::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::Show) {
        QLineEdit *edit = qobject_cast<QLineEdit*>(object);

        if(edit) {
            int endPos = edit->text().lastIndexOf('.');

            if(endPos == -1)
                edit->selectAll();
            else
                edit->setSelection(0, endPos);
        }
    }

    return QStyledItemDelegate::eventFilter(object, event);
}

void DFileItemDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);

    if (parent()->isSelected(index))
        option->state |= QStyle::State_Selected;
    else
        option->state &= QStyle::State_Selected;
}
