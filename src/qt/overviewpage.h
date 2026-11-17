#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>
#include <QLabel>

QT_BEGIN_NAMESPACE
class QModelIndex;
class QResizeEvent;
QT_END_NAMESPACE

namespace Ui {
    class OverviewPage;
}
class WalletModel;
class TxViewDelegate;
class QPushButton;

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setModel(WalletModel *model);

protected:
    void resizeEvent(QResizeEvent *event) override;

public slots:
    void setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 parked);
    void setNumTransactions(int count);
    void refreshStakingStatus();
    void setStakingNetworkStatus(const QString &status);
    void setStakingWalletState(bool isShares, bool encrypted, bool locked, bool mintOnly, bool stakingDisabled);

signals:
    void transactionClicked(const QModelIndex &index);
    void stakingUnlockRequested(bool unlock);
    void stakingDetailsChanged(const QString &walletStatus, const QString &networkStatus,
                               const QString &configurationStatus, const QString &actionText,
                               bool actionVisible, bool unlock);

private:
    Ui::OverviewPage *ui;
    WalletModel *model;
    qint64 currentBalance;
    qint64 currentStake;
    qint64 currentUnconfirmedBalance;
    qint64 currentParked;

    TxViewDelegate *txdelegate;
    QLabel *stakingLabel;
    QLabel *stakingStatus;
    QLabel *stakingNetworkStatus;
    QLabel *stakingConfigStatus;
    QPushButton *stakingAction;

    int stakeRow;

    void removeLabels(QLabel* labelLabel, QLabel* label);
    void insertLabels(QLabel* labelLabel, QLabel* label, int row);
    void emitStakingDetails();

private slots:
    void displayUnitChanged();
};

#endif // OVERVIEWPAGE_H
