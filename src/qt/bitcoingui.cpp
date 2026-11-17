/*
 * Qt4 Peershares GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Bitcoin Developers 2011-2012
 * The PPCoin developers 2011-2013
 * The Peershares Developers 2013-2014
 */
#include "bitcoingui.h"
#include "transactiontablemodel.h"
#include "addressbookpage.h"
#include "sendcoinsdialog.h"
#include "signverifymessagedialog.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "editaddressdialog.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "addresstablemodel.h"
#include "transactionview.h"
#include "overviewpage.h"
#include "bitcoinunits.h"
#include "guiconstants.h"
#include "askpassphrasedialog.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "addnodesdialog.h"
#include "wallet.h"
#include "distributedivdialog.h"
#include "parkpage.h"
#include "votepage.h"
#include "autoupdate.h"

#ifdef Q_OS_MACOS
#include "macdockiconhandler.h"
#endif

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QMenu>
#include <QActionGroup>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QLocale>
#include <QMessageBox>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTimer>
#include <QScrollArea>

#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QUrl>

#include <iostream>

#include <QFont>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QPixmap>

static QIcon currencyIcon(unsigned char unit)
{
    const QPixmap asset(QString(":/images/%1")
        .arg(QChar::fromLatin1(static_cast<char>(unit))));
    if (asset.isNull())
        return QIcon();
    return QIcon(asset.copy(0, 0, asset.height(), asset.height()));
}

BitcoinGUI::BitcoinGUI(QWidget *parent):
    QMainWindow(parent),
    clientModel(0),
    walletModel(0),
    stakingPage(0),
    stakingPageWalletStatus(0),
    stakingPageNetworkStatus(0),
    stakingPageConfigurationStatus(0),
    stakingPageUnlockButton(0),
    unitSelectorButton(0),
    moreButton(0),
    moreMenu(0),
    exportPeercoinKeysAction(0),
    encryptWalletAction(0),
    unlockForMintingAction(0),
    changePassphraseAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    addNodesDialog(0),
    fPromptedNoPeers(false),
    fNoPeersTimerStarted(false)
{
    resize(980, 620);
    setMinimumSize(750, 500);
    setWindowTitle(tr("Nu"));
    setStyle(QStyleFactory::create("Fusion"));
#ifndef Q_OS_MACOS
    setWindowIcon(QIcon(":icons/nu"));
#else
    // nu: setting this breaks the visual styles for the toolbar, so turning off
    setUnifiedTitleAndToolBarOnMac(false);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the tray icon (or setup the dock icon)
    createTrayIcon();

    // Create tabs
    overviewPage = new OverviewPage();

    transactionsPage = new QWidget(this);
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->setContentsMargins(20, 20, 20, 20);
    transactionView = new TransactionView(this);
    vbox->addWidget(transactionView);
    transactionsPage->setLayout(vbox);

    addressBookPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::SendingTab);

    receiveCoinsPage = new AddressBookPage(AddressBookPage::ForEditing, AddressBookPage::ReceivingTab);

    sendCoinsPage = new SendCoinsDialog(this);

    messagePage = new SignVerifyMessageDialog(this);

    parkPage = new ParkPage(this);
    parkPage->setObjectName("parkingPage");

    votePage = new VotePage(this);

    stakingPage = new QWidget(this);
    stakingPage->setObjectName("stakingPage");
    QVBoxLayout *stakingLayout = new QVBoxLayout(stakingPage);
    stakingLayout->setContentsMargins(24, 24, 24, 24);
    QLabel *stakingTitle = new QLabel(tr("NuShares staking"), stakingPage);
    QFont stakingTitleFont = stakingTitle->font();
    stakingTitleFont.setPointSize(stakingTitleFont.pointSize() + 2);
    stakingTitleFont.setBold(true);
    stakingTitle->setFont(stakingTitleFont);
    QLabel *stakingExplanation = new QLabel(
        tr("Review wallet security, startup settings, and network state. These do not confirm staking participation or guarantee rewards."),
        stakingPage);
    stakingExplanation->setWordWrap(true);
    stakingPageWalletStatus = new QLabel(stakingPage);
    stakingPageWalletStatus->setObjectName("stakingPageWalletStatus");
    stakingPageNetworkStatus = new QLabel(stakingPage);
    stakingPageNetworkStatus->setObjectName("stakingPageNetworkStatus");
    stakingPageConfigurationStatus = new QLabel(stakingPage);
    stakingPageConfigurationStatus->setObjectName("stakingPageConfigurationStatus");
    stakingPageConfigurationStatus->setWordWrap(true);
    stakingPageUnlockButton = new QPushButton(stakingPage);
    stakingPageUnlockButton->setObjectName("stakingPageAction");
    stakingPageUnlockButton->setVisible(false);
    stakingLayout->addWidget(stakingTitle);
    stakingLayout->addWidget(stakingExplanation);
    stakingLayout->addSpacing(16);
    stakingLayout->addWidget(stakingPageWalletStatus);
    stakingLayout->addWidget(stakingPageNetworkStatus);
    stakingLayout->addWidget(stakingPageConfigurationStatus);
    stakingLayout->addWidget(stakingPageUnlockButton);
    stakingLayout->addStretch(1);
    connect(overviewPage, &OverviewPage::stakingDetailsChanged, this,
        [this](const QString &walletStatus, const QString &networkStatus,
               const QString &configurationStatus, const QString &actionText,
               bool actionVisible, bool unlock) {
            stakingPageWalletStatus->setText(walletStatus);
            stakingPageNetworkStatus->setText(networkStatus);
            stakingPageConfigurationStatus->setText(configurationStatus);
            stakingPageConfigurationStatus->setVisible(!configurationStatus.isEmpty());
            stakingPageUnlockButton->setText(actionText);
            stakingPageUnlockButton->setProperty("unlock", unlock);
            stakingPageUnlockButton->setVisible(actionVisible);
            stakingPageUnlockButton->setEnabled(actionVisible);
        });
    connect(stakingPageUnlockButton, &QPushButton::clicked, this, [this]() {
        unlockForMinting(stakingPageUnlockButton->property("unlock").toBool());
    });

    centralWidget = new QStackedWidget(this);
    centralWidget->setObjectName("centralWidget");
    centralWidget->setAttribute(Qt::WA_StyledBackground, true);
    centralWidget->setAutoFillBackground(true);

    overviewPage->setAttribute(Qt::WA_StyledBackground, true);
    overviewPage->setAutoFillBackground(true);
    transactionsPage->setAttribute(Qt::WA_StyledBackground, true);
    transactionsPage->setAutoFillBackground(true);
    addressBookPage->setAttribute(Qt::WA_StyledBackground, true);
    addressBookPage->setAutoFillBackground(true);
    receiveCoinsPage->setAttribute(Qt::WA_StyledBackground, true);
    receiveCoinsPage->setAutoFillBackground(true);
    sendCoinsPage->setAttribute(Qt::WA_StyledBackground, true);
    sendCoinsPage->setAutoFillBackground(true);
