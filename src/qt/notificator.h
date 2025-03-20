#ifndef NOTIFICATOR_H
#define NOTIFICATOR_H

#if defined(HAVE_CONFIG_H)
#include "bitcoin-config.h"
#endif

#include <QObject>
#include <QIcon>

QT_BEGIN_NAMESPACE
#ifdef USE_DBUS
class QDBusInterface;
#endif
QT_END_NAMESPACE

/** Cross-platform desktop notification client. */
class Notificator: public QObject
{
    Q_OBJECT

public:
    /** Create a new notificator.
    */
    Notificator(const QString &programName=QString(), QWidget *parent=0);
    ~Notificator();

    // Message class
    enum Class
    {
        Information,    /**< Informational message */
        Warning,        /**< Notify user of potential problem */
        Critical        /**< An error occurred */
    };

public slots:
    /** Show notification message.
       @param[in] cls    general message class
       @param[in] title  title shown with message
       @param[in] text   message content
       @param[in] icon   optional icon to show with message
       @param[in] millisTimeout notification timeout in milliseconds (defaults to 10 seconds)
       @note Platform implementations are free to ignore any of the provided fields except for \a text.
     */
    void notify(Class cls, const QString &title, const QString &text,
                const QIcon &icon = QIcon(), int millisTimeout = 10000);

private:
    QWidget *parent;
    enum Mode {
        None,                       /**< Ignore informational notifications, and show a modal pop-up dialog for Critical notifications. */
        Freedesktop,                /**< Use DBus org.freedesktop.Notifications */
        UserNotificationCenter      /**< Use the 10.8+ User Notification Center (Mac only) */
    };
    QString programName;
    Mode mode;
#ifdef USE_DBUS
    QDBusInterface *interface;

    void notifyDBus(Class cls, const QString &title, const QString &text, const QIcon &icon, int millisTimeout);
#endif
};

#endif // NOTIFICATOR_H
