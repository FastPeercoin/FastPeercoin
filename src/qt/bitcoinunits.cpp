#include "bitcoinunits.h"

#include <QStringList>

QString BitcoinUnits::name()
{
    return QString("FPC");
}

qint64 BitcoinUnits::factor()
{
    return 1000000;
}

int BitcoinUnits::decimals()
{
    return 6;
}

QString BitcoinUnits::format(qint64 n, bool fPlus, bool trimzeros)
{
    // Note: not using straight sprintf here because we do NOT want
    // localized number formatting.
    qint64 coin = factor();
    int num_decimals = decimals();
    qint64 n_abs = (n > 0 ? n : -n);
    qint64 quotient = n_abs / coin;
    qint64 remainder = n_abs % coin;
    QString quotient_str = QString::number(quotient);
    QString remainder_str = QString::number(remainder).rightJustified(num_decimals, '0');

    if (trimzeros)
    {
        // Right-trim excess zeros after the decimal point
        int nTrim = 0;
        for (int i = remainder_str.size()-1; i>=2 && (remainder_str.at(i) == '0'); --i)
            ++nTrim;
        remainder_str.chop(nTrim);
    }

    if (n < 0)
        quotient_str.insert(0, '-');
    else if (fPlus && n > 0)
        quotient_str.insert(0, '+');
    return quotient_str + QString(".") + remainder_str;
}

QString BitcoinUnits::formatWithUnit(qint64 amount, bool plussign, bool trimzeros)
{
    return format(amount, plussign, trimzeros) + QString(" ") + name();
}

bool BitcoinUnits::parse(const QString &value, qint64 *val_out)
{
    if(value.isEmpty())
        return false; // Refuse to parse invalid unit or empty string
    int num_decimals = decimals();
    QStringList parts = value.split(".");

    if(parts.size() > 2)
    {
        return false; // More than one dot
    }
    QString whole = parts[0];
    QString decimals;

    if(parts.size() > 1)
    {
        decimals = parts[1];
    }
    if(decimals.size() > num_decimals)
    {
        return false; // Exceeds max precision
    }
    bool ok = false;
    QString str = whole + decimals.leftJustified(num_decimals, '0');

    if(str.size() > 18)
    {
        return false; // Longer numbers will exceed 63 bits
    }
    qint64 retvalue = str.toLongLong(&ok);
    if(val_out)
    {
        *val_out = retvalue;
    }
    return ok;
}