#ifdef FIRST_CLASS_MESSAGING
    messagePage->setAttribute(Qt::WA_StyledBackground, true);
    messagePage->setAutoFillBackground(true);
#endif
    parkPage->setAttribute(Qt::WA_StyledBackground, true);
    parkPage->setAutoFillBackground(true);
    votePage->setAttribute(Qt::WA_StyledBackground, true);
    votePage->setAutoFillBackground(true);

    auto addScrollablePage = [this](QWidget *page, const QString &name) {
        QScrollArea *scrollArea = new QScrollArea(centralWidget);
        scrollArea->setObjectName(name);
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidget(page);
        centralWidget->addWidget(scrollArea);
    };
    addScrollablePage(overviewPage, "overviewPageScrollArea");
    addScrollablePage(transactionsPage, "transactionsPageScrollArea");
    addScrollablePage(addressBookPage, "addressBookPageScrollArea");
    addScrollablePage(receiveCoinsPage, "receiveCoinsPageScrollArea");
    addScrollablePage(sendCoinsPage, "sendCoinsPageScrollArea");
#ifdef FIRST_CLASS_MESSAGING
    addScrollablePage(messagePage, "messagePageScrollArea");
#endif
    addScrollablePage(parkPage, "parkingPageScrollArea");
    addScrollablePage(votePage, "votePageScrollArea");
    addScrollablePage(stakingPage, "stakingPageScrollArea");

    // Create dedicated vertical sidebar and integrate with mainContainer
    createToolBars();

    // Create status bar
    statusBar();

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setMinimumWidth(56);
    frameBlocks->setMaximumWidth(56);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    labelEncryptionIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelConnectionsIcon->installEventFilter(this);
    labelConnectionsIcon->setCursor(Qt::PointingHandCursor);
    labelBlocksIcon = new QLabel();
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelEncryptionIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBarLabel->setProperty("class", "progressBarLabel");
    progressBar = new QProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);
    progressBar->setProperty("class", "statusBarStyle");

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    syncIconMovie = new QMovie(":/movies/update_spinner", "mng", this);

    // Clicking on a transaction on the overview page simply sends you to transaction history page
    connect(overviewPage, SIGNAL(transactionClicked(QModelIndex)), this, SLOT(gotoHistoryPage()));
    connect(overviewPage, SIGNAL(stakingUnlockRequested(bool)), this, SLOT(unlockForMinting(bool)));

    // Doubleclicking on a transaction on the transaction history page shows details
    connect(transactionView, SIGNAL(doubleClicked(QModelIndex)), transactionView, SLOT(showDetails()));

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    gotoOverviewPage();
    overviewPage->refreshStakingStatus();
}

