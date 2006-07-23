
#include "highlighter.h"

// static
QVector<QTextCharFormat> Highlighter::s_formats;

Highlighter::Highlighter(QTextDocument* parent): QSyntaxHighlighter(parent)
{
	if (s_formats.size() == 0)
		createFormats();
}

void Highlighter::setRules(const QList<Rule>& rules)
{
	m_rules = rules;
}

void Highlighter::setMultiLineCommentStart(const QString& s)
{
	m_multiLineCommentStart = s;
}

void Highlighter::setMultiLineCommentEnd(const QString& s)
{
	m_multiLineCommentEnd = s;
}

// static
void Highlighter::createFormats()
{
	s_formats.clear();
	s_formats.resize(FormatType_Max);

	QTextCharFormat format;
	//format.setFont(QFont("Monospace", 8));

	format.setForeground(Qt::darkBlue);
	format.setFontWeight(QFont::Bold);
	s_formats[Keyword] = format;
	format.setFontWeight(QFont::Normal);

	format.setForeground(Qt::darkRed);
	s_formats[DataType] = format;

	format.setForeground(Qt::darkBlue);
	s_formats[BuiltinVar] = format;

	format.setForeground(Qt::darkBlue);
	format.setFontWeight(QFont::Bold);
	s_formats[BuiltinFunction] = format;
	format.setFontWeight(QFont::Normal);

	format.setForeground(Qt::darkCyan);
	s_formats[Number] = format;

	format.setForeground(Qt::darkMagenta);
	s_formats[String] = format;

	format.setFontItalic(true);
	format.setForeground(Qt::gray);
	s_formats[Comment] = format;
	
	format.setFontItalic(false);
	format.setForeground(Qt::darkYellow);
	s_formats[Misc] = format;
}

void Highlighter::highlightBlock(const QString& text)
{
	foreach (Rule rule, m_rules)
	{
		int index = text.indexOf(rule.pattern);
		while (index >= 0)
		{
			int length = rule.pattern.matchedLength();
			setFormat(index, length, s_formats[rule.type]);
			index = text.indexOf(rule.pattern, index + length);
		}
	}

	if (!m_multiLineCommentStart.isEmpty())
	{
		setCurrentBlockState(0);

		int startIndex = 0;
		if (previousBlockState() != 1)
			startIndex = text.indexOf(m_multiLineCommentStart);

		while (startIndex >= 0) {
			int endIndex = text.indexOf(m_multiLineCommentEnd, startIndex);
			int commentLength;
			if (endIndex == -1) {
				setCurrentBlockState(1);
				commentLength = text.length() - startIndex;
			} else {
				commentLength = endIndex - startIndex + m_multiLineCommentStart.length();
			}
			setFormat(startIndex, commentLength, s_formats[Comment]);
			startIndex = text.indexOf(m_multiLineCommentStart, startIndex + commentLength);
		}
	}
}
