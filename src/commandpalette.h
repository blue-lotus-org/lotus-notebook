#ifndef COMMANDPALETTE_H
#define COMMANDPALETTE_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QList>
#include <functional>

class CommandPalette : public QDialog
{
    Q_OBJECT

public:
    struct Command {
        QString name;
        QString description;
        QString shortcut;
        std::function<void()> action;
        QString category;
    };

    explicit CommandPalette(QWidget *parent = nullptr);
    ~CommandPalette();

    void addCommand(const QString &name, const QString &description,
                    const QString &shortcut, std::function<void()> action,
                    const QString &category = "General");

    void showPalette();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onTextChanged(const QString &text);
    void onItemActivated(QListWidgetItem *item);

private:
    void filterCommands(const QString &text);
    void updateListVisibility();

    QLineEdit *searchLineEdit;
    QListWidget *commandListWidget;
    QVBoxLayout *mainLayout;

    QList<Command> allCommands;
    QList<Command> filteredCommands;

    // Style sheet for the palette
    static const char *STYLE_SHEET;
};

#endif // COMMANDPALETTE_H
