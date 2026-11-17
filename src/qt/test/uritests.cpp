#include <QUrl>
#include <QPushButton>
#include <QSignalSpy>
#include <QDir>
#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QTimer>
#include <QTranslator>
#include <QToolButton>
#include <QComboBox>
#include <QSettings>
#include <QLabel>
#include <QStackedWidget>
#include <QCoreApplication>
#include <QAction>
#include <QListWidget>
#include <QMenu>
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>
#include <QLayout>

#include "uritests.h"
#include "../bitcoinamountfield.h"
#include "../bitcoinunits.h"
#include "../bitcoingui.h"
#include "../guiutil.h"
#include "../optionsdialog.h"
#include "../optionsmodel.h"
#include "../overviewpage.h"
#include "../walletmodel.h"
#include "../../main.h"
#include "../bitcoinaddressvalidator.h"
#include "../askpassphrasedialog.h"
#include "wallet.h"

void BitcoinGUITests::constructsWithoutModels()
{
    BitcoinGUI gui;
    QVERIFY(gui.findChild<OverviewPage *>());
}

void BitcoinGUITests::everydayNavigationAndSettingsAreVisible()
{
    BitcoinGUI gui;
    gui.resize(750, 500);
    gui.show();
    QApplication::processEvents();
    for (const char *name : {"overviewActionNavButton", "sendCoinsActionNavButton",
                             "receiveCoinsActionNavButton", "historyActionNavButton",
                             "addressBookActionNavButton", "moreToolsButton", "settingsButton"}) {
        QToolButton *button = gui.findChild<QToolButton *>(name);
        QVERIFY2(button, name);
        QVERIFY(button->isVisibleTo(&gui));
    }
    QToolButton *walletSelector = gui.findChild<QToolButton *>("walletUnitSelector");
    QVERIFY(walletSelector);
    QFrame *sidebar = gui.findChild<QFrame *>("sidebar");
    QVERIFY(sidebar);
    QCOMPARE(walletSelector->width(), sidebar->layout()->contentsRect().width());
    QVERIFY(walletSelector->width() >= walletSelector->sizeHint().width());
    QScrollArea *overviewScroll = gui.findChild<QScrollArea *>("overviewPageScrollArea");
    QVERIFY(overviewScroll);
    OverviewPage *overview = gui.findChild<OverviewPage *>();
    QCOMPARE(overview->width(), overviewScroll->viewport()->width());
    QCOMPARE(overviewScroll->horizontalScrollBar()->maximum(), 0);
    OptionsModel model;
    OptionsDialog options(&gui);
    options.setModel(&model);
    QComboBox *language = options.findChild<QComboBox *>("languageCombo");
    QVERIFY(language);
    QVERIFY(language->findData("pt_BR") >= 0);
    QVERIFY(language->findData(QString()) >= 0);
    QVERIFY(options.findChild<QLabel *>("languageRestartLabel")->text().contains("restart"));
}

void BitcoinGUITests::activeAssetShowsRelevantWalletPage()
{
    CWallet shares;
    shares.SetUnit('S');
    OptionsModel options;
    WalletModel sharesModel(&shares, &options);
    {
        BitcoinGUI gui;
        gui.resize(750, 500);
        gui.setWalletModel(&sharesModel);
        gui.show();
        QApplication::processEvents();
        QToolButton *asset = gui.findChild<QToolButton *>("walletUnitSelector");
        QApplication::processEvents();
        QCOMPARE(asset->width(), gui.findChild<QFrame *>("sidebar")->layout()->contentsRect().width());
        QVERIFY(asset->width() >= asset->sizeHint().width());
        QVERIFY(asset->text().contains("NuShares (NSR)"));
        QVERIFY(gui.findChild<QToolButton *>("stakingPageActionNavButton")->isVisibleTo(&gui));
        QVERIFY(!gui.findChild<QToolButton *>("parkActionNavButton")->isVisibleTo(&gui));
        gui.findChild<QToolButton *>("stakingPageActionNavButton")->click();
        QCOMPARE(gui.findChild<QStackedWidget *>("centralWidget")->currentWidget(),
                 gui.findChild<QScrollArea *>("stakingPageScrollArea"));
    }

    CWallet bits;
    bits.SetUnit('B');
    WalletModel bitsModel(&bits, &options);
    BitcoinGUI gui;
    gui.resize(750, 500);
    gui.setWalletModel(&bitsModel);
    gui.show();
    QApplication::processEvents();
    QToolButton *asset = gui.findChild<QToolButton *>("walletUnitSelector");
    QCOMPARE(asset->width(), gui.findChild<QFrame *>("sidebar")->layout()->contentsRect().width());
    QVERIFY(asset->width() >= asset->sizeHint().width());
    QVERIFY(asset->text().contains("US-NBT (US-NBT)"));
    QVERIFY(gui.findChild<QToolButton *>("parkActionNavButton")->isVisibleTo(&gui));
    QVERIFY(!gui.findChild<QToolButton *>("stakingPageActionNavButton")->isVisibleTo(&gui));
    gui.findChild<QToolButton *>("parkActionNavButton")->click();
    QCOMPARE(gui.findChild<QStackedWidget *>("centralWidget")->currentWidget(),
             gui.findChild<QScrollArea *>("parkingPageScrollArea"));
}

