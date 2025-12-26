#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QFont>
#include <QString>
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>
#include <QWidget>
#include <QSpinBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QSettings>
#include <QDialogButtonBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog();

    void loadSettings();
    void saveSettings();

signals:
    void themeChanged(const QString &theme);
    void fontChanged(const QFont &font);
    void fontSizeChanged(int size);
    void autoSaveChanged(bool enabled);
    void autoSaveIntervalChanged(int minutes);

private slots:
    void onThemeChanged(int index);
    void onSelectFont();
    void onFontSizeChanged(int size);
    void onAutoSaveToggled(bool checked);
    void onAutoSaveIntervalChanged(int value);
    void onBrowseWorkDir();
    void onButtonBoxAccepted();

private:
    void setupUi();
    void setupConnections();

    QTabWidget *m_tabWidget;

    // Appearance tab
    QWidget *m_appearanceTab;
    QComboBox *m_themeComboBox;
    QLabel *m_themePreviewLabel;
    QCheckBox *m_darkModeCheckBox;

    // Editor tab
    QWidget *m_editorTab;
    QLabel *m_fontLabel;
    QLabel *m_fontValueLabel;
    QPushButton *m_fontButton;
    QLabel *m_fontSizeLabel;
    QSpinBox *m_fontSizeSpinBox;
    QCheckBox *m_lineNumbersCheckBox;
    QCheckBox *m_autoIndentCheckBox;
    QCheckBox *m_wordWrapCheckBox;

    // General tab
    QWidget *m_generalTab;
    QCheckBox *m_autoSaveCheckBox;
    QSpinBox *m_autoSaveIntervalSpinBox;
    QLabel *m_workDirLabel;
    QLineEdit *m_workDirLineEdit;
    QPushButton *m_browseWorkDirButton;

    // Buttons
    QDialogButtonBox *m_buttonBox;

    // Settings
    QSettings *m_settings;

    // Current settings state
    QString m_currentTheme;
    QFont m_currentFont;
    int m_currentFontSize;
    bool m_autoSaveEnabled;
    int m_autoSaveInterval;
};

#endif // SETTINGSDIALOG_H
