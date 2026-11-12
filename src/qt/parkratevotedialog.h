#ifndef PARKRATEVOTEDIALOG_H
#define PARKRATEVOTEDIALOG_H

#include <QDialog>

namespace Ui {
class ParkRateVoteDialog;
}
class WalletModel;
class CParkRateVote;

class ParkRateVoteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ParkRateVoteDialog(QWidget *parent = 0);
    ~ParkRateVoteDialog();

    void setModel(WalletModel *model);

    double getAnnualRatePercentage(int row);
    qint64 getRate(int row);
    qint64 getDuration(int row);
    int getCompactDuration(int row);

    void setDuration(int row, qint64 duration);
    void setAnnualRatePercentage(int row, double rate);

private:
    Ui::ParkRateVoteDialog *ui;

    WalletModel *model;

    std::vector<CParkRateVote> vParkRateVote;
    unsigned char currentDataUnit;
    int currentDataTabIndex;
    bool hasCurrentData;

    void error(const QString& message);
    unsigned char selectedCurrency();
    void fillCurrencyData();
    bool storeCurrentData();

private slots:
    void on_addShorter_clicked();
    void on_addLonger_clicked();
    void on_remove_clicked();
    void on_table_cellChanged(int row, int column);
    void accept();
    void selectedCurrencyChanged(int);
};

#endif // PARKRATEVOTEDIALOG_H