void BitcoinGUITests::loadedWalletMenuSwitchesWallets()
{
    CWallet bits;
    bits.SetUnit('B');
    CWallet shares;
    shares.SetUnit('S');
    struct WalletRegistration {
        CWallet *bits;
        CWallet *shares;
        WalletRegistration(CWallet *b, CWallet *s) : bits(b), shares(s)
        {
            RegisterWallet(bits);
            RegisterWallet(shares);
        }
        ~WalletRegistration()
        {
            UnregisterWallet(bits);
            UnregisterWallet(shares);
        }
    } registration(&bits, &shares);

    OptionsModel options;
    struct ModelCleanup {
        WalletModel *model;
        ~ModelCleanup() { delete model; }
    } cleanup{new WalletModel(&bits, &options)};

    {
        BitcoinGUI gui;
        gui.resize(750, 500);
        gui.setWalletModel(cleanup.model);
        gui.show();
        QApplication::processEvents();
        QToolButton *selector = gui.findChild<QToolButton *>("walletUnitSelector");
        QMenu *menu = selector ? selector->menu() : nullptr;
        QVERIFY(menu);
        QAction *sharesAction = nullptr;
        QAction *bitsAction = nullptr;
        for (QAction *action : menu->actions()) {
            if (action->text() == BitcoinUnits::baseName('S'))
                sharesAction = action;
            if (action->text() == BitcoinUnits::baseName('B'))
                bitsAction = action;
        }
        QVERIFY(sharesAction);
        QVERIFY(bitsAction);

        sharesAction->trigger();
        cleanup.model = gui.getWalletModel();
        QCOMPARE(cleanup.model->getWallet(), &shares);
        QCOMPARE(selector->text(), QString("NuShares (NSR)"));
        QTRY_COMPARE(selector->width(), gui.findChild<QFrame *>("sidebar")->layout()->contentsRect().width());
        QVERIFY(selector->width() >= selector->sizeHint().width());
        QVERIFY(gui.findChild<QToolButton *>("stakingPageActionNavButton")->isVisibleTo(&gui));
        QVERIFY(!gui.findChild<QToolButton *>("parkActionNavButton")->isVisibleTo(&gui));
        QVERIFY(gui.findChild<QAction *>("voteAction")->isVisible());
        bitsAction = nullptr;
        for (QAction *action : menu->actions()) {
            if (action->text() == BitcoinUnits::baseName('B'))
                bitsAction = action;
        }
        QVERIFY(bitsAction);
        bitsAction->trigger();
        cleanup.model = gui.getWalletModel();
        QCOMPARE(cleanup.model->getWallet(), &bits);
        QCOMPARE(selector->text(), QString("US-NBT (US-NBT)"));
        QTRY_COMPARE(selector->width(), gui.findChild<QFrame *>("sidebar")->layout()->contentsRect().width());
        QVERIFY(selector->width() >= selector->sizeHint().width());
        QVERIFY(!gui.findChild<QToolButton *>("stakingPageActionNavButton")->isVisibleTo(&gui));
        QVERIFY(gui.findChild<QToolButton *>("parkActionNavButton")->isVisibleTo(&gui));
        QVERIFY(!gui.findChild<QAction *>("voteAction")->isVisible());
    }
}

