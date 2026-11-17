#ifndef URITESTS_H
#define URITESTS_H

#include <QTest>
#include <QObject>
#include "../overviewpage.h"

class URITests : public QObject
{
    Q_OBJECT

private slots:
    void uriTests();
    void addressValidatorTests();
    void embeddedTranslationLoads();
    void languagePreferencePriority();
    void amountFieldEmitsValueChange();
};

class OverviewPageTests : public QObject
{
    Q_OBJECT
private slots:
    void stakingWalletStates();
};

class PassphraseDialogTests : public QObject
{
    Q_OBJECT
private slots:
    void cancelKeepsWalletLocked();
    void incorrectPassphraseKeepsWalletLocked();
};

class BitcoinGUITests : public QObject
{
    Q_OBJECT
private slots:
    void constructsWithoutModels();
    void everydayNavigationAndSettingsAreVisible();
    void activeAssetShowsRelevantWalletPage();
    void loadedWalletMenuSwitchesWallets();
    void navigationUsesInstalledTranslation();
    void languagePreferenceCancelAndReload();
    void capturesMainWindowAtSupportedSizes();
};

#endif // URITESTS_H
