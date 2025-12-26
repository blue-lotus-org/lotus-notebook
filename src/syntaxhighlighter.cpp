#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , commentLength(0)
    , inMultilineComment(false)
{
    setupRules();
}

void SyntaxHighlighter::setupRules()
{
    // Keyword colors (Lotus theme - calm greens)
    keywordFormat.setForeground(QColor("#2E7D32"));  // Forest green
    keywordFormat.setFontWeight(QFont::Bold);

    // Python keywords
    keywordPatterns << "\\bFalse\\b" << "\\bNone\\b" << "\\bTrue\\b"
                    << "\\band\\b" << "\\bas\\b" << "\\bassert\\b"
                    << "\\basync\\b" << "\\bawait\\b" << "\\bbreak\\b"
                    << "\\bclass\\b" << "\\bcontinue\\b" << "\\bdef\\b"
                    << "\\bdel\\b" << "\\belif\\b" << "\\belse\\b"
                    << "\\bexcept\\b" << "\\bfinally\\b" << "\\bfor\\b"
                    << "\\bfrom\\b" << "\\bglobal\\b" << "\\bif\\b"
                    << "\\bimport\\b" << "\\bin\\b" << "\\bis\\b"
                    << "\\blambda\\b" << "\\bnonlocal\\b" << "\\bnot\\b"
                    << "\\bor\\b" << "\\bpass\\b" << "\\braise\\b"
                    << "\\breturn\\b" << "\\btry\\b" << "\\bwhile\\b"
                    << "\\bwith\\b" << "\\byield\\b";

    for (const QString &pattern : keywordPatterns) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // Function format - blue
    functionFormat.setForeground(QColor("#1976D2"));  // Blue
    functionFormat.setFontItalic(true);

    // Match functions: def name( or decorators @name
    HighlightRule functionRule;
    functionRule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]*(?=\\()");
    functionRule.format = functionFormat;
    highlightingRules.append(functionRule);

    // Decorator format - purple
    decoratorFormat.setForeground(QColor("#7B1FA2"));  // Purple
    decoratorFormat.setFontItalic(true);

    HighlightRule decoratorRule;
    decoratorRule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_]*");
    decoratorRule.format = decoratorFormat;
    highlightingRules.append(decoratorRule);

    // String format - warm brown/orange
    stringFormat.setForeground(QColor("#E65100"));  // Dark orange

    // Double-quoted strings
    HighlightRule stringRule1;
    stringRule1.pattern = QRegularExpression("\"[^\"\\\\]*(?:\\\\.[^\"\\\\]*)*\"");
    stringRule1.format = stringFormat;
    highlightingRules.append(stringRule1);

    // Single-quoted strings
    HighlightRule stringRule2;
    stringRule2.pattern = QRegularExpression("'[^'\\\\]*(?:\\\\.[^'\\\\]*)*'");
    stringRule2.format = stringFormat;
    highlightingRules.append(stringRule2);

    // Triple-quoted strings (multiline)
    HighlightRule stringRule3;
    stringRule3.pattern = QRegularExpression("\"\"\"[^\"\"\\\\]*(?:\\\\.[^\"\"\\\\]*)*\"\"\"");
    stringRule3.format = stringFormat;
    highlightingRules.append(stringRule3);

    HighlightRule stringRule4;
    stringRule4.pattern = QRegularExpression("'''[^'\\\\]*(?:\\\\.[^'\\\\]*)*'''");
    stringRule4.format = stringFormat;
    highlightingRules.append(stringRule4);

    // Number format - teal
    numberFormat.setForeground(QColor("#00796B"));  // Teal

    // Integer numbers
    HighlightRule numberRule1;
    numberRule1.pattern = QRegularExpression("\\b[0-9]+\\b");
    numberRule1.format = numberFormat;
    highlightingRules.append(numberRule1);

    // Float numbers
    HighlightRule numberRule2;
    numberRule2.pattern = QRegularExpression("\\b[0-9]*\\.[0-9]+\\b");
    numberRule2.format = numberFormat;
    highlightingRules.append(numberRule2);

    // Hex numbers
    HighlightRule numberRule3;
    numberRule3.pattern = QRegularExpression("\\b0x[0-9A-Fa-f]+\\b");
    numberRule3.format = numberFormat;
    highlightingRules.append(numberRule3);

    // Comment format - gray (muted)
    commentFormat.setForeground(QColor("#757575"));  // Gray
    commentFormat.setFontItalic(true);

    // Single-line comments
    singleLineCommentFormat.setForeground(QColor("#757575"));
    singleLineCommentFormat.setFontItalic(true);

    HighlightRule singleLineCommentRule;
    singleLineCommentRule.pattern = QRegularExpression("#[^\\n]*");
    singleLineCommentRule.format = singleLineCommentFormat;
    highlightingRules.append(singleLineCommentRule);

    // Multiline comment expressions
    commentStartExpression = QRegularExpression("'''|\"\"\"");
    commentEndExpression = QRegularExpression("'''|\"\"\"");
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Reset multiline comment state at start of block
    if (inMultilineComment) {
        // Continue from where we left off
        setCurrentBlockState(1);
    } else {
        setCurrentBlockState(0);
    }

    // Apply all highlighting rules
    highlightPythonCode(text);

    // Handle multiline comments
    if (currentBlockState() == 1) {
        // We're in a multiline comment
        int startIndex = 0;
        int commentEnd = commentEndExpression.match(text).capturedStart();

        if (commentEnd == -1) {
            // No end found in this block
            setFormat(0, text.length(), commentFormat);
            commentLength = text.length();
        } else {
            // End found
            setFormat(0, commentEnd - startIndex, commentFormat);
            commentLength = 0;
            setCurrentBlockState(0);
        }
    } else {
        // Check if we enter a multiline comment
        int index = 0;
        QRegularExpressionMatch match = commentStartExpression.match(text);

        if (match.hasMatch()) {
            index = match.capturedStart();
            QString captured = match.captured(0);

            if (index > 0) {
                // Check for string starting before comment
                setFormat(0, index, commentFormat);
            }

            // Find the end of the multiline comment
            QRegularExpressionMatch endMatch = commentEndExpression.match(text, index + captured.length());
            int endIndex;

            if (endMatch.hasMatch()) {
                endIndex = endMatch.capturedStart();
                setFormat(index, endIndex - index + captured.length(), commentFormat);
            } else {
                // No end found in this block
                setFormat(index, text.length() - index, commentFormat);
                commentLength = text.length() - index;
                setCurrentBlockState(1);
            }
        }
    }
}

void SyntaxHighlighter::highlightPythonCode(const QString &text)
{
    // Apply all highlighting rules
    for (const HighlightRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);

        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}