void BitcoinGUITests::navigationUsesInstalledTranslation()
{
    QTranslator translator;
    QVERIFY(translator.load(":/translations/bitcoin_pt_BR.qm"));
    const QPair<const char *, const char *> requiredTranslations[] = {
        {"BitcoinGUI", "&Addresses"},
        {"BitcoinGUI", "Browse transaction history"},
        {"BitcoinGUI", "Edit the list of stored addresses and labels"},
        {"BitcoinGUI", "Send coins to a %1 address"},
        {"BitcoinGUI", "Show the list of addresses for receiving %1"},
        {"BitcoinGUI", "Manage NuBits parked in the wallet"},
        {"BitcoinGUI", "Review wallet security, startup settings, and network state. These do not confirm staking participation or guarantee rewards."},
        {"BitcoinGUI", "&Backup Wallet"},
        {"BitcoinGUI", "&Encrypt Portfolio"},
        {"BitcoinGUI", "&Unlock Wallet for Minting Only"},
        {"BitcoinGUI", "&Change Passphrase"},
        {"BitcoinGUI", "&Export NuBits keys..."},
        {"BitcoinGUI", "&Distribute dividends..."},
        {"BitcoinGUI", "&Debug window"},
        {"BitcoinGUI", "More"},
        {"BitcoinGUI", "More tools"},
        {"BitcoinGUI", "&Network peers..."},
        {"BitcoinGUI", "Choose a loaded NuBits or NuShares wallet"},
        {"BitcoinGUI", "No NuBits network connections were found. Open More > Network peers to add one manually."},
    };
    for (const auto &entry : requiredTranslations) {
        const QString source = QString::fromUtf8(entry.second);
        const QString translated = translator.translate(entry.first, entry.second);
        QVERIFY2(!translated.isEmpty(), entry.second);
        QCOMPARE(source.contains("%1"), translated.contains("%1"));
    }
    qApp->installTranslator(&translator);
    {
        BitcoinGUI gui;
        gui.show();
        QApplication::processEvents();
        QToolButton *send = gui.findChild<QToolButton *>("sendCoinsActionNavButton");
        QVERIFY(send);
        QCOMPARE(send->text(), QString("Enviar"));
        QAction *sendAction = gui.findChild<QAction *>("sendCoinsAction");
        QVERIFY(sendAction);
        QCOMPARE(sendAction->text(), QString("&Enviar"));
        QCOMPARE(send->toolTip(), QString("Enviar moedas para um endereço %1")
                     .arg(BitcoinUnits::baseName()));
        QToolButton *settings = gui.findChild<QToolButton *>("settingsButton");
        QVERIFY(settings);
        QCOMPARE(settings->text(), QString("Configurações"));
        QAction *settingsAction = gui.findChild<QAction *>("optionsAction");
        QVERIFY(settingsAction);
        QCOMPARE(settingsAction->text(), QString("&Configurações..."));
        OptionsDialog preferences(&gui);
        QLabel *languageLabel = preferences.findChild<QLabel *>("languageRestartLabel");
        QComboBox *language = preferences.findChild<QComboBox *>("languageCombo");
        QVERIFY(languageLabel);
        QVERIFY(language);
        QCOMPARE(languageLabel->text(), QString("&Idioma (requer reinicialização):"));
        QCOMPARE(language->itemText(language->findData(QString())),
                 QString("Usar o idioma do sistema"));

        QSignalSpy sendTriggered(sendAction, &QAction::triggered);
        QTest::keyClick(&gui, Qt::Key_2, Qt::AltModifier);
        QCOMPARE(sendTriggered.count(), 1);
        QVERIFY(sendAction->isChecked());
        sendTriggered.clear();
        send->click();
        QCOMPARE(sendTriggered.count(), 1);
    }
    qApp->removeTranslator(&translator);
}