BitcoinGUI::~BitcoinGUI()
{
#ifdef Q_OS_MACOS
    delete appMenuBar;
#endif
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setObjectName("overviewAction");
    overviewAction->setToolTip(tr("Show general overview of holdings"));
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setObjectName("historyAction");
    historyAction->setToolTip(tr("Browse transaction history"));
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Addresses"), this);
    addressBookAction->setObjectName("addressBookAction");
    addressBookAction->setToolTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setObjectName("receiveCoinsAction");
    receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving %1").arg(BitcoinUnits::baseName()));
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setObjectName("sendCoinsAction");
    sendCoinsAction->setToolTip(tr("Send coins to a %1 address").arg(BitcoinUnits::baseName()));
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    messageAction = new QAction(QIcon(":/icons/edit"), tr("Sign/Verify &message"), this);
    messageAction->setToolTip(tr("Prove you control an address"));
#ifdef FIRST_CLASS_MESSAGING
    messageAction->setCheckable(true);
#endif
    tabGroup->addAction(messageAction);

    parkAction = new QAction(QIcon(":/icons/tx_mined"), tr("&Parking"), this);
    parkAction->setObjectName("parkAction");
    parkAction->setToolTip(tr("Manage NuBits parked in the wallet"));
    parkAction->setCheckable(true);
    parkAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    parkAction->setVisible(false);
    tabGroup->addAction(parkAction);

    voteAction = new QAction(QIcon(":/icons/edit"), tr("&Vote"), this);
    voteAction->setObjectName("voteAction");
    voteAction->setToolTip(tr("Change your vote in the DAO"));
    voteAction->setCheckable(true);
    voteAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_7));
    voteAction->setVisible(false);
    tabGroup->addAction(voteAction);

    stakingPageAction = new QAction(QIcon(":/icons/lock_open"), tr("&Staking"), this);
    stakingPageAction->setObjectName("stakingPageAction");
    stakingPageAction->setToolTip(tr("Review NuShares staking wallet and network status"));
    stakingPageAction->setCheckable(true);
    stakingPageAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_8));
    stakingPageAction->setVisible(false);
    tabGroup->addAction(stakingPageAction);

    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(messageAction, SIGNAL(triggered()), this, SLOT(gotoMessagePage()));
    connect(parkAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(parkAction, SIGNAL(triggered()), this, SLOT(gotoParkPage()));
    connect(voteAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(voteAction, SIGNAL(triggered()), this, SLOT(gotoVotePage()));
    connect(stakingPageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(stakingPageAction, SIGNAL(triggered()), this, SLOT(gotoStakingPage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setToolTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/nu"), tr("&About %1").arg(qApp->applicationName()), this);
    aboutAction->setToolTip(tr("Show information about Nu"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    checkUpdatesAction = new QAction(tr("Check for &Updates..."), this);
    checkUpdatesAction->setToolTip(tr("Check GitHub for newer versions of NuBits"));
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Settings..."), this);
    optionsAction->setObjectName("optionsAction");
    optionsAction->setToolTip(tr("Open Nu settings"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    addNodesAction = new QAction(QIcon(":/icons/connect_4"), tr("&Network peers..."), this);
    addNodesAction->setToolTip(tr("View connected peers and add a peer manually"));
    toggleHideAction = new QAction(QIcon(":/icons/nu"), tr("Show/Hide &Nu"), this);
    toggleHideAction->setToolTip(tr("Show or hide the Nu window"));
    exportAction = new QAction(QIcon(":/icons/export"), tr("&Export..."), this);
    exportAction->setToolTip(tr("Export the data in the current tab to a file"));
    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Portfolio"), this);
    encryptWalletAction->setToolTip(tr("Encrypt or decrypt portfolio"));
    encryptWalletAction->setCheckable(true);
    unlockForMintingAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Unlock Wallet for Minting Only"), this);
    unlockForMintingAction->setToolTip(tr("Unlock wallet only for minting. Sending NuShares will still require the passphrase."));
    unlockForMintingAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet"), this);
    backupWalletAction->setToolTip(tr("Backup portfolio to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase"), this);
    changePassphraseAction->setToolTip(tr("Change the passphrase used for portfolio encryption"));
    openRPCConsoleAction = new QAction(tr("&Debug window"), this);
    openRPCConsoleAction->setToolTip(tr("Open debugging and diagnostic console"));
    exportPeercoinKeysAction = new QAction(QIcon(":/icons/export"), tr("&Export NuBits keys..."), this);
    exportPeercoinKeysAction->setToolTip(tr("Export NuBits private keys via RPC"));
    distributeDividendsAction = new QAction(tr("&Distribute dividends..."), this);
    distributeDividendsAction->setToolTip(tr("Distribute dividends to share holders"));

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(addNodesAction, SIGNAL(triggered()), this, SLOT(addNodesClicked()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(checkUpdatesAction, SIGNAL(triggered()), this, SLOT(checkForUpdatesClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), this, SLOT(encryptWallet(bool)));
    connect(unlockForMintingAction, SIGNAL(triggered(bool)), this, SLOT(unlockForMinting(bool)));
    connect(backupWalletAction, SIGNAL(triggered()), this, SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), this, SLOT(changePassphrase()));
    connect(exportPeercoinKeysAction, SIGNAL(triggered()), this, SLOT(exportPeercoinKeys()));
    connect(distributeDividendsAction, SIGNAL(triggered()), this, SLOT(distributeDividendsClicked()));
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MACOS
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(backupWalletAction);
    file->addAction(exportAction);
#ifndef FIRST_CLASS_MESSAGING
    file->addAction(messageAction);
#endif
    file->addSeparator();
    file->addAction(quitAction);

    sharesMenu = appMenuBar->addMenu(tr("Sh&ares"));
    sharesMenu->addAction(exportPeercoinKeysAction);
    sharesMenu->addAction(distributeDividendsAction);

    unitMenu = appMenuBar->addMenu(tr("&Unit"));
    unitMenu->setObjectName("unitMenu");

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    settings->addAction(encryptWalletAction);
    settings->addAction(unlockForMintingAction);
    settings->addAction(changePassphraseAction);
    settings->addSeparator();
    settings->addAction(addNodesAction);
    settings->addAction(optionsAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(checkUpdatesAction);
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcoinGUI::createToolBars()
{
    // Main container holding the sidebar and pages
    QWidget *mainContainer = new QWidget(this);
    mainContainer->setObjectName("mainContainer");
    mainContainer->setAttribute(Qt::WA_StyledBackground, true);
    mainContainer->setAutoFillBackground(true);

    QHBoxLayout *mainLayout = new QHBoxLayout(mainContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Keep frequent wallet tasks visible and group occasional tools separately.
    QFrame *sidebar = new QFrame(mainContainer);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(180);
    sidebar->setAttribute(Qt::WA_StyledBackground, true);
    sidebar->setAutoFillBackground(true);

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(10, 14, 10, 14);
    sidebarLayout->setSpacing(4);

    unitSelectorButton = new QToolButton(sidebar);
    unitSelectorButton->setObjectName("walletUnitSelector");
    unitSelectorButton->setCursor(Qt::PointingHandCursor);
    unitSelectorButton->setFixedHeight(54);
    unitSelectorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    unitSelectorButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    unitSelectorButton->setIconSize(QSize(28, 28));
    unitSelectorButton->setPopupMode(QToolButton::InstantPopup);
    unitSelectorButton->setText(tr("Select wallet"));
    unitSelectorButton->setAccessibleName(tr("Active wallet asset"));
    unitSelectorButton->setToolTip(tr("Choose a loaded NuBits or NuShares wallet"));
    unitSelectorButton->setMenu(unitMenu);
    sidebarLayout->addWidget(unitSelectorButton);
    sidebarLayout->addSpacing(8);

    QScrollArea *navigationScroll = new QScrollArea(sidebar);
    navigationScroll->setObjectName("navigationScrollArea");
    navigationScroll->setWidgetResizable(true);
    navigationScroll->setFrameShape(QFrame::NoFrame);
    navigationScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QWidget *navigationContent = new QWidget(navigationScroll);
    QVBoxLayout *navigationLayout = new QVBoxLayout(navigationContent);
    navigationLayout->setContentsMargins(0, 0, 0, 0);
    navigationLayout->setSpacing(4);
    navigationScroll->setWidget(navigationContent);
    sidebarLayout->addWidget(navigationScroll, 1);

    // Navigation buttons helper
    auto addNavButton = [navigationContent, navigationLayout](QAction *action) {
        QToolButton *btn = new QToolButton(navigationContent);
        btn->setObjectName(action->objectName() + "NavButton");
        btn->setDefaultAction(action);
        btn->setVisible(action->isVisible());
        btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        btn->setIconSize(QSize(18, 18));
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(40);
        btn->setCursor(Qt::PointingHandCursor);
        QObject::connect(action, &QAction::changed, btn, [action, btn]() {
            btn->setVisible(action->isVisible());
        });
        navigationLayout->addWidget(btn);
        return btn;
    };

    addNavButton(overviewAction);
    addNavButton(sendCoinsAction);
    addNavButton(receiveCoinsAction);
    addNavButton(historyAction);
    addNavButton(addressBookAction);
    addNavButton(parkAction);
    addNavButton(stakingPageAction);

    moreButton = new QToolButton(navigationContent);
    moreButton->setObjectName("moreToolsButton");
    moreButton->setText(tr("More"));
    moreButton->setAccessibleName(tr("More tools"));
    moreButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    moreButton->setIcon(QIcon(":/icons/edit"));
    moreButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    moreButton->setFixedHeight(40);
    moreButton->setPopupMode(QToolButton::InstantPopup);
    moreMenu = new QMenu(moreButton);
    moreMenu->addAction(messageAction);
    moreMenu->addAction(voteAction);
    moreMenu->addSeparator();
    moreMenu->addAction(exportPeercoinKeysAction);
    moreMenu->addAction(distributeDividendsAction);
    moreMenu->addAction(addNodesAction);
    moreMenu->addAction(openRPCConsoleAction);
    moreMenu->addSeparator();
    moreMenu->addAction(backupWalletAction);
    moreMenu->addAction(exportAction);
    moreMenu->addAction(encryptWalletAction);
    moreMenu->addAction(unlockForMintingAction);
    moreMenu->addAction(changePassphraseAction);
    moreButton->setMenu(moreMenu);
    navigationLayout->addWidget(moreButton);

    QToolButton *settingsButton = new QToolButton(sidebar);
    settingsButton->setObjectName("settingsButton");
    settingsButton->setDefaultAction(optionsAction);
    settingsButton->setAccessibleName(tr("Settings"));
    settingsButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    settingsButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    settingsButton->setFixedHeight(40);
    sidebarLayout->addWidget(settingsButton);

    mainLayout->addWidget(sidebar);
    mainLayout->addWidget(centralWidget, 1);

    setCentralWidget(mainContainer);
}

void BitcoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        if(clientModel->isTestNet())
        {
            QString title_testnet = windowTitle() + QString(" ") + tr("[testnet]");
            setWindowTitle(title_testnet);
#ifndef Q_OS_MACOS
            setWindowIcon(QIcon(":icons/nu_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/nu_testnet"));
#endif
            if(trayIcon)
            {
                trayIcon->setToolTip(title_testnet);
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
            }
        }

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks());
        connect(clientModel, SIGNAL(numBlocksChanged(int)), this, SLOT(setNumBlocks(int)));

        // Report errors from network/worker thread
        connect(clientModel, SIGNAL(error(QString,QString, bool)), this, SLOT(error(QString,QString,bool)));

        rpcConsole->setClientModel(clientModel);
        votePage->setClientModel(clientModel);

        if (!fNoPeersTimerStarted)
        {
            fNoPeersTimerStarted = true;
            QTimer::singleShot(45000, this, SLOT(checkNoPeersOnStartup()));
        }
    }
}

void BitcoinGUI::setWalletModel(WalletModel *walletModel)
{
    this->walletModel = walletModel;
    if(walletModel)
    {
        // nubit: set current base unit
        BitcoinUnits::baseUnit = walletModel->getUnit();

        // nubit: update the send and receive tooltip text to display the proper unit (NuShares/NuBits)
        receiveCoinsAction->setToolTip(tr("Show the list of addresses for receiving %1").arg(BitcoinUnits::baseName()));
        sendCoinsAction->setToolTip(tr("Send coins to a %1 address").arg(BitcoinUnits::baseName()));

        // Report errors from wallet thread
        connect(walletModel, SIGNAL(error(QString,QString,bool)), this, SLOT(error(QString,QString,bool)));

        // Put transaction list in tabs
        transactionView->setModel(walletModel);

        overviewPage->setModel(walletModel);
        addressBookPage->setModel(walletModel->getAddressTableModel());
        receiveCoinsPage->setModel(walletModel->getAddressTableModel());
        sendCoinsPage->setModel(walletModel);
        messagePage->setModel(walletModel);
        rpcConsole->setModel(walletModel);
        parkPage->setModel(walletModel);
        votePage->setModel(walletModel);

        parkAction->setVisible(walletModel->getUnit() != 'S');

        voteAction->setVisible(walletModel->getUnit() == 'S');
        stakingPageAction->setVisible(walletModel->getUnit() == 'S');
        exportPeercoinKeysAction->setVisible(walletModel->getUnit() == 'S');
        distributeDividendsAction->setVisible(walletModel->getUnit() == 'S');
        updateUnitSelector();

        if (walletModel->getUnit() != 'S' && centralWidget->currentWidget() == scrollAreaForPage(votePage))
            gotoOverviewPage();

        if (walletModel->getUnit() == 'S' && centralWidget->currentWidget() == scrollAreaForPage(parkPage))
            gotoOverviewPage();
        if (walletModel->getUnit() != 'S' && centralWidget->currentWidget() == scrollAreaForPage(stakingPage))
            gotoOverviewPage();

        sharesMenu->setEnabled(walletModel->getUnit() == 'S');

        setEncryptionStatus(walletModel->getEncryptionStatus());
        connect(walletModel, SIGNAL(encryptionStatusChanged(int)), this, SLOT(setEncryptionStatus(int)));
        overviewPage->refreshStakingStatus();

        // Balloon popup for new transaction
        connect(walletModel->getTransactionTableModel(), SIGNAL(rowsInserted(QModelIndex,int,int)),
                this, SLOT(incomingTransaction(QModelIndex,int,int)));

        // Ask for passphrase if needed
        connect(walletModel, SIGNAL(requireUnlock()), this, SLOT(unlockWallet()));

        if (isMinimized() || isHidden())
            walletModel->setPollInterval(MODEL_UPDATE_DELAY_BACKGROUND);

        // nubit: change the client stylesheet when the unit context changes
        QFile stylesheet(QString(":/styles/%1.qss").arg(QChar::fromLatin1(static_cast<char>(walletModel->getUnit()))));
        if (stylesheet.open(QFile::ReadOnly))
        {
            QString setSheet = QLatin1String(stylesheet.readAll());
            qApp->setStyleSheet(setSheet);
        }

        // Embed application fonts
        int fontId = QFontDatabase::addApplicationFont(":/fonts/rob_r");
        if (fontId != -1)
        {
            QStringList families = QFontDatabase::applicationFontFamilies(fontId);
            if (!families.isEmpty())
            {
                QFont appFont(families.at(0), 12);
                qApp->setFont(appFont);
            }
        }

        changeUnitActions.clear();
        BOOST_FOREACH(CWallet *wallet, setpwalletRegistered)
        {
            QString unitString(QChar::fromLatin1(static_cast<char>(wallet->Unit())));
            QAction *action = new QAction(currencyIcon(wallet->Unit()),
                                          BitcoinUnits::baseName(wallet->Unit()), this);
            action->setCheckable(true);
            if (unitString == QString(QChar::fromLatin1(static_cast<char>(walletModel->getWallet()->Unit()))))
                action->setChecked(true);

            changeUnitActions.append(action);

            const QString walletUnit = unitString;
            connect(action, &QAction::triggered, this, [this, walletUnit]() {
                changeUnit(walletUnit);
            });
        }
        unitMenu->clear();
        for (int i=0; i < changeUnitActions.size(); ++i)
            unitMenu->addAction(changeUnitActions[i]);

    }
}

void BitcoinGUI::changeUnit(const QString &unit)
{
    WalletModel *oldWalletModel = this->walletModel;
    OptionsModel *optionsModel = oldWalletModel->getOptionsModel();
    WalletModel *newWalletModel = NULL;

    BOOST_FOREACH(CWallet *wallet, setpwalletRegistered)
    {
        if (QString(QChar::fromLatin1(static_cast<char>(wallet->Unit()))) == unit)
        {
            newWalletModel = new WalletModel(wallet, optionsModel);
            break;
        }
    }
    if (!newWalletModel)
        throw std::runtime_error("Wallet not found");

    setUpdatesEnabled(false);
    setWalletModel(newWalletModel);
    setUpdatesEnabled(true);
    delete oldWalletModel;
}

void BitcoinGUI::createTrayIcon()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MACOS
    trayIcon = new QSystemTrayIcon(this);
    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);
    trayIcon->setToolTip(tr("Nu client"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
    trayIcon->show();
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    trayIconMenu = dockIconHandler->dockMenu();
    dockIconHandler->setMainWindow((QMainWindow *)this);
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(messageAction);
#ifndef FIRST_CLASS_MESSAGING
    trayIconMenu->addSeparator();
#endif
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(addNodesAction);
    trayIconMenu->addAction(optionsAction);
#ifndef Q_OS_MACOS // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif

    notificator = new Notificator(tr("Nu-qt"), trayIcon);

    QTimer *updateCheckTimer = new QTimer(this);
    connect(updateCheckTimer, SIGNAL(timeout()), this, SLOT(checkPeriodicUpdate()));
    updateCheckTimer->start(60000); // Check update status every 60 seconds
}

#ifndef Q_OS_MACOS
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers "show/hide Peershares"
        toggleHideAction->trigger();
    }
}
#endif

void BitcoinGUI::toggleHidden()
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else
        hide();
}

void BitcoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::addNodesClicked()
{
    if (!addNodesDialog)
        addNodesDialog = new AddNodesDialog(this);
    addNodesDialog->refreshPeers();
    addNodesDialog->refreshCustomNodes();
    addNodesDialog->show();
    addNodesDialog->raise();
    addNodesDialog->activateWindow();
}

bool BitcoinGUI::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == labelConnectionsIcon && event->type() == QEvent::MouseButtonRelease)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            addNodesClicked();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void BitcoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void BitcoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelConnectionsIcon->setToolTip(tr("%n active connection(s) to Nu network", "", count) + QString("\n") + tr("Click to manage network peers"));
    if (overviewPage)
        setNumBlocks(clientModel ? clientModel->getNumBlocks() : 0);
}

void BitcoinGUI::setNumBlocks(int count)
{
    // don't show / hide progressBar and it's label if we have no connection(s) to the network
    if (!clientModel || clientModel->getNumConnections() == 0)
    {
        if (overviewPage)
            overviewPage->setStakingNetworkStatus(tr("No network connections"));
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);

        return;
    }

    int nTotalBlocks = clientModel->getNumBlocksOfPeers();
    QString tooltip;
    if (overviewPage)
    {
        const QString warnings = clientModel->getStatusBarWarnings();
        if (!warnings.isEmpty())
            overviewPage->setStakingNetworkStatus(tr("Network warning"));
        else if (clientModel->inInitialBlockDownload() || count < nTotalBlocks)
            overviewPage->setStakingNetworkStatus(tr("Synchronizing with network"));
        else
            overviewPage->setStakingNetworkStatus(tr("Network connected"));
    }

    if(count < nTotalBlocks)
    {
        int nRemainingBlocks = nTotalBlocks - count;
        float nPercentageDone = count / (nTotalBlocks * 0.01f);

        if (clientModel->getStatusBarWarnings() == "")
        {
            progressBarLabel->setText(tr("Synchronizing with network..."));
            progressBarLabel->setVisible(true);
            progressBar->setFormat(tr("~%n block(s) remaining", "", nRemainingBlocks));
            progressBar->setMaximum(nTotalBlocks);
            progressBar->setValue(count);
            progressBar->setVisible(true);
        }
        else
        {
            progressBarLabel->setText(clientModel->getStatusBarWarnings());
            progressBarLabel->setVisible(true);
            progressBar->setVisible(false);
        }
        tooltip = tr("Downloaded %1 of %2 blocks of transaction history (%3% done).").arg(count).arg(nTotalBlocks).arg(nPercentageDone, 0, 'f', 2);
    }
    else
    {
        if (clientModel->getStatusBarWarnings() == "")
            progressBarLabel->setVisible(false);
        else
        {
            progressBarLabel->setText(clientModel->getStatusBarWarnings());
            progressBarLabel->setVisible(true);
        }
        progressBar->setVisible(false);
        tooltip = tr("Downloaded %1 blocks of transaction history.").arg(count);
    }

    QDateTime now = QDateTime::currentDateTime();
    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    int secs = lastBlockDate.secsTo(now);
    QString text;

    // Represent time from last generated block in human readable text
    if(secs <= 0)
    {
        // Fully up to date. Leave text empty.
    }
    else if(secs < 60)
    {
        text = tr("%n second(s) ago","",secs);
    }
    else if(secs < 60*60)
    {
        text = tr("%n minute(s) ago","",secs/60);
    }
    else if(secs < 24*60*60)
    {
        text = tr("%n hour(s) ago","",secs/(60*60));
    }
    else
    {
        text = tr("%n day(s) ago","",secs/(60*60*24));
    }

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60 && count >= nTotalBlocks)
    {
        tooltip = tr("Up to date") + QString(".\n") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
    }
    else
    {
        tooltip = tr("Catching up...") + QString("\n") + tooltip;
        labelBlocksIcon->setMovie(syncIconMovie);
        syncIconMovie->start();
        if (isMinimized() || isHidden())
            syncIconMovie->setPaused(true);
    }

    if(!text.isEmpty())
    {
        tooltip += QString("\n");
        tooltip += tr("Last received block was generated %1.").arg(text);
    }

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void BitcoinGUI::error(const QString &title, const QString &message, bool modal)
{
    // Report errors from network/worker thread
    if(modal)
    {
        QMessageBox::critical(this, title, message, QMessageBox::Ok, QMessageBox::Ok);
    } else {
        notificator->notify(Notificator::Critical, title, message);
    }
}

void BitcoinGUI::updatePollingInterval(bool background)
{
    int interval = background ? MODEL_UPDATE_DELAY_BACKGROUND : MODEL_UPDATE_DELAY;

    if (walletModel)
        walletModel->setPollInterval(interval);

    if (votePage)
        votePage->setPollInterval(interval);

    if (background)
    {
        // Pause sync icon spinner movie to eliminate continuous drawing/CPU load in background
        if (syncIconMovie && syncIconMovie->state() == QMovie::Running)
            syncIconMovie->setPaused(true);

        // Pause unnecessary GUI updates and repaints while minimized / in tray
        if (centralWidget)
            centralWidget->setUpdatesEnabled(false);
    }
    else
    {
        // Re-enable interface updates and trigger a repaint
        if (centralWidget)
        {
            centralWidget->setUpdatesEnabled(true);
            centralWidget->update();
        }

        // Resume sync spinner movie if catching up
        if (syncIconMovie && syncIconMovie->state() == QMovie::Paused)
            syncIconMovie->setPaused(false);
    }
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MACOS // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }

        if (isMinimized())
        {
            updatePollingInterval(true);
        }
        else if (!isHidden())
        {
            updatePollingInterval(false);
        }
    }
#endif
}

void BitcoinGUI::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!isMinimized())
        updatePollingInterval(false);
}

