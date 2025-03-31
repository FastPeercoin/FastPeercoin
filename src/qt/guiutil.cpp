#include <QApplication>

#include "guiutil.h"

#include "bitcoinaddressvalidator.h"
#include "walletmodel.h"
#include "bitcoinunits.h"

#include "util.h"
#include "init.h"

#include <QDateTime>
#include <QDoubleValidator>
#include <QFont>
#include <QLineEdit>
#if QT_VERSION >= 0x050000
#include <QUrlQuery>
#else
#include <QUrl>
#endif
#include <QTextDocument> // For Qt::escape
#include <QAbstractItemView>
#include <QClipboard>
#include <QFileDialog>
#include <QDesktopServices>
#include <QThread>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501
#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "shlwapi.h"
#include "shlobj.h"
#include "shellapi.h"
#endif

namespace GUIUtil {

QString dateTimeStr(const QDateTime &date)
{
    return date.date().toString(Qt::SystemLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}

QString dateTimeStr(qint64 nTime)
{
    return dateTimeStr(QDateTime::fromTime_t((qint32)nTime));
}

QFont bitcoinAddressFont()
{
    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);
    return font;
}

void setupAddressWidget(QLineEdit *widget, QWidget *parent)
{
    widget->setMaxLength(BitcoinAddressValidator::MaxAddressLength);
    widget->setValidator(new BitcoinAddressValidator(parent));
    widget->setFont(bitcoinAddressFont());
}

void setupAmountWidget(QLineEdit *widget, QWidget *parent)
{
    QDoubleValidator *amountValidator = new QDoubleValidator(parent);
    amountValidator->setDecimals(8);
    amountValidator->setBottom(0.0);
    widget->setValidator(amountValidator);
    widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
}

bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out)
{
    // return if URI is not valid or is no bitcoin URI
    if(!uri.isValid() || (uri.scheme() != QString("peercoin") && uri.scheme() != QString("ppcoin")))
        return false;

    SendCoinsRecipient rv;
    rv.address = uri.path();
    rv.amount = 0;
#if QT_VERSION < 0x050000
    QList<QPair<QString, QString> > items = uri.queryItems();
#else
    QUrlQuery uriQuery(uri);
    QList<QPair<QString, QString> > items = uriQuery.queryItems();
#endif
    for (QList<QPair<QString, QString> >::iterator i = items.begin(); i != items.end(); i++)
    {
        bool fShouldReturnFalse = false;
        if (i->first.startsWith("req-"))
        {
            i->first.remove(0, 4);
            fShouldReturnFalse = true;
        }

        if (i->first == "label")
        {
            rv.label = i->second;
            fShouldReturnFalse = false;
        }
        else if (i->first == "amount")
        {
            if(!i->second.isEmpty())
            {
                if(!BitcoinUnits::parse(i->second, &rv.amount))
                {
                    return false;
                }
            }
            fShouldReturnFalse = false;
        }

        if (fShouldReturnFalse)
            return false;
    }
    if(out)
    {
        *out = rv;
    }
    return true;
}

bool parseBitcoinURI(QString uri, SendCoinsRecipient *out)
{
    // Convert bitcoin:// to bitcoin:
    //
    //    Cannot handle this later, because bitcoin:// will cause Qt to see the part after // as host,
    //    which will lower-case it (and thus invalidate the address).
    if(uri.startsWith("peercoin://"))
    {
        uri.replace(0, 11, "peercoin:");
    }
    if(uri.startsWith("ppcoin://"))
    {
        uri.replace(0, 9, "ppcoin:");
    }
    QUrl uriInstance(uri);
    return parseBitcoinURI(uriInstance, out);
}

QString HtmlEscape(const QString& str, bool fMultiLine)
{
#if QT_VERSION < 0x050000
    QString escaped = Qt::escape(str);
#else
    QString escaped = str.toHtmlEscaped();
#endif
    if(fMultiLine)
    {
        escaped = escaped.replace("\n", "<br>\n");
    }
    return escaped;
}

QString HtmlEscape(const std::string& str, bool fMultiLine)
{
    return HtmlEscape(QString::fromStdString(str), fMultiLine);
}

void copyEntryData(QAbstractItemView *view, int column, int role)
{
    if(!view || !view->selectionModel())
        return;
    QModelIndexList selection = view->selectionModel()->selectedRows(column);

    if(!selection.isEmpty())
    {
        // Copy first item (global clipboard)
        QApplication::clipboard()->setText(selection.at(0).data(role).toString(), QClipboard::Clipboard);
        // Copy first item (global mouse selection for e.g. X11 - NOP on Windows)
        QApplication::clipboard()->setText(selection.at(0).data(role).toString(), QClipboard::Selection);
    }
}