void BitcoinGUITests::languagePreferenceCancelAndReload()
{
    QSettings preferences;
    preferences.remove("language");
    OptionsModel model;
    {
        OptionsDialog dialog;
        dialog.setModel(&model);
        QComboBox *language = dialog.findChild<QComboBox *>("languageCombo");
        QVERIFY(language);
        language->setCurrentIndex(language->findData("pt_BR"));
        dialog.findChild<QDialogButtonBox *>("optionsButtonBox")
            ->button(QDialogButtonBox::Cancel)->click();
    }
    QVERIFY(!preferences.contains("language"));

    {
        OptionsDialog dialog;
        dialog.setModel(&model);
        QComboBox *language = dialog.findChild<QComboBox *>("languageCombo");
        language->setCurrentIndex(language->findData("pt_BR"));
        QTimer::singleShot(0, &dialog, []() {
            if (QMessageBox *notice =
                    qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                notice->accept();
        });
        dialog.findChild<QDialogButtonBox *>("optionsButtonBox")
            ->button(QDialogButtonBox::Apply)->click();
    }
    QCOMPARE(preferences.value("language").toString(), QString("pt_BR"));
    {
        OptionsDialog reopened;
        QComboBox *language = reopened.findChild<QComboBox *>("languageCombo");
        QVERIFY(language);
        QCOMPARE(language->currentData().toString(), QString("pt_BR"));
    }
    preferences.remove("language");
}

void BitcoinGUITests::capturesMainWindowAtSupportedSizes()
{
    const QString evidenceDir = qEnvironmentVariable("NUBITS_UI_EVIDENCE_DIR");
    if (evidenceDir.isEmpty())
        return;
    QVERIFY(QDir().mkpath(evidenceDir));

    QTranslator translator;
    CWallet bits;
    bits.SetUnit('B');
    CWallet shares;
    shares.SetUnit('S');
    RegisterWallet(&bits);
    RegisterWallet(&shares);
    struct WalletUnregister {
        CWallet *bits;
        CWallet *shares;
        ~WalletUnregister() { UnregisterWallet(bits); UnregisterWallet(shares); }
    } walletUnregister{&bits, &shares};
    OptionsModel options;
    WalletModel bitsModel(&bits, &options);
    WalletModel sharesModel(&shares, &options);
    const QSize sizes[] = {QSize(750, 500), QSize(1100, 700)};
    const QString locales[] = {"en", "pt_BR"};
    for (const QString &locale : locales) {
        if (locale == "pt_BR") {
            QVERIFY(translator.load(":/translations/bitcoin_pt_BR.qm"));
            qApp->installTranslator(&translator);
        } else {
            qApp->removeTranslator(&translator);
        }
        BitcoinGUI gui;
        gui.setWalletModel(&bitsModel);
        gui.findChild<OverviewPage *>()->setStakingNetworkStatus(
            QCoreApplication::translate("OverviewPage", "Network state unavailable."));
        gui.show();
        for (const QSize &size : sizes) {
            gui.resize(size);
            QApplication::processEvents();
            QCOMPARE(gui.size(), size);
            QString path = QString("%1/%2-overview-nbt-%3x%4.png")
                .arg(evidenceDir, locale).arg(size.width()).arg(size.height());
            QVERIFY2(gui.grab().save(path), qPrintable(path));

            const char *navigation[] = {"sendCoinsActionNavButton", "receiveCoinsActionNavButton",
                                        "historyActionNavButton", "addressBookActionNavButton",
                                        "parkActionNavButton"};
            const char *evidence[] = {"send", "receive", "transactions", "addresses", "parking"};
            for (int page = 0; page < 5; ++page) {
                gui.findChild<QToolButton *>(navigation[page])->click();
                QApplication::processEvents();
                path = QString("%1/%2-%3-%4x%5.png")
                    .arg(evidenceDir, locale, evidence[page])
                    .arg(size.width()).arg(size.height());
                QVERIFY2(gui.grab().save(path), qPrintable(path));
            }

            QMenu *unitMenu = gui.findChild<QToolButton *>("walletUnitSelector")->menu();
            QVERIFY(unitMenu);
            unitMenu->popup(gui.mapToGlobal(QPoint(0, 0)));
            QApplication::processEvents();
            path = QString("%1/%2-wallets-%3x%4.png")
                .arg(evidenceDir, locale).arg(size.width()).arg(size.height());
            QVERIFY2(unitMenu->grab().save(path), qPrintable(path));
            unitMenu->hide();

            QMenu *moreMenu = gui.findChild<QToolButton *>("moreToolsButton")->menu();
            QVERIFY(moreMenu);
            moreMenu->popup(gui.mapToGlobal(QPoint(0, 0)));
            QApplication::processEvents();
            path = QString("%1/%2-more-%3x%4.png")
                .arg(evidenceDir, locale).arg(size.width()).arg(size.height());
            QVERIFY2(moreMenu->grab().save(path), qPrintable(path));
            moreMenu->hide();

            OptionsDialog dialog(&gui);
            dialog.setModel(&options);
            dialog.resize(size);
            dialog.show();
            QListWidget *settingsPages = dialog.findChild<QListWidget *>();
            QVERIFY(settingsPages);
            settingsPages->setCurrentRow(1);
            QApplication::processEvents();
            QVERIFY(dialog.findChild<QComboBox *>("languageCombo")->isVisibleTo(&dialog));
            const QSize effectiveSize = dialog.size();
            QCOMPARE(effectiveSize,
                     QSize(qMax(size.width(), dialog.minimumWidth()),
                           qMax(size.height(), dialog.minimumHeight())));
            path = QString("%1/%2-settings-display-%3x%4.png")
                .arg(evidenceDir, locale).arg(effectiveSize.width()).arg(effectiveSize.height());
            QVERIFY2(dialog.grab().save(path), qPrintable(path));
            dialog.close();
            gui.findChild<QToolButton *>("overviewActionNavButton")->click();
        }

        BitcoinGUI sharesGui;
        sharesGui.setWalletModel(&sharesModel);
        sharesGui.findChild<OverviewPage *>()->setStakingNetworkStatus(
            QCoreApplication::translate("OverviewPage", "Network state unavailable."));
        sharesGui.show();
        for (const QSize &size : sizes) {
            sharesGui.resize(size);
            QApplication::processEvents();
            QCOMPARE(sharesGui.size(), size);
            QString path = QString("%1/%2-overview-nsr-%3x%4.png")
                .arg(evidenceDir, locale).arg(size.width()).arg(size.height());
            QVERIFY2(sharesGui.grab().save(path), qPrintable(path));
            sharesGui.findChild<QToolButton *>("stakingPageActionNavButton")->click();
            QApplication::processEvents();
            path = QString("%1/%2-staking-%3x%4.png")
                .arg(evidenceDir, locale).arg(size.width()).arg(size.height());
            QVERIFY2(sharesGui.grab().save(path), qPrintable(path));
        }
    }
    qApp->removeTranslator(&translator);
}

void URITests::amountFieldEmitsValueChange()
{
    BitcoinAmountField field;
    QSignalSpy changed(&field, &BitcoinAmountField::textChanged);

    field.setValue(1234);

    QCOMPARE(field.value(), qint64(1234));
    QVERIFY(changed.count() > 0);
}

/*
struct SendCoinsRecipient
{
    QString address;
    QString label;
    qint64 amount;
};
*/

void URITests::uriTests()
{
    SendCoinsRecipient rv;
    QUrl uri;
    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?req-dontexist="));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?dontexist="));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?label=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString("Wikipedia Example Address"));
    QVERIFY(rv.amount == 0);

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=0.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QCOMPARE(rv.amount, qint64(10));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1.001"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QCOMPARE(rv.amount, qint64(10010));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=100&label=Wikipedia Example"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QCOMPARE(rv.amount, qint64(1000000));
    QVERIFY(rv.label == QString("Wikipedia Example"));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?message=Wikipedia Example Address"));
    QVERIFY(GUIUtil::parseBitcoinURI(uri, &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());

    QVERIFY(GUIUtil::parseBitcoinURI("nu://175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?message=Wikipedia Example Address", &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QVERIFY(rv.label == QString());
    QVERIFY(GUIUtil::parseBitcoinURI("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1.001", &rv));
    QVERIFY(rv.address == QString("175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W"));
    QCOMPARE(rv.amount, qint64(10010));
    QVERIFY(!GUIUtil::parseBitcoinURI("bitcoin:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W", &rv));

    // We currently dont implement the message paramenter (ok, yea, we break spec...)
    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?req-message=Wikipedia Example Address"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1,000&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=1,000.0&label=Wikipedia Example"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));

    uri.setUrl(QString("nu:175tWpb8K1S7NmH4Zx6rewF9WQrcZv245W?amount=0.00001"));
    QVERIFY(!GUIUtil::parseBitcoinURI(uri, &rv));
}

