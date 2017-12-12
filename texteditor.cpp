/*
 * This file is part of Ample.
 *
 * Ample is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ample is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ample.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QTextLayout>
#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QString>
#include <QFileInfo>
#include "texteditor.h"

int TextEditor::unnamedCount = 0;

TextEditor::TextEditor(Worksheet *document, QWidget *parent)
    : QTextEdit(parent)
{
    setDocument(document);
    doc = document;
    setAcceptRichText(false);
    //setFrameStyle(QFrame::NoFrame);
    lastBlockCount = doc->blockCount();
    connect(doc, SIGNAL(blockCountChanged(int)), this, SLOT(blockCountChanged(int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(cursorMoved()));
}

TextEditor::~TextEditor()
{
    activeDocuments->removeAction(menuAction);
}

QAction* TextEditor::createMenuAction(int index, QActionGroup *actionGroup)
{
    menuAction = new QAction(this);
    menuAction->setData(index);
    menuAction->setCheckable(true);
    connect(menuAction, SIGNAL(triggered(bool)), this, SLOT(menuActionTriggered(bool)));
    QString text;
    if (doc->isUnnamed())
        text = tr("Unnamed document ") + QString::number(++unnamedCount);
    else
        text = QFileInfo(doc->fileName).baseName();
    menuAction->setText(text);
    activeDocuments = actionGroup;
    activeDocuments->addAction(menuAction);
    return menuAction;
}

void TextEditor::paintEvent(QPaintEvent *event)
{
    QTextEdit::paintEvent(event);
    QTextBlock block = doc->begin();
    QPainter painter(this->viewport());
    painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
    while (block.isValid())
    {
        QTextLayout *layout = block.layout();
        QTextBlockFormat format = block.blockFormat();
        if (format.hasProperty(Worksheet::CounterTypeId) &&
                format.intProperty(Worksheet::CounterTypeId) != Worksheet::None &&
                format.hasProperty(Worksheet::CounterTagId))
        {
            QFont font = doc->counterFont(format.intProperty(Worksheet::CounterTypeId));
            QFontMetrics fontMetrics(font);
            qreal x = layout->position().x() - doc->paragraphMargin;
            qreal y = layout->position().y() + fontMetrics.leading() + layout->lineAt(0).ascent() + 1;
            QString tag = format.stringProperty(Worksheet::CounterTagId);
            QPointF where(x,y);
            painter.setFont(font);
            painter.drawText(where, tag);
        }
        block = block.next();
    }
}

void TextEditor::keyPressEvent(QKeyEvent *event)
{
    if (!event->matches(QKeySequence::Copy) && event->text().length() > 0)
        return;
    QTextCursor cursor = textCursor();
    bool isShiftPressed = event->modifiers().testFlag(Qt::ShiftModifier);
    //bool isCtrlPressed = event->modifiers().testFlag(Qt::ControlModifier);
    //bool isAltPressed = event->modifiers().testFlag(Qt::AltModifier);
    if (!cursor.hasSelection()) {
        QTextBlock block = cursor.block();
        QTextBlockFormat blockFormat = block.blockFormat();
        bool isParagraph = blockFormat.intProperty(Worksheet::CounterTypeId) == Worksheet::None;
        switch (event->key())
        {
        case Qt::Key_Delete:
        case Qt::Key_Backspace:
            if (!isParagraph && (isShiftPressed || block.text().length() == 0))
            {
                doc->blockToParagraph(cursor);
                doc->updateEnumeration();
                return;
            }
            break;
        default:
            break;
        }
    }
    QTextEdit::keyPressEvent(event);
}

void TextEditor::blockCountChanged(int count)
{
    QTextCursor cursor = textCursor();
    if (cursor.currentFrame() != doc->rootFrame())
        return;
    if (count < lastBlockCount && Worksheet::isHeading(cursor.block()))
    {
        Worksheet::applyFormatToBlock(cursor, cursor.charFormat(), false);
    }
    else if (count > lastBlockCount)
    {
        int n = count - lastBlockCount, m = 0;
        QTextBlock block = cursor.block();
        for (int i = 0; i < n && block.isValid(); ++i)
        {
            block = block.previous();
            ++m;
        }
        if (m == n && Worksheet::isHeading(block))
        {
            QTextCursor tempCursor(doc);
            tempCursor.setPosition(block.position());
            for (int i = 0; i < n; ++i)
            {
                tempCursor.movePosition(QTextCursor::NextBlock);
                doc->blockToParagraph(tempCursor);
            }
        }
    }
    lastBlockCount = count;
    doc->updateEnumeration();
}

void TextEditor::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textCursor();
    if (!cursor.hasSelection() && !cursorAtEndOfWord())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    mergeCurrentCharFormat(format);
}

void TextEditor::paragraphStyleChanged(int type)
{
    bool isList = type == Worksheet::List || type == Worksheet::NumberedList;
    QTextCursor cursor = textCursor();
    QTextBlockFormat blockFormat = cursor.blockFormat();
    QTextCharFormat charFormat;
    if (cursor.currentFrame() != doc->rootFrame() ||
            blockFormat.intProperty(Worksheet::ParagraphTypeId) == type)
        return;
    switch (type)
    {
    case Worksheet::TextBody:
        doc->blockToParagraph(cursor);
        break;
    case Worksheet::Title:
        charFormat.setFont(doc->titleFont);
        blockFormat.setProperty(Worksheet::CounterTypeId, Worksheet::None);
        blockFormat.setTextIndent(0.0);
        break;
    case Worksheet::Section:
        charFormat.setFont(doc->sectionFont);
        blockFormat.setProperty(Worksheet::CounterTypeId, Worksheet::SectionCounter);
        break;
    case Worksheet::Subsection:
        charFormat.setFont(doc->subsectionFont);
        blockFormat.setProperty(Worksheet::CounterTypeId, Worksheet::SubsectionCounter);
        break;
    default:
        break;
    }
    if (!isList && type != Worksheet::TextBody)
    {
        cursor.setBlockCharFormat(charFormat);
        blockFormat.setProperty(Worksheet::ParagraphTypeId, type);
        cursor.setBlockFormat(blockFormat);
        doc->applyFormatToBlock(cursor, charFormat, true);
    }
    doc->updateEnumeration();
}

void TextEditor::cursorMoved()
{
    /*
    if (textCursor().atStart() && doc->hasTitleFrame())
        setTextCursor(doc->firstTitleCursorPosition());
    */
}

bool TextEditor::cursorAtEndOfWord()
{
    QTextCursor cursor = textCursor();
    QTextCursor pCursor(cursor);
    pCursor.movePosition(QTextCursor::EndOfWord);
    return cursor.position() == pCursor.position();
}

void TextEditor::menuActionTriggered(bool active)
{
    if (active)
        emit focusRequested(menuAction->data().toInt());
}
