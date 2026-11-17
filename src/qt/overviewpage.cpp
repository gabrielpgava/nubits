#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "walletmodel.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include "guiconstants.h"
#include "util.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <QPushButton>
#include <QFormLayout>
#include <QBoxLayout>
#include <QResizeEvent>
#include <QVBoxLayout>

#define DECORATION_SIZE 64
#define NUM_ITEMS 3

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
#if QT_VERSION >= 0x050000
        if(value.canConvert<QColor>())
#else
        if(qVariantCanConvert<QColor>(value))
#endif
        {
            foreground = qvariant_cast<QColor>(value);
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address);

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    model(0),
    currentBalance(-1),
    currentStake(0),
    currentUnconfirmedBalance(-1),
    currentParked(0),
    txdelegate(new TxViewDelegate()),
    stakingLabel(new QLabel(tr("Staking:"), this)),
    stakingStatus(new QLabel(this)),
    stakingNetworkStatus(new QLabel(this)),
    stakingConfigStatus(new QLabel(this)),
    stakingAction(new QPushButton(this))
{
    ui->setupUi(this);
    stakingStatus->setObjectName("stakingStatus");
    stakingStatus->setWordWrap(true);
    stakingStatus->setAccessibleName(tr("Staking wallet status"));
    stakingNetworkStatus->setObjectName("stakingNetworkStatus");
    stakingNetworkStatus->setWordWrap(true);
    stakingNetworkStatus->setAccessibleName(tr("Network status for staking"));
    stakingConfigStatus->setObjectName("stakingConfigStatus");
    stakingConfigStatus->setWordWrap(true);
    stakingConfigStatus->setAccessibleName(tr("Staking startup configuration"));
    stakingAction->setObjectName("stakingAction");
    stakingAction->setVisible(false);
    QFormLayout *formLayout = qobject_cast<QFormLayout *>(ui->frame->layout());
    QWidget *stakingDetails = new QWidget(ui->frame);
    QVBoxLayout *stakingLayout = new QVBoxLayout(stakingDetails);
    stakingLayout->setContentsMargins(0, 0, 0, 0);
    stakingLayout->setSpacing(6);
    stakingLayout->addWidget(stakingStatus);
    stakingLayout->addWidget(stakingNetworkStatus);
    stakingLayout->addWidget(stakingConfigStatus);
    stakingLayout->addWidget(stakingAction);
    formLayout->insertRow(3, stakingLabel, stakingDetails);
    connect(stakingAction, &QPushButton::clicked, this, [this]() {
        emit stakingUnlockRequested(stakingAction->property("unlock").toBool());
    });

    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    ui->frame->setAttribute(Qt::WA_StyledBackground, true);
    ui->frame->setAutoFillBackground(true);
    ui->frame_2->setAttribute(Qt::WA_StyledBackground, true);
    ui->frame_2->setAutoFillBackground(true);

    // Balance: <balance>
    ui->labelBalance->setToolTip(tr("Your current balance"));
    ui->labelBalance->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    // ppcoin: stake: <stake>
    ui->labelStake->setToolTip(tr("Your current stake"));
    ui->labelStake->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    // Unconfirmed balance: <balance>
    ui->labelUnconfirmed->setToolTip(tr("Total of transactions that have yet to be confirmed, and do not yet count toward the current balance"));
    ui->labelUnconfirmed->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    // nubit: parked: <parked>
    ui->labelParked->setToolTip(tr("Your current parked amount"));
    ui->labelParked->setTextInteractionFlags(Qt::TextSelectableByMouse|Qt::TextSelectableByKeyboard);

    ui->labelNumTransactions->setToolTip(tr("Total number of transactions in wallet"));

    // Recent transactions
    ui->listTransactions->setStyleSheet("QListView { background: transparent; border: none; outline: none; }");
    ui->listTransactions->setFrameShape(QFrame::NoFrame);
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setSelectionMode(QAbstractItemView::NoSelection);
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SIGNAL(transactionClicked(QModelIndex)));

    // Hiding doesn't remove the margin so we have to remove the widgets
    // See https://bugreports.qt-project.org/browse/QTBUG-6864
    formLayout->getWidgetPosition(ui->labelStake, &stakeRow, NULL);
    removeLabels(ui->labelParkedLabel, ui->labelParked);
    removeLabels(ui->labelStakeLabel, ui->labelStake);
    refreshStakingStatus();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::resizeEvent(QResizeEvent *event)
{
    ui->horizontalLayout->setDirection(width() < 800
        ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    QWidget::resizeEvent(event);
}

void OverviewPage::removeLabels(QLabel* labelLabel, QLabel* label)
{
    labelLabel->hide();
    label->hide();
    ui->frame->layout()->removeWidget(labelLabel);
    ui->frame->layout()->removeWidget(label);
}

void OverviewPage::insertLabels(QLabel* labelLabel, QLabel* label, int row)
{
    QFormLayout* formLayout = (QFormLayout*)ui->frame->layout();

    formLayout->insertRow(row, labelLabel, label);
    labelLabel->show();
    label->show();
}

void OverviewPage::setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 parked)
{
    int unit = model->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentStake = stake;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentParked = parked;
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance));
    ui->labelStake->setText(BitcoinUnits::formatWithUnit(unit, stake));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance));
    ui->labelParked->setText(BitcoinUnits::formatWithUnit(unit, parked));

    if (model->getUnit() == 'S')
    {
        removeLabels(ui->labelParkedLabel, ui->labelParked);
        insertLabels(ui->labelStakeLabel, ui->labelStake, stakeRow);
    }
    else
    {
        removeLabels(ui->labelStakeLabel, ui->labelStake);
        insertLabels(ui->labelParkedLabel, ui->labelParked, stakeRow);
    }
}