void URITests::addressValidatorTests()
{
    BitcoinAddressValidator validator;
    QString input("1abc");
    int position = input.size();
    QCOMPARE(validator.validate(input, position), QValidator::Acceptable);

    input = "1I0l";
    position = input.size();
    QCOMPARE(validator.validate(input, position), QValidator::Acceptable);
    QCOMPARE(input, QString("11o1"));

    input = "1abc?";
    position = input.size();
    QCOMPARE(validator.validate(input, position), QValidator::Invalid);

    input.clear();
    position = 0;
    QCOMPARE(validator.validate(input, position), QValidator::Intermediate);
}

void URITests::embeddedTranslationLoads()
{
    QTranslator translator;
    QVERIFY(translator.load(":/translations/bitcoin_pt_BR.qm"));
    QCOMPARE(translator.translate("BitcoinGUI", "&Overview"), QString("&Visão geral"));
    const QStringList catalogs = QDir(":/translations").entryList(
        QStringList() << "bitcoin_*.qm", QDir::Files, QDir::Name);
    QCOMPARE(catalogs.size(), 31);
    for (const QString &catalog : catalogs) {
        QTranslator availableCatalog;
        QVERIFY2(availableCatalog.load(":/translations/" + catalog), qPrintable(catalog));
    }
    QCOMPARE(GUIUtil::resolveLanguagePreference("unsupported_LOCALE", QString(), "fr_FR"),
             QString("en"));
    QTranslator unsupportedCatalog;
    QVERIFY(!unsupportedCatalog.load(":/translations/bitcoin_unsupported_LOCALE.qm"));
    QCOMPARE(GUIUtil::resolveLanguagePreference("unsupported_LOCALE", QString(), "fr_FR"),
             QString("en"));
    QCOMPARE(QCoreApplication::translate("BitcoinGUI", "&Send"), QString("&Send"));
}