void BitcoinGUI::hideEvent(QHideEvent *event)
{
    QMainWindow::hideEvent(event);
    updatePollingInterval(true);
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MACOS // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            qApp->quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void BitcoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage =
        tr("This transaction requires a network fee of %1. "
          "Do you want to pay the fee and continue?").arg(
                BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Sending..."), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void BitcoinGUI::incomingTransaction(const QModelIndex & parent, int start, int end)
{
    if(!walletModel || !clientModel)
        return;
    TransactionTableModel *ttm = walletModel->getTransactionTableModel();
    qint64 amount = ttm->index(start, TransactionTableModel::Amount, parent)
                    .data(Qt::EditRole).toULongLong();
    if(!clientModel->inInitialBlockDownload())
    {
        // On new transaction, make an info balloon
        // Unless the initial block download is in progress, to prevent balloon-spam
        QString date = ttm->index(start, TransactionTableModel::Date, parent)
                        .data().toString();
        QString type = ttm->index(start, TransactionTableModel::Type, parent)
                        .data().toString();
        QString address = ttm->index(start, TransactionTableModel::ToAddress, parent)
                        .data().toString();
        QIcon icon = qvariant_cast<QIcon>(ttm->index(start,
                            TransactionTableModel::ToAddress, parent)
                        .data(Qt::DecorationRole));

        notificator->notify(Notificator::Information,
                            (amount)<0 ? tr("Sent transaction") :
                                         tr("Incoming transaction"),
                              tr("Date: %1\n"
                                 "Amount: %2\n"
                                 "Type: %3\n"
                                 "Address: %4\n")
                              .arg(date)
                              .arg(BitcoinUnits::formatWithUnit(walletModel->getOptionsModel()->getDisplayUnit(), amount, true))
                              .arg(type)
                              .arg(address), icon);
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(overviewPage));

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(transactionsPage));

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), transactionView, SLOT(exportClicked()));
}

