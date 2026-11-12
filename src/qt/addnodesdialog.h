#ifndef ADDNODESDIALOG_H
#define ADDNODESDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QHideEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QShowEvent;
class QTableWidget;
class QTimer;
QT_END_NAMESPACE

/** Dialog to view connected network peers and manage manual/custom server nodes. */
class AddNodesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddNodesDialog(QWidget *parent = 0);
    ~AddNodesDialog();

public slots:
    void refreshPeers();
    void refreshCustomNodes();

protected:
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);

private slots:
    void onFillRecoveryNode();
    void onAddNodeClicked();
    void onRemoveNodeClicked();
    void onCustomNodeSelectionChanged();

private:
    QLabel *connectionsStatusLabel;
    QTableWidget *peersTable;

    QLineEdit *nodeInput;
    QPushButton *fillRecoveryButton;
    QCheckBox *saveToConfCheck;
    QPushButton *addButton;

    QListWidget *customNodesList;
    QPushButton *removeButton;

    QLabel *statusLabel;
    QPushButton *closeButton;
    QTimer *refreshTimer;

    void setupUi();
    void setStatusMessage(const QString &message, bool isError = false);
};

#endif // ADDNODESDIALOG_H