void URITests::languagePreferencePriority()
{
    QCOMPARE(GUIUtil::resolveLanguagePreference("pt_BR", "en", "fr_FR"), QString("pt_BR"));
    QCOMPARE(GUIUtil::resolveLanguagePreference(QString(), "pt_BR", "en_US"), QString("pt_BR"));
    QCOMPARE(GUIUtil::resolveLanguagePreference(QString(), QString(), "pt_BR"), QString("pt_BR"));
    QCOMPARE(GUIUtil::resolveLanguagePreference("unsupported_LOCALE", "pt_BR", "fr_FR"), QString("en"));
}

void OverviewPageTests::stakingWalletStates()
{
    OverviewPage page;
    QLabel *status = page.findChild<QLabel *>("stakingStatus");
    QPushButton *action = page.findChild<QPushButton *>("stakingAction");
    QVERIFY(status);
    QVERIFY(action);
    QSignalSpy actionRequested(&page, &OverviewPage::stakingUnlockRequested);

    page.setStakingWalletState(true, true, true, true, false);
    QVERIFY(status->text().contains("Wallet locked"));
    QVERIFY(!action->isHidden());
    QCOMPARE(action->text(), QString("Unlock for staking"));
    page.show();
    action->setFocus();
    QTRY_VERIFY(action->hasFocus());
    QTest::keyClick(action, Qt::Key_Space);
    QCOMPARE(actionRequested.takeFirst().at(0).toBool(), true);

    page.setStakingWalletState(true, true, false, true, false);
    QCOMPARE(status->text(), QString("Unlocked for staking only"));
    QCOMPARE(action->text(), QString("Lock wallet"));
    action->click();
    QCOMPARE(actionRequested.takeFirst().at(0).toBool(), false);

    page.setStakingWalletState(true, true, false, false, false);
    QCOMPARE(status->text(), QString("Wallet fully unlocked"));
    QVERIFY(action->isHidden());

    page.setStakingWalletState(true, false, false, false, false);
    QCOMPARE(status->text(), QString("Wallet not encrypted"));
    QVERIFY(action->isHidden());

    page.setStakingWalletState(true, true, true, true, true);
    QCOMPARE(status->text(), QString("Wallet locked"));
    QVERIFY(action->isHidden());
    QLabel *configuration = page.findChild<QLabel *>("stakingConfigStatus");
    QVERIFY(configuration);
    QVERIFY(configuration->text().contains("Restart Nu"));
    QVERIFY(!configuration->isHidden());

    page.setStakingWalletState(true, true, true, true, false);
    page.setModel(0);
    QCOMPARE(status->text(), QString("Wallet status unavailable."));
    QVERIFY(action->isHidden());
    QVERIFY(!action->isEnabled());
    QVERIFY(!action->property("unlock").toBool());

    page.setStakingWalletState(false, true, true, false, false);
    QVERIFY(!status->isVisibleTo(&page));
    QVERIFY(!action->isVisibleTo(&page));
    page.setStakingWalletState(true, true, true, true, false);
    QVERIFY(status->isVisibleTo(&page));
    QVERIFY(action->isVisibleTo(&page));

    QLabel *network = page.findChild<QLabel *>("stakingNetworkStatus");
    QVERIFY(network);
    page.setStakingNetworkStatus("No network connections");
    QCOMPARE(network->text(), QString("No network connections"));
    page.setStakingNetworkStatus("Synchronizing with network");
    QCOMPARE(network->text(), QString("Synchronizing with network"));
    page.setStakingNetworkStatus("Network connected");
    QCOMPARE(network->text(), QString("Network connected"));

    const QString evidenceDir = qEnvironmentVariable("NUBITS_UI_EVIDENCE_DIR");
    if (evidenceDir.isEmpty())
        return;
    QVERIFY(QDir().mkpath(evidenceDir));

    struct Specimen {
        const char *name;
        bool shares;
        bool encrypted;
        bool locked;
        bool mintOnly;
        bool disabled;
    };
    const Specimen specimens[] = {
        {"nsr-locked", true, true, true, false, false},
        {"nsr-mint-only", true, true, false, true, false},
        {"nsr-disabled", true, true, true, false, true},
        {"nbt", false, false, false, false, false}
    };
    const QSize sizes[] = {QSize(750, 500), QSize(1100, 700)};
    for (const Specimen &specimen : specimens) {
        page.setStakingWalletState(specimen.shares, specimen.encrypted, specimen.locked,
                                   specimen.mintOnly, specimen.disabled);
        for (const QSize &size : sizes) {
            page.resize(size);
            QApplication::processEvents();
            const QString path = QString("%1/%2-%3x%4.png")
                .arg(evidenceDir, QString::fromLatin1(specimen.name))
                .arg(size.width()).arg(size.height());
            QVERIFY2(page.grab().save(path), qPrintable(path));
        }
    }
}

