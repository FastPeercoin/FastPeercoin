/*
 * W.J. van der Laan 2011-2012
 * The Peercoin developers 2013-2019
 */

#include <QApplication>

#include "bitcoingui.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "init.h"
#include "util.h"
#include "ui_interface.h"
#include "paymentserver.h"
#include "splashscreen.h"

#include <QMessageBox>
#if QT_VERSION < 0x050000
#include <QTextCodec>
#endif
#include <QLocale>
#include <QTimer>
#include <QTranslator>
#include <QLibraryInfo>

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#if defined(BITCOIN_NEED_QT_PLUGINS) && !defined(_BITCOIN_QT_PLUGINS_INCLUDED)
#define _BITCOIN_QT_PLUGINS_INCLUDED
#define __INSURE__
#include <QtPlugin>
Q_IMPORT_PLUGIN(qcncodecs)
Q_IMPORT_PLUGIN(qjpcodecs)
Q_IMPORT_PLUGIN(qtwcodecs)
Q_IMPORT_PLUGIN(qkrcodecs)
Q_IMPORT_PLUGIN(qtaccessiblewidgets)
#endif

// Declare meta types used for QMetaObject::invokeMethod
Q_DECLARE_METATYPE(bool*)

// Need a global reference for the notifications to find the GUI
static BitcoinGUI *guiref;
static SplashScreen *splashref;

static bool ThreadSafeMessageBox(const std::string& message, const std::string& caption, unsigned int style)
{
    // Message from network thread
    if(guiref)
    {
        bool modal = (style & CClientUIInterface::MODAL);
        bool ret = false;
        // In case of modal message, use blocking connection to wait for user to click a button
        QMetaObject::invokeMethod(guiref, "message",
                                   modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                                   Q_ARG(QString, QString::fromStdString(caption)),
                                   Q_ARG(QString, QString::fromStdString(message)),
                                   Q_ARG(unsigned int, style),
                                   Q_ARG(bool*, &ret));
        return ret;
    }
    else
    {
        printf("%s: %s\n", caption.c_str(), message.c_str());
        fprintf(stderr, "%s: %s\n", caption.c_str(), message.c_str());
        return false;
    }
}

static bool ThreadSafeAskFee(int64 nFeeRequired)
{
    if(!guiref)
        return false;
    if(nFeeRequired < CTransaction::nMinTxFee || nFeeRequired <= nTransactionFee || fDaemon)
        return true;

    bool payFee = false;

    QMetaObject::invokeMethod(guiref, "askFee", GUIUtil::blockingGUIThreadConnection(),
                               Q_ARG(qint64, nFeeRequired),
                               Q_ARG(bool*, &payFee));

    return payFee;
}

static void InitMessage(const std::string &message)
{
    splashref->showMessage(QString::fromStdString(message), Qt::AlignBottom|Qt::AlignHCenter, QColor(55,55,55));
    qApp->processEvents();
    printf("init message: %s\n", message.c_str());
}

/* Handle runaway exceptions. Shows a message box with the problem and quits the program.
 */
static void handleRunawayException(std::exception *e)
{
    PrintExceptionContinue(e, "Runaway exception");
    QMessageBox::critical(0, "Runaway exception", BitcoinGUI::tr("A fatal error occurred. Peercoin can no longer continue safely and will quit.") + QString("\n\n") + QString::fromStdString(strMiscWarning));
    exit(1);
}

#ifndef BITCOIN_QT_TEST
int main(int argc, char *argv[])
{
    // Command-line options take precedence:
    ParseParameters(argc, argv);

#if QT_VERSION < 0x050000
    // Internal string conversion is all UTF-8
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForCStrings(QTextCodec::codecForTr());
#endif

    Q_INIT_RESOURCE(bitcoin);
    QApplication app(argc, argv);

    // Register meta types used for QMetaObject::invokeMethod
    qRegisterMetaType< bool* >();

    // Do this early as we don't want to bother initializing if we are just calling IPC
    // ... but do it after creating app, so QCoreApplication::arguments is initialized:
    if (PaymentServer::ipcSendCommandLine())
        exit(0);
    PaymentServer* paymentServer = new PaymentServer(&app);

    // Install global event filter that makes sure that long tooltips can be word-wrapped
    app.installEventFilter(new GUIUtil::ToolTipToRichTextFilter(TOOLTIP_WRAP_THRESHOLD, &app));

    // ... then bitcoin.conf:
    if (!boost::filesystem::is_directory(GetDataDir(false)))
    {
        // This message can not be translated, as translation is not initialized yet
        // (which not yet possible because lang=XX can be overridden in bitcoin.conf in the data directory)
        QMessageBox::critical(0, "Bitcoin",
                              QString("Error: Specified data directory \"%1\" does not exist.").arg(QString::fromStdString(mapArgs["-datadir"])));
        return 1;
    }
    ReadConfigFile(mapArgs, mapMultiArgs);

    // Application identification
    QApplication::setOrganizationName("Peercoin");
    QApplication::setOrganizationDomain("peercoin.net");
    QApplication::setApplicationName("Peercoin");

    // Subscribe to global signals from core
    uiInterface.ThreadSafeMessageBox.connect(ThreadSafeMessageBox);
    uiInterface.ThreadSafeAskFee.connect(ThreadSafeAskFee);
    uiInterface.InitMessage.connect(InitMessage);

    // Show help message immediately after parsing command-line options (for "-lang") and setting locale,
    // but before showing splash screen.
    if (mapArgs.count("-?") || mapArgs.count("--help"))
    {
        GUIUtil::HelpMessageBox help;
        help.showOrPrint();
        return 1;
    }

    SplashScreen splash(QPixmap(), 0);
    splash.show();
    splash.setAutoFillBackground(true);
    splashref = &splash;

    app.processEvents();
    app.setQuitOnLastWindowClosed(false);

    try
    {
        boost::thread_group threadGroup;

        BitcoinGUI window;
        guiref = &window;

        QTimer* pollShutdownTimer = new QTimer(guiref);
        QObject::connect(pollShutdownTimer, SIGNAL(timeout()), guiref, SLOT(detectShutdown()));
        pollShutdownTimer->start(200);

        if(AppInit2(threadGroup))
        {
            {
                // Put this in a block, so that the Model objects are cleaned up before
                // calling Shutdown().

                splash.finish(&window);

                ClientModel clientModel;
                WalletModel walletModel(pwalletMain);

                window.setClientModel(&clientModel);

                window.addWallet("~Default", &walletModel);
                window.setCurrentWallet("~Default");

                window.show();

                // Now that initialization/startup is done, process any command-line
                // bitcoin: URIs
                QObject::connect(paymentServer, SIGNAL(receivedURI(QString)), &window, SLOT(handleURI(QString)));
                QTimer::singleShot(100, paymentServer, SLOT(uiReady()));

                app.exec();

                window.hide();
                window.setClientModel(0);
                window.removeAllWallets();
                guiref = 0;
            }
            // Shutdown the core and its threads, but don't exit Bitcoin-Qt here
            threadGroup.interrupt_all();
            threadGroup.join_all();
            Shutdown();
        }
        else
        {
            threadGroup.interrupt_all();
            threadGroup.join_all();
            Shutdown();
            return 1;
        }
    } catch (std::exception& e) {
        handleRunawayException(&e);
    } catch (...) {
        handleRunawayException(NULL);
    }
    return 0;
}

#endif // BITCOIN_QT_TEST