void BitcoinGUI::gotoAddressBookPage()
{
    addressBookAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(addressBookPage));

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), addressBookPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(receiveCoinsPage));

    exportAction->setEnabled(true);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
    connect(exportAction, SIGNAL(triggered()), receiveCoinsPage, SLOT(exportClicked()));
}

void BitcoinGUI::gotoSendCoinsPage()
{
    sendCoinsAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(sendCoinsPage));

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoMessagePage()
{
#ifdef FIRST_CLASS_MESSAGING
    messageAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(messagePage));

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
#else
    messagePage->show();
    messagePage->setFocus();
#endif
}

void BitcoinGUI::gotoParkPage()
{
    parkAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(parkPage));

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoVotePage()
{
    voteAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(votePage));

    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::gotoStakingPage()
{
    stakingPageAction->setChecked(true);
    centralWidget->setCurrentWidget(scrollAreaForPage(stakingPage));
    exportAction->setEnabled(false);
    disconnect(exportAction, SIGNAL(triggered()), 0, 0);
}

void BitcoinGUI::updateUnitSelector()
{
    if (!unitSelectorButton || !walletModel)
        return;
    const unsigned char unit = walletModel->getUnit();
    const QString asset = BitcoinUnits::baseName(unit);
    unitSelectorButton->setIcon(currencyIcon(unit));
    unitSelectorButton->setText(asset);
}