void OverviewPage::setNumTransactions(int count)
{
    ui->labelNumTransactions->setText(QLocale::system().toString(count));
}

void OverviewPage::setModel(WalletModel *model)
{
    if (this->model && this->model != model)
        disconnect(this->model, SIGNAL(encryptionStatusChanged(int)), this, SLOT(refreshStakingStatus()));
    this->model = model;
    refreshStakingStatus();
    if(model)
    {
        // Set up transaction list
        TransactionFilterProxy *filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getStake(), model->getUnconfirmedBalance(), model->getParked());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64, qint64)));
        connect(model, SIGNAL(encryptionStatusChanged(int)), this, SLOT(refreshStakingStatus()));

        setNumTransactions(model->getNumTransactions());
        connect(model, SIGNAL(numTransactionsChanged(int)), this, SLOT(setNumTransactions(int)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(displayUnitChanged()));

        emit displayUnitChanged();
    }
}

void OverviewPage::refreshStakingStatus()
{
    QFormLayout *formLayout = qobject_cast<QFormLayout *>(ui->frame->layout());
    if (!model) {
        stakingAction->setVisible(false);
        stakingAction->setEnabled(false);
        stakingAction->setProperty("unlock", false);
        stakingConfigStatus->clear();
        stakingConfigStatus->setVisible(false);
        stakingStatus->setText(tr("Wallet status unavailable."));
        stakingStatus->setAccessibleDescription(stakingStatus->text());
        formLayout->setRowVisible(stakingLabel, true);
        emitStakingDetails();
        return;
    }
    if (model->getUnit() != 'S') {
        setStakingWalletState(false, false, false, false, false);
        return;
    }
    const WalletModel::EncryptionStatus status = model->getEncryptionStatus();
    setStakingWalletState(true, status != WalletModel::Unencrypted,
                          status == WalletModel::Locked,
                          model->isUnlockedForMintingOnly(),
                          !GetBoolArg("-stakegen", true));
}

void OverviewPage::setStakingWalletState(bool isShares, bool encrypted, bool locked, bool mintOnly, bool stakingDisabled)
{
    QFormLayout *formLayout = qobject_cast<QFormLayout *>(ui->frame->layout());
    formLayout->setRowVisible(stakingLabel, isShares);
    stakingAction->setVisible(false);
    stakingAction->setEnabled(false);
    stakingAction->setProperty("unlock", false);
    if (!isShares) {
        stakingConfigStatus->setVisible(false);
        emitStakingDetails();
        return;
    }

    if (locked) {
        stakingStatus->setText(tr("Wallet locked"));
    } else if (!encrypted) {
        stakingStatus->setText(tr("Wallet not encrypted"));
    } else if (mintOnly) {
        stakingStatus->setText(tr("Unlocked for staking only"));
    } else {
        stakingStatus->setText(tr("Wallet fully unlocked"));
    }
    stakingConfigStatus->setVisible(stakingDisabled);
    stakingConfigStatus->setText(stakingDisabled
        ? tr("Staking is disabled in startup settings. Restart Nu with staking enabled to participate.")
        : QString());
    stakingStatus->setToolTip(tr("Sending still requires wallet authorization. Participation also depends on startup settings, synchronization, and eligible NuShares."));
    stakingNetworkStatus->setToolTip(tr("A network connection does not confirm staking participation."));
    stakingAction->setAccessibleDescription(stakingStatus->text());

    if (stakingDisabled && !(mintOnly && !locked)) {
        emitStakingDetails();
        return;
    }
    if (locked && encrypted) {
        stakingAction->setText(tr("Unlock for staking"));
        stakingAction->setProperty("unlock", true);
        stakingAction->setToolTip(stakingStatus->toolTip());
        stakingAction->setEnabled(true);
        stakingAction->setVisible(true);
    } else if (mintOnly && !locked) {
        stakingAction->setText(tr("Lock wallet"));
        stakingAction->setProperty("unlock", false);
        stakingAction->setToolTip(tr("Lock the wallet."));
        stakingAction->setEnabled(true);
        stakingAction->setVisible(true);
    }
    emitStakingDetails();
}

void OverviewPage::setStakingNetworkStatus(const QString &status)
{
    stakingNetworkStatus->setText(status);
    emitStakingDetails();
}

void OverviewPage::emitStakingDetails()
{
    emit stakingDetailsChanged(stakingStatus->text(), stakingNetworkStatus->text(),
                               stakingConfigStatus->text(), stakingAction->text(),
                               !stakingAction->isHidden(),
                               stakingAction->property("unlock").toBool());
}

void OverviewPage::displayUnitChanged()
{
    if(!model || !model->getOptionsModel())
        return;
    if(currentBalance != -1)
        setBalance(currentBalance, currentStake, currentUnconfirmedBalance, currentParked);

    txdelegate->unit = model->getOptionsModel()->getDisplayUnit();
    ui->listTransactions->update();
}
