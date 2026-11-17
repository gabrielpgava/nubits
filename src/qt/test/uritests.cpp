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

#include "uritests.h"
#include "../bitcoinamountfield.h"
#include "../bitcoingui.h"
#include "../guiutil.h"
#include "../walletmodel.h"
#include "../bitcoinaddressvalidator.h"
#include "../askpassphrasedialog.h"
#include "wallet.h"

void BitcoinGUITests::constructsWithoutModels()
{
    BitcoinGUI gui;
    QVERIFY(gui.findChild<OverviewPage *>());
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
