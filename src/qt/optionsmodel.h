#ifndef OPTIONSMODEL_H
#define OPTIONSMODEL_H

#include <QAbstractListModel>

/** Interface from Qt to configuration data structure for Bitcoin client.
   To Qt, the options are presented as a list with the different options
   laid out vertically.
   This can be changed to a tree once the settings become sufficiently
   complex.
 */
class OptionsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit OptionsModel(QObject *parent = 0);

    enum OptionID {
        ProxyUse,          // bool
        ProxyIP,           // QString
        ProxyPort,         // int
        ProxySocksVersion, // int
        DisplayUnit,       // BitcoinUnits::Unit
        Language,          // QString
        CoinControlFeatures, // bool
        OptionIDRowCount,
    };

    void Init();

    /* Migrate settings from wallet.dat after app initialization */
    bool Upgrade(); /* returns true if settings upgraded */

    int rowCount(const QModelIndex & parent = QModelIndex()) const;
    QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);

    /* Explicit getters */
    int getDisplayUnit() { return nDisplayUnit; }
    QString getLanguage() { return language; }

    bool getCoinControlFeatures() { return fCoinControlFeatures; }

private:
    int nDisplayUnit;

    bool fCoinControlFeatures;

    QString language;

signals:
    void displayUnitChanged(int unit);
    void coinControlFeaturesChanged(bool);
};

#endif // OPTIONSMODEL_H
