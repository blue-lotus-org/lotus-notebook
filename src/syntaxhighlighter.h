#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

protected:
    void highlightBlock(const QString &text) override;

private:
    void setupRules();
    void highlightPythonCode(const QString &text);
    void highlightComments(const QString &text);
    void highlightStrings(const QString &text);
    void highlightNumbers(const QString &text);
    void highlightFunctions(const QString &text);
    void highlightKeywords(const QString &text);

    // Highlight rule structure
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    QList<HighlightRule> highlightingRules;

    // Keyword patterns
    QStringList keywordPatterns;

    // Built-in function patterns
    QStringList functionPatterns;

    // Python built-in types and constants
    QStringList builtinPatterns;

    // Character formats
    QTextCharFormat keywordFormat;
    QTextCharFormat classFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat variableFormat;
    QTextCharFormat stringFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat decoratorFormat;
    QTextCharFormat singleLineCommentFormat;

    // Multiline comment handling
    QRegularExpression commentStartExpression;
    QRegularExpression commentEndExpression;
    int commentLength;
    bool inMultilineComment;
};

#endif // SYNTAXHIGHLIGHTER_H