void PassphraseDialogTests::cancelKeepsWalletLocked()
{
    CWallet wallet;
    wallet.SetUnit('S');
    QVERIFY(wallet.Lock());
    WalletModel model(&wallet, 0);
    AskPassphraseDialog dialog(AskPassphraseDialog::Unlock);
    dialog.setModel(&model);
    QTimer::singleShot(0, &dialog, [&dialog]() {
        dialog.findChild<QDialogButtonBox *>("buttonBox")->button(QDialogButtonBox::Cancel)->click();
    });

    QCOMPARE(dialog.exec(), int(QDialog::Rejected));
    QCOMPARE(model.getEncryptionStatus(), WalletModel::Locked);
    QVERIFY(!model.isUnlockedForMintingOnly());
}

void PassphraseDialogTests::incorrectPassphraseKeepsWalletLocked()
{
    CWallet wallet;
    wallet.SetUnit('S');
    QVERIFY(wallet.Lock());
    WalletModel model(&wallet, 0);
    AskPassphraseDialog dialog(AskPassphraseDialog::Unlock);
    dialog.setModel(&model);
    bool errorClosed = false;
    QTimer::singleShot(0, &dialog, [&dialog]() {
        dialog.findChild<QLineEdit *>("passEdit1")->setText("incorrect");
        dialog.findChild<QDialogButtonBox *>("buttonBox")->button(QDialogButtonBox::Ok)->click();
    });
    QTimer::singleShot(50, &dialog, [&errorClosed]() {
        QMessageBox *message = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
        if (message && message->button(QMessageBox::Ok)) {
            errorClosed = true;
            message->button(QMessageBox::Ok)->click();
        }
    });
    QTimer::singleShot(100, &dialog, [&dialog]() { dialog.reject(); });

    QCOMPARE(dialog.exec(), int(QDialog::Rejected));
    QVERIFY(errorClosed);
    QCOMPARE(model.getEncryptionStatus(), WalletModel::Locked);
    QVERIFY(!model.isUnlockedForMintingOnly());
}
