#include "commandpalette.h"

#include <QKeyEvent>
#include <QListWidgetItem>
#include <QFrame>
#include <QLabel>
#include <QHBoxLayout>
#include <QStyle>
#include <QDebug>

const char *CommandPalette::STYLE_SHEET = R"(
    CommandPalette {
        background-color: #ffffff;
        border: 1px solid #e0e0e0;
        border-radius: 8px;
    }
    CommandPalette QLineEdit {
        border: none;
        border-bottom: 1px solid #e0e0e0;
        padding: 12px 16px;
        font-size: 14px;
        background-color: #ffffff;
        border-radius: 0;
    }
    CommandPalette QLineEdit:focus {
        outline: none;
    }
    CommandPalette QListWidget {
        border: none;
        background-color: #ffffff;
        padding: 8px;
    }
    CommandPalette QListWidget::item {
        padding: 10px 12px;
        border-radius: 4px;
        margin: 1px;
    }
    CommandPalette QListWidget::item:selected {
        background-color: #e3f2fd;
        color: #1976d2;
    }
    CommandPalette QListWidget::item:hover {
        background-color: #f5f5f5;
    }
)";

CommandPalette::CommandPalette(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Command Palette");
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
    setModal(true);
    setStyleSheet(STYLE_SHEET);
    setMinimumWidth(400);
    setMaximumWidth(600);

    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Search input
    searchLineEdit = new QLineEdit(this);
    searchLineEdit->setPlaceholderText("Type a command...");
    searchLineEdit->setStyleSheet(R"(
        QLineEdit {
            border: none;
            border-bottom: 1px solid #e0e0e0;
            padding: 12px 16px;
            font-size: 14px;
        }
        QLineEdit:focus {
            outline: none;
        }
    )");
    mainLayout->addWidget(searchLineEdit);

    // Command list
    commandListWidget = new QListWidget(this);
    commandListWidget->setStyleSheet(R"(
        QListWidget {
            border: none;
            background-color: #ffffff;
            padding: 8px;
        }
        QListWidget::item {
            padding: 10px 12px;
            border-radius: 4px;
            margin: 1px;
        }
        QListWidget::item:selected {
            background-color: #e3f2fd;
            color: #1976d2;
        }
        QListWidget::item:hover {
            background-color: #f5f5f5;
        }
    )");
    commandListWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainLayout->addWidget(commandListWidget);

    // Connect signals
    connect(searchLineEdit, &QLineEdit::textChanged,
            this, &CommandPalette::onTextChanged);
    connect(commandListWidget, &QListWidget::itemActivated,
            this, &CommandPalette::onItemActivated);
}

CommandPalette::~CommandPalette()
{
}

void CommandPalette::addCommand(const QString &name, const QString &description,
                                 const QString &shortcut, std::function<void()> action,
                                 const QString &category)
{
    Command cmd;
    cmd.name = name;
    cmd.description = description;
    cmd.shortcut = shortcut;
    cmd.action = std::move(action);
    cmd.category = category;

    allCommands.append(cmd);
}

void CommandPalette::showPalette()
{
    searchLineEdit->clear();
    filterCommands("");
    searchLineEdit->setFocus();

    // Position at top center of parent
    if (parentWidget()) {
        int x = parentWidget()->geometry().center().x() - width() / 2;
        int y = 100;
        move(parentWidget()->mapToGlobal(QPoint(x, y)));
    }

    exec();
}

void CommandPalette::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
        case Qt::Key_Escape:
            reject();
            break;
        case Qt::Key_Up:
            if (commandListWidget->currentRow() > 0) {
                commandListWidget->setCurrentRow(commandListWidget->currentRow() - 1);
            }
            break;
        case Qt::Key_Down:
            if (commandListWidget->currentRow() < commandListWidget->count() - 1) {
                commandListWidget->setCurrentRow(commandListWidget->currentRow() + 1);
            }
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (commandListWidget->currentItem()) {
                onItemActivated(commandListWidget->currentItem());
            }
            break;
        default:
            QDialog::keyPressEvent(event);
            break;
    }
}

void CommandPalette::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    searchLineEdit->setFocus();
    if (commandListWidget->count() > 0) {
        commandListWidget->setCurrentRow(0);
    }
}

void CommandPalette::onTextChanged(const QString &text)
{
    filterCommands(text);
}

void CommandPalette::onItemActivated(QListWidgetItem *item)
{
    int row = commandListWidget->row(item);
    if (row >= 0 && row < filteredCommands.size()) {
        if (filteredCommands[row].action) {
            filteredCommands[row].action();
        }
    }
    accept();
}

void CommandPalette::filterCommands(const QString &text)
{
    commandListWidget->clear();
    filteredCommands.clear();

    QString lowerText = text.toLower();

    for (const Command &cmd : allCommands) {
        // Filter by name or description
        bool matches = cmd.name.toLower().contains(lowerText) ||
                       cmd.description.toLower().contains(lowerText) ||
                       cmd.category.toLower().contains(lowerText);

        if (matches || text.isEmpty()) {
            filteredCommands.append(cmd);

            // Create list widget item
            QListWidgetItem *item = new QListWidgetItem(commandListWidget);

            // Format: "Command Name    Shortcut"
            QString itemText = cmd.name;
            if (!cmd.shortcut.isEmpty()) {
                itemText += QString("          %1").arg(cmd.shortcut);
            }

            item->setText(itemText);
            item->setToolTip(cmd.description);
        }
    }

    // Resize list to content
    if (commandListWidget->count() > 0) {
        commandListWidget->setMaximumHeight(
            std::min(commandListWidget->count(), 10) * 45 + 16
        );
    } else {
        commandListWidget->setMaximumHeight(100);
    }

    // Select first item
    if (commandListWidget->count() > 0) {
        commandListWidget->setCurrentRow(0);
    }
}
