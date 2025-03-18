#if defined(HAVE_CONFIG_H)
#include "bitcoin-config.h"
#endif

#include "optionsmodel.h"

#include "bitcoinunits.h"
#include "init.h"
#include "walletdb.h"
#include "guiutil.h"

#include <QSettings>

OptionsModel::OptionsModel(QObject *parent) :
    QAbstractListModel(parent)
{
    Init();
}

bool static ApplyProxySettings()
{
    QSettings settings;
    CService addrProxy(settings.value("addrProxy", "127.0.0.1:9050").toString().toStdString());
    int nSocksVersion(settings.value("nSocksVersion", 5).toInt());
    if (!settings.value("fUseProxy", false).toBool()) {
        addrProxy = CService();
        nSocksVersion = 0;
        return false;
    }
    if (nSocksVersion && !addrProxy.IsValid())
        return false;
    if (!IsLimited(NET_IPV4))
        SetProxy(NET_IPV4, addrProxy, nSocksVersion);
    if (nSocksVersion > 4) {
#ifdef USE_IPV6
        if (!IsLimited(NET_IPV6))
            SetProxy(NET_IPV6, addrProxy, nSocksVersion);
#endif
        SetNameProxy(addrProxy, nSocksVersion);
    }
    return true;
}

void OptionsModel::Init()
{
    QSettings settings;

    // These are Qt-only settings:
    nDisplayUnit = settings.value("nDisplayUnit", BitcoinUnits::BTC).toInt();
    fCoinControlFeatures = settings.value("fCoinControlFeatures", false).toBool();
    language = settings.value("language", "").toString();

    // These are shared with core Bitcoin; we want
    // command-line options to override the GUI settings:
    if (settings.contains("addrProxy") && settings.value("fUseProxy").toBool())
        SoftSetArg("-proxy", settings.value("addrProxy").toString().toStdString());
    if (settings.contains("nSocksVersion") && settings.value("fUseProxy").toBool())
        SoftSetArg("-socks", settings.value("nSocksVersion").toString().toStdString());
    if (!language.isEmpty())
        SoftSetArg("-lang", language.toStdString());
}

bool OptionsModel::Upgrade()
{
    QSettings settings;

    if (settings.contains("bImportFinished"))
        return false; // Already upgraded

    settings.setValue("bImportFinished", true);

    // Move settings from old wallet.dat (if any):
    CWalletDB walletdb("wallet.dat");

    QList<QString> intOptions;
    intOptions << "nDisplayUnit";
    foreach(QString key, intOptions)
    {
        int value = 0;
        if (walletdb.ReadSetting(key.toStdString(), value))
        {
            settings.setValue(key, value);
            walletdb.EraseSetting(key.toStdString());
        }
    }
    QList<QString> boolOptions;
    boolOptions << "fUseProxy";
    foreach(QString key, boolOptions)
    {
        bool value = false;
        if (walletdb.ReadSetting(key.toStdString(), value))
        {
            settings.setValue(key, value);
            walletdb.EraseSetting(key.toStdString());
        }
    }
    try
    {
        CAddress addrProxyAddress;
        if (walletdb.ReadSetting("addrProxy", addrProxyAddress))
        {
            settings.setValue("addrProxy", addrProxyAddress.ToStringIPPort().c_str());
            walletdb.EraseSetting("addrProxy");
        }
    }
    catch (std::ios_base::failure &e)
    {
        // 0.6.0rc1 saved this as a CService, which causes failure when parsing as a CAddress
        CService addrProxy;
        if (walletdb.ReadSetting("addrProxy", addrProxy))
        {
            settings.setValue("addrProxy", addrProxy.ToStringIPPort().c_str());
            walletdb.EraseSetting("addrProxy");
        }
    }
    ApplyProxySettings();
    Init();

    return true;
}


int OptionsModel::rowCount(const QModelIndex & parent) const
{
    return OptionIDRowCount;
}

QVariant OptionsModel::data(const QModelIndex & index, int role) const
{
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case ProxyUse: {
            proxyType proxy;
            return QVariant(GetProxy(NET_IPV4, proxy));
        }
        case ProxyIP: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(QString::fromStdString(proxy.first.ToStringIP()));
            else
                return QVariant(QString::fromStdString("127.0.0.1"));
        }
        case ProxyPort: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(proxy.first.GetPort());
            else
                return QVariant(9050);
        }
        case ProxySocksVersion: {
            proxyType proxy;
            if (GetProxy(NET_IPV4, proxy))
                return QVariant(proxy.second);
            else
                return QVariant(5);
        }
        case DisplayUnit:
            return QVariant(nDisplayUnit);
        case CoinControlFeatures:
            return QVariant(fCoinControlFeatures);
        case Language:
            return settings.value("language", "");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

bool OptionsModel::setData(const QModelIndex & index, const QVariant & value, int role)
{
    bool successful = true; /* set to false on parse error */
    if(role == Qt::EditRole)
    {
        QSettings settings;
        switch(index.row())
        {
        case ProxyUse:
            settings.setValue("fUseProxy", value.toBool());
            successful = ApplyProxySettings();
            break;
        case ProxyIP: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", 9050);
            GetProxy(NET_IPV4, proxy);

            CNetAddr addr(value.toString().toStdString());
            proxy.first.SetIP(addr);
            settings.setValue("addrProxy", proxy.first.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxyPort: {
            proxyType proxy;
            proxy.first = CService("127.0.0.1", 9050);
            GetProxy(NET_IPV4, proxy);

            proxy.first.SetPort(value.toInt());
            settings.setValue("addrProxy", proxy.first.ToStringIPPort().c_str());
            successful = ApplyProxySettings();
        }
        break;
        case ProxySocksVersion: {
            proxyType proxy;
            proxy.second = 5;
            GetProxy(NET_IPV4, proxy);

            proxy.second = value.toInt();
            settings.setValue("nSocksVersion", proxy.second);
            successful = ApplyProxySettings();
        }
        break;
        case DisplayUnit:
            nDisplayUnit = value.toInt();
            settings.setValue("nDisplayUnit", nDisplayUnit);
            emit displayUnitChanged(nDisplayUnit);
            break;
        case Language:
            settings.setValue("language", value);
            break;
        case CoinControlFeatures:
            fCoinControlFeatures = value.toBool();
            settings.setValue("fCoinControlFeatures", fCoinControlFeatures);
            emit coinControlFeaturesChanged(fCoinControlFeatures);
            break;
        default:
            break;
        }
    }
    emit dataChanged(index, index);

    return successful;
}
