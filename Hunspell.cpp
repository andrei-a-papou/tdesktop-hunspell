#include <QSyntaxHighlighter>
#include <QTextCodec>
#include <QFileInfo>
#include <QTextEdit>
#include <QLibrary>
#include <QLocale>

#include <memory>
#include <vector>

using namespace std;

using Hunhandle = void;
using Hunspell_create_type = Hunhandle *(*)(const char *affpath, const char *dpath);
using Hunspell_get_dic_encoding_type = char *(*)(Hunhandle *pHunspell);
using Hunspell_spell_type = int(*)(Hunhandle* pHunspell, const char*);
using Hunspell_destroy_type = void(*)(Hunhandle *pHunspell);

static Hunspell_create_type Hunspell_create;
static Hunspell_get_dic_encoding_type Hunspell_get_dic_encoding;
static Hunspell_spell_type Hunspell_spell;
static Hunspell_destroy_type Hunspell_destroy;

class HunspellHelper
{
    Q_DISABLE_COPY(HunspellHelper)

public:
    HunspellHelper(const QByteArray &lang)
        : m_hunspell(nullptr, Hunspell_destroy)
    {
        const QByteArray affPath = m_hunspellPath + lang + ".aff";
        const QByteArray dicPath = m_hunspellPath + lang + ".dic";
        if (QFileInfo(affPath).exists() && QFileInfo(dicPath).exists())
        {
            m_hunspell.reset(Hunspell_create(affPath, dicPath));
            m_codec = QTextCodec::codecForName(Hunspell_get_dic_encoding(m_hunspell.get()));
            if (!m_codec)
                m_hunspell.reset();
        }
    }

    bool isOpen() const
    {
        return (m_hunspell != nullptr);
    }

    bool spell(const QStringRef &word)
    {
        return (Hunspell_spell(m_hunspell.get(), m_codec->fromUnicode(word.data(), word.count())) != 0);
    }

private:
    const QByteArray m_hunspellPath = "/usr/share/hunspell/";

    unique_ptr<Hunhandle, Hunspell_destroy_type> m_hunspell;
    QTextCodec *m_codec = nullptr;
};

/**/

class HunspellHighlighter final : public QSyntaxHighlighter
{
public:
    HunspellHighlighter(QTextEdit *textEdit)
        : QSyntaxHighlighter(textEdit->document())
    {}

    void setSpellCheckers(QStringList languages)
    {
        m_hunspells.clear();
        languages.removeDuplicates();
        for (const QString &lang : languages)
        {
            auto hunspell = make_unique<HunspellHelper>(lang.toLatin1());
            if (hunspell->isOpen())
                m_hunspells.push_back(move(hunspell));
        }
    }

private:
    void highlightBlock(const QString &text)
    {
        if (m_hunspells.empty())
            return;

        QTextCharFormat underlineFmt;
        underlineFmt.setFontUnderline(true);
        underlineFmt.setUnderlineColor(Qt::red);

        const int textLen = text.length();
        int beginIdx = 0;
        for (int endIdx = 0; endIdx < textLen; ++endIdx)
        {
            const bool letterAtBeginIdx = text[beginIdx].isLetter();
            const bool letterAtEndIdx = text[endIdx].isLetter();
            const bool atEnd = letterAtEndIdx && (endIdx == textLen - 1);
            if (letterAtBeginIdx && (!letterAtEndIdx || atEnd))
            {
                const int wordLen = endIdx + atEnd - beginIdx;
                const QStringRef word = text.midRef(beginIdx, wordLen);
                if (word.length() > 1)
                {
                    bool correctWord = false;
                    for (auto &&hunspell : m_hunspells)
                    {
                        if (hunspell->spell(word))
                        {
                            correctWord = true;
                            break;
                        }
                    }
                    setFormat(beginIdx, wordLen, correctWord ? QTextCharFormat() : underlineFmt);
                }
            }
            if (!letterAtBeginIdx || !letterAtEndIdx)
                beginIdx = endIdx + 1;
        }
    }

private:
    vector<unique_ptr<HunspellHelper>> m_hunspells;
};

/**/

bool installHunspellSyntaxHighlighter(QTextEdit *textEdit)
{
    static bool hasHunspellFunctions = false;
    if (!hasHunspellFunctions)
    {
        QLibrary hunspellLib("hunspell");
        if (hunspellLib.load())
        {
            Hunspell_create = (Hunspell_create_type)hunspellLib.resolve("Hunspell_create");
            Hunspell_get_dic_encoding = (Hunspell_get_dic_encoding_type)hunspellLib.resolve("Hunspell_get_dic_encoding");
            Hunspell_spell = (Hunspell_spell_type)hunspellLib.resolve("Hunspell_spell");
            Hunspell_destroy = (Hunspell_destroy_type)hunspellLib.resolve("Hunspell_destroy");
            hasHunspellFunctions = (Hunspell_create && Hunspell_get_dic_encoding && Hunspell_spell && Hunspell_destroy);
        }
    }
    if (hasHunspellFunctions)
    {
        (new HunspellHighlighter(textEdit))->setSpellCheckers({QLocale::system().name(), "en_US"});
        return true;
    }
    return false;
}