QWidget *BitcoinGUI::scrollAreaForPage(QWidget *page) const
{
    for (int i = 0; i < centralWidget->count(); ++i) {
        QScrollArea *scrollArea = qobject_cast<QScrollArea *>(centralWidget->widget(i));
        if (scrollArea && scrollArea->widget() == page)
            return scrollArea;
    }
    return page;
}

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        gotoSendCoinsPage();
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            sendCoinsPage->handleURI(uri.toString());
        }
    }

    event->acceptProposedAction();
}

void BitcoinGUI::handleURI(QString strURI)
{
    gotoSendCoinsPage();
    sendCoinsPage->handleURI(strURI);

    if(!isActiveWindow())
        activateWindow();

    showNormalIfMinimized();
}

void BitcoinGUI::setEncryptionStatus(int status)
{
    bool fShares = (walletModel->getUnit() == 'S');
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        unlockForMintingAction->setEnabled(false);
        unlockForMintingAction->setChecked(false);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(walletModel->isUnlockedForMintingOnly()? tr("Wallet is <b>encrypted</b> and currently <b>unlocked for block minting only</b>") : tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        unlockForMintingAction->setEnabled(fShares && walletModel->isUnlockedForMintingOnly());
        unlockForMintingAction->setChecked(fShares && walletModel->isUnlockedForMintingOnly());
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        unlockForMintingAction->setEnabled(fShares);
        unlockForMintingAction->setChecked(false);
        break;
    }
}