QString getSaveFileName(QWidget *parent, const QString &caption,
                                 const QString &dir,
                                 const QString &filter,
                                 QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty()) // Default to user documents location
    {
#if QT_VERSION < 0x050000
        myDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
#else
        myDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
#endif
    }
    else
    {
        myDir = dir;
    }
    QString result = QFileDialog::getSaveFileName(parent, caption, myDir, filter, &selectedFilter);

    /* Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...) */
    QRegExp filter_re(".* \\(\\*\\.(.*)[ \\)]");
    QString selectedSuffix;
    if(filter_re.exactMatch(selectedFilter))
    {
        selectedSuffix = filter_re.cap(1);
    }

    /* Add suffix if needed */
    QFileInfo info(result);
    if(!result.isEmpty())
    {
        if(info.suffix().isEmpty() && !selectedSuffix.isEmpty())
        {
            /* No suffix specified, add selected suffix */
            if(!result.endsWith("."))
                result.append(".");
            result.append(selectedSuffix);
        }
    }

    /* Return selected suffix if asked to */
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}

Qt::ConnectionType blockingGUIThreadConnection()
{
    if(QThread::currentThread() != qApp->thread())
    {
        return Qt::BlockingQueuedConnection;
    }
    else
    {
        return Qt::DirectConnection;
    }
}

bool checkPoint(const QPoint &p, const QWidget *w)
{
    QWidget *atW = QApplication::widgetAt(w->mapToGlobal(p));
    if (!atW) return false;
    return atW->topLevelWidget() == w;
}

bool isObscured(QWidget *w)
{
    return !(checkPoint(QPoint(0, 0), w)
        && checkPoint(QPoint(w->width() - 1, 0), w)
        && checkPoint(QPoint(0, w->height() - 1), w)
        && checkPoint(QPoint(w->width() - 1, w->height() - 1), w)
        && checkPoint(QPoint(w->width() / 2, w->height() / 2), w));
}

ToolTipToRichTextFilter::ToolTipToRichTextFilter(int size_threshold, QObject *parent) :
    QObject(parent), size_threshold(size_threshold)
{

}

bool ToolTipToRichTextFilter::eventFilter(QObject *obj, QEvent *evt)
{
    if(evt->type() == QEvent::ToolTipChange)
    {
        QWidget *widget = static_cast<QWidget*>(obj);
        QString tooltip = widget->toolTip();
        if(tooltip.size() > size_threshold && !tooltip.startsWith("<qt/>") && !Qt::mightBeRichText(tooltip))
        {
            // Prefix <qt/> to make sure Qt detects this as rich text
            // Escape the current message as HTML and replace \n by <br>
            tooltip = "<qt/>" + HtmlEscape(tooltip, true);
            widget->setToolTip(tooltip);
            return true;
        }
    }
    return QObject::eventFilter(obj, evt);
}

HelpMessageBox::HelpMessageBox(QWidget *parent) :
    QMessageBox(parent)
{
    header = tr("FastPeercoin-qt") + " " + tr("version") + " " +
        QString::fromStdString(FormatFullVersion()) + "\n\n" +
        tr("Usage:") + "\n" +
        "  FastPeercoin-qt [" + tr("command-line options") + "]                     " + "\n";

    coreOptions = QString::fromStdString(HelpMessage(HMM_BITCOIN_QT));

    setWindowTitle(tr("FastPeercoin-qt"));
    setTextFormat(Qt::PlainText);
    // setMinimumWidth is ignored for QMessageBox so put in non-breaking spaces to make it wider.
    setText(header + QString(QChar(0x2003)).repeated(50));
    setDetailedText(coreOptions + "\n" + uiOptions);
}

void HelpMessageBox::printToConsole()
{
    // On other operating systems, the expected action is to print the message to the console.
    QString strUsage = header + "\n" + coreOptions + "\n" + uiOptions;
    fprintf(stdout, "%s", strUsage.toStdString().c_str());
}

void HelpMessageBox::showOrPrint()
{
#if defined(WIN32)
        // On Windows, show a message box, as there is no stderr/stdout in windowed applications
        exec();
#else
        // On other operating systems, print help text to console
        printToConsole();
#endif
}

} // namespace GUIUtil
