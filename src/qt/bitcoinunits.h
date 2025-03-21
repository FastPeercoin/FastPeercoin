#ifndef BITCOINUNITS_H
#define BITCOINUNITS_H

#include <QString>

/** Bitcoin unit definitions. Encapsulates parsing and formatting
*/
class BitcoinUnits
{
public:
    //! Short name
    static QString name();
    //! Number of Satoshis (1e-8) per unit
    static qint64 factor();
    //! Number of decimals left
    static int decimals();
    //! Format as string
    static QString format(qint64 amount, bool plussign=false, bool trimzeros=true);
    //! Format as string (with unit)
    static QString formatWithUnit(qint64 amount, bool plussign=false, bool trimzeros=true);
    //! Parse string to coin amount
    static bool parse(const QString &value, qint64 *val_out);
};

#endif // BITCOINUNITS_H