void BitcoinGUI::encryptWallet(bool status)
{
    if(!walletModel)
        return;
    AskPassphraseDialog dlg(status ? AskPassphraseDialog::Encrypt:
                                     AskPassphraseDialog::Decrypt, this);
    dlg.setModel(walletModel);
    dlg.exec();

    setEncryptionStatus(walletModel->getEncryptionStatus());
}

void BitcoinGUI::unlockForMinting(bool status)
{
    if(!walletModel)
        return;

    if (status)
    {
        if(walletModel->getEncryptionStatus() != WalletModel::Locked)
            return;

        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();

        if(walletModel->getEncryptionStatus() != WalletModel::Unlocked)
            return;

        walletModel->setUnlockedForMintingOnly(true);
    }
    else
    {
        if(walletModel->getEncryptionStatus() != WalletModel::Unlocked)
            return;

        if (!walletModel->isUnlockedForMintingOnly())
            return;

        walletModel->setWalletLocked(true);
        walletModel->setUnlockedForMintingOnly(false);
    }
    if (overviewPage)
        overviewPage->refreshStakingStatus();
}

void BitcoinGUI::backupWallet()
{
    QString saveDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QString filename = QFileDialog::getSaveFileName(this, tr("Backup Portfolio"), saveDir, tr("Portfolio Data (*.dat)"));
    if(!filename.isEmpty()) {
        if(!walletModel->backupWallet(filename)) {
            QMessageBox::warning(this, tr("Backup Failed"), tr("There was an error trying to save the portfolio data to the new location."));
        }
    }
}

