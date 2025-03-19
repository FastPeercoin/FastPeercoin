#include "splashscreen.h"
#include "clientversion.h"


#include <QPainter>
#include <QApplication>

#include "util.h"
SplashScreen::SplashScreen(const QPixmap &pixmap, Qt::WindowFlags f) :
    QSplashScreen(pixmap, f)
{
    this->setPixmap(QPixmap(":/images/splash"));
}
