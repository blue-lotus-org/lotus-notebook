#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QApplication>
#include <QPalette>
#include <QFont>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum Theme {
        Light,
        Dark,
        System
    };

    explicit ThemeManager(QObject *parent = nullptr);
    ~ThemeManager();

    // Theme management
    static ThemeManager* instance();
    void applyTheme(Theme theme);
    Theme currentTheme() const { return m_currentTheme; }

    // Palette and style
    QPalette getPalette(Theme theme) const;
    QString getStylesheet(Theme theme) const;

    // Font management
    QFont getEditorFont() const { return m_editorFont; }
    QFont getUiFont() const { return m_uiFont; }
    void setEditorFont(const QFont &font);
    void setUiFont(const QFont &font);
    int getEditorFontSize() const { return m_editorFont.pointSize(); }
    void setEditorFontSize(int size);

    // Settings persistence
    void saveSettings();
    void loadSettings();

signals:
    void themeChanged(Theme theme);
    void fontChanged(const QFont &editorFont, const QFont &uiFont);

private:
    static ThemeManager* m_instance;
    Theme m_currentTheme;
    QFont m_editorFont;
    QFont m_uiFont;

    void buildDarkPalette();
    void buildLightPalette();
    QString generateDarkStylesheet() const;
    QString generateLightStylesheet() const;
};

#endif // THEMEMANAGER_H