void BitcoinGUI::changePassphrase()
{
    AskPassphraseDialog dlg(AskPassphraseDialog::ChangePass, this);
    dlg.setModel(walletModel);
    dlg.exec();
}

void BitcoinGUI::unlockWallet()
{
    if(!walletModel)
        return;
    // Unlock wallet when requested by wallet model
    if(walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();
    }
}

void BitcoinGUI::exportPeercoinKeys()
{
    QMessageBox::StandardButton reply;

    QString sQuestion = tr("All your private keys will be converted and imported into your NuBits wallet.\n\nThe wallet must be running, unlocked (if it was encrypted) and accept RPC commands.\n\nThis process may take several minutes because the blockchain will be scanned for transactions on all the imported keys.\n\nDo you want to proceed?");
    reply = QMessageBox::warning(this, tr("NuBits keys export confirmation"), sQuestion, QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
        return;

    bool fMustLock = false;
    if (walletModel->getEncryptionStatus() == WalletModel::Locked)
    {
        AskPassphraseDialog dlg(AskPassphraseDialog::Unlock, this);
        dlg.setModel(walletModel);
        dlg.exec();

        if(walletModel->getEncryptionStatus() != WalletModel::Unlocked)
            return;

        fMustLock = true;
    }

    try {
        int iExportedCount, iErrorCount;
        walletModel->ExportPeercoinKeys(iExportedCount, iErrorCount);
        if (fMustLock)
            walletModel->setWalletLocked(true);
        QMessageBox::information(this,
                tr("NuBits keys export"),
                tr("%1 key(s) were exported to NuBits.\n%2 key(s) were either already known or invalid.")
                  .arg(iExportedCount)
                  .arg(iErrorCount)
                );
    }
    catch (std::runtime_error &e) {
        if (fMustLock)
            walletModel->setWalletLocked(true);
        QMessageBox::critical(this,
                tr("NuBits keys export"),
                tr("Error: %1").arg(e.what()));
    }
}

void BitcoinGUI::showNormalIfMinimized()
{
    if(!isVisible()) // Show, if hidden
        show();
    if(isMinimized()) // Unminimize, if minimized
        showNormal();
}

void BitcoinGUI::distributeDividendsClicked()
{
    DistributeDivDialog dd(this);
    dd.exec();
}

void BitcoinGUI::checkForUpdatesClicked()
{
    std::string err;
    bool success = CAutoUpdate::CheckForUpdates(false, &err);
    UpdateInfo info = CAutoUpdate::GetUpdateInfo();
    if (info.fUpdateAvailable)
    {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("NuBits update"));
        msgBox.setText(tr("A new version of NuBits is available (%1)!\n\nCurrent version: v%2\n\nDownload and install it now?")
            .arg(QString::fromStdString(info.strLatestVersion))
            .arg(QString::fromStdString(info.strCurrentVersion)));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::Yes)
        {
            std::string msg;
            if (!CAutoUpdate::ApplyUpdate(msg))
            {
                QMessageBox::warning(this, tr("Update error"), QString::fromStdString(msg));
            }
        }
    }
    else if (!success)
    {
        QMessageBox::warning(this, tr("Update check"), QString::fromStdString(err));
    }
    else
    {
        QMessageBox::information(this, tr("NuBits is up to date"),
            tr("You are already running the latest version of NuBits (v%1).")
            .arg(QString::fromStdString(info.strCurrentVersion)));
    }
}

void BitcoinGUI::checkPeriodicUpdate()
{
    static bool fNotifiedOnce = false;
    UpdateInfo info = CAutoUpdate::GetUpdateInfo();
    if (info.fUpdateAvailable && !fNotifiedOnce)
    {
        fNotifiedOnce = true;
        if (notificator)
        {
            notificator->notify(Notificator::Information,
                tr("Update available"),
                tr("A new version of NuBits (%1) is available on GitHub.").arg(QString::fromStdString(info.strLatestVersion)));
        }

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("NuBits update"));
        msgBox.setText(tr("A new version of NuBits is available (%1)!\n\nCurrent version: v%2\n\nDownload and install it now?")
            .arg(QString::fromStdString(info.strLatestVersion))
            .arg(QString::fromStdString(info.strCurrentVersion)));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        if (msgBox.exec() == QMessageBox::Yes)
        {
            std::string msg;
            if (!CAutoUpdate::ApplyUpdate(msg))
            {
                QMessageBox::warning(this, tr("Update error"), QString::fromStdString(msg));
            }
        }
    }
}

void BitcoinGUI::checkNoPeersOnStartup()
{
    if (fPromptedNoPeers || !clientModel)
        return;

    if (!clientModel->isTestNet() && clientModel->getNumConnections() == 0)
    {
        fPromptedNoPeers = true;

        if (addNodesDialog && addNodesDialog->isVisible())
            return;

        if (notificator)
        {
            notificator->notify(Notificator::Warning,
                tr("No NuBits network connections"),
                tr("No NuBits network connections were found. Open More > Network peers to add one manually."));
        }

        QMessageBox *msgBox = new QMessageBox(
            QMessageBox::Question,
            tr("No active peers"),
            tr("No active peers were found on the network.\n\nWould you like to open the peer manager and add a peer (for example, 188.68.55.35:7890)?"),
            QMessageBox::Yes | QMessageBox::No,
            this);
        msgBox->setDefaultButton(QMessageBox::Yes);
        msgBox->setWindowModality(Qt::NonModal);
        msgBox->setAttribute(Qt::WA_DeleteOnClose);
        connect(msgBox, SIGNAL(finished(int)), this, SLOT(onNoPeersPromptFinished(int)));
        msgBox->show();
    }
}

void BitcoinGUI::onNoPeersPromptFinished(int result)
{
    if (result == QMessageBox::Yes)
        addNodesClicked();
}
