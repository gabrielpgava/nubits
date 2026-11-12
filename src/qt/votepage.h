#ifndef VOTEPAGE_H
#define VOTEPAGE_H

#include <QWidget>

namespace Ui {
class VotePage;
}
class WalletModel;
class ClientModel;
class CBlockIndex;
class QTimer;

class VotePage : public QWidget
{
    Q_OBJECT

public:
    explicit VotePage(QWidget *parent = 0);
    ~VotePage();

    void setModel(WalletModel *model);
    void setClientModel(ClientModel *clientModel);
    void setPollInterval(int interval);

private:
    Ui::VotePage *ui;
    WalletModel *model;
    ClientModel *clientModel;
    QTimer *pollTimer;

    CBlockIndex* lastBestBlock;
    void fillCustodianTable();
    void fillParkRateTable();
    unsigned char selectedCurrency();

private slots:
    void on_custodianVote_clicked();
    void on_parkRateVote_clicked();
    void on_motionVote_clicked();
    void on_feeVote_clicked();
    void on_dataFeedButton_clicked();
    void update();
    void updateLiquidity();
    void selectedCurrencyChanged(int);
};

#endif // VOTEPAGE_H
