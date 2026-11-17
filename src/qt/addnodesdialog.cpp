#include "addnodesdialog.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QHideEvent>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include <string>
#include <vector>

#include "net.h"
#include "util.h"

AddNodesDialog::AddNodesDialog(QWidget *parent) :
    QDialog(parent),
    connectionsStatusLabel(0),
    peersTable(0),
    nodeInput(0),
    fillRecoveryButton(0),
    saveToConfCheck(0),
    addButton(0),
    customNodesList(0),
    removeButton(0),
    statusLabel(0),
    closeButton(0),
    refreshTimer(0)
{
    setupUi();

    refreshTimer = new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refreshPeers()));

    refreshPeers();
    refreshCustomNodes();
}

AddNodesDialog::~AddNodesDialog()
{
}

void AddNodesDialog::setupUi()
{
    setWindowTitle(tr("Network peers"));
    setMinimumSize(680, 560);
    resize(740, 600);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Section 1: Connected Peers
    QGroupBox *peersGroup = new QGroupBox(tr("Connected peers"), this);
    QVBoxLayout *peersLayout = new QVBoxLayout(peersGroup);
    peersLayout->setContentsMargins(12, 14, 12, 12);
    peersLayout->setSpacing(8);

    connectionsStatusLabel = new QLabel(tr("Active connections: 0"), peersGroup);
    peersLayout->addWidget(connectionsStatusLabel);

    peersTable = new QTableWidget(0, 4, peersGroup);
    QStringList headers;
    headers << tr("Address / IP") << tr("Version / Client") << tr("Block height") << tr("Direction");
    peersTable->setHorizontalHeaderLabels(headers);
    peersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    peersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    peersTable->setSelectionMode(QAbstractItemView::SingleSelection);
    peersTable->setAlternatingRowColors(true);
    peersTable->verticalHeader()->setVisible(false);
    peersTable->setMinimumHeight(180);
#if QT_VERSION >= 0x050000
    peersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    peersTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    peersTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    peersTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
#else
    peersTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);
    peersTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
    peersTable->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
    peersTable->horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);
#endif
    peersLayout->addWidget(peersTable);
    mainLayout->addWidget(peersGroup, 2);

    // Section 2: Add New Server
    QGroupBox *addGroup = new QGroupBox(tr("Add a peer"), this);
    QVBoxLayout *addLayout = new QVBoxLayout(addGroup);
    addLayout->setContentsMargins(12, 14, 12, 12);
    addLayout->setSpacing(8);

    QHBoxLayout *inputRow = new QHBoxLayout();
    inputRow->setSpacing(8);
    nodeInput = new QLineEdit(addGroup);
    nodeInput->setPlaceholderText(tr("For example: node.example.org:7890"));
    addButton = new QPushButton(QIcon(":/icons/add"), tr("Add and connect"), addGroup);
    addButton->setAutoDefault(false);
    addButton->setDefault(false);
    addButton->setCursor(Qt::PointingHandCursor);
    addButton->setStyleSheet(
        "QPushButton { background-color: #238636; border: 1px solid #2ea043; color: #ffffff; font-weight: 600; }"
        "QPushButton:hover { background-color: #2ea043; border-color: #3fb950; }"
        "QPushButton:pressed { background-color: #196c2e; }"
    );
    inputRow->addWidget(nodeInput, 1);
    inputRow->addWidget(addButton);
    addLayout->addLayout(inputRow);

    QHBoxLayout *optionsRow = new QHBoxLayout();
    optionsRow->setSpacing(8);
    saveToConfCheck = new QCheckBox(tr("Save permanently in nu.conf"), addGroup);
    saveToConfCheck->setChecked(true);
    fillRecoveryButton = new QPushButton(tr("Fill recovery peer"), addGroup);
    fillRecoveryButton->setAutoDefault(false);
    fillRecoveryButton->setDefault(false);
    fillRecoveryButton->setCursor(Qt::PointingHandCursor);
    fillRecoveryButton->setToolTip(tr("Fill in the known historical recovery peer address"));
    optionsRow->addWidget(saveToConfCheck);
    optionsRow->addStretch(1);
    optionsRow->addWidget(fillRecoveryButton);
    addLayout->addLayout(optionsRow);

    mainLayout->addWidget(addGroup);

    // Section 3: Manually Added Servers
    QGroupBox *customGroup = new QGroupBox(tr("Manually added peers"), this);
    QVBoxLayout *customLayout = new QVBoxLayout(customGroup);
    customLayout->setContentsMargins(12, 14, 12, 12);
    customLayout->setSpacing(8);

    customNodesList = new QListWidget(customGroup);
    customNodesList->setSelectionMode(QAbstractItemView::SingleSelection);
    customNodesList->setMinimumHeight(100);
    customLayout->addWidget(customNodesList);

    QHBoxLayout *customButtonsRow = new QHBoxLayout();
    customButtonsRow->addStretch(1);
    removeButton = new QPushButton(QIcon(":/icons/remove"), tr("Remove selected"), customGroup);
    removeButton->setAutoDefault(false);
    removeButton->setDefault(false);
    removeButton->setEnabled(false);
    removeButton->setCursor(Qt::PointingHandCursor);
    customButtonsRow->addWidget(removeButton);
    customLayout->addLayout(customButtonsRow);

    mainLayout->addWidget(customGroup, 1);

    // Bottom status bar & Close button
    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(12);
    statusLabel = new QLabel(this);
    statusLabel->setWordWrap(true);
    closeButton = new QPushButton(tr("Close"), this);
    closeButton->setAutoDefault(false);
    closeButton->setDefault(false);
    closeButton->setCursor(Qt::PointingHandCursor);
    bottomRow->addWidget(statusLabel, 1);
    bottomRow->addWidget(closeButton);
    mainLayout->addLayout(bottomRow);

    connect(fillRecoveryButton, SIGNAL(clicked()), this, SLOT(onFillRecoveryNode()));
    connect(addButton, SIGNAL(clicked()), this, SLOT(onAddNodeClicked()));
    connect(nodeInput, SIGNAL(returnPressed()), this, SLOT(onAddNodeClicked()));
    connect(removeButton, SIGNAL(clicked()), this, SLOT(onRemoveNodeClicked()));
    connect(customNodesList, SIGNAL(itemSelectionChanged()), this, SLOT(onCustomNodeSelectionChanged()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

void AddNodesDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    refreshPeers();
    refreshCustomNodes();
    if (refreshTimer && !refreshTimer->isActive())
        refreshTimer->start(2000);
}

void AddNodesDialog::hideEvent(QHideEvent *event)
{
    if (refreshTimer && refreshTimer->isActive())
        refreshTimer->stop();
    QDialog::hideEvent(event);
}

void AddNodesDialog::refreshPeers()
{
    std::vector<CNodeStats> vStats;
    {
        LOCK(cs_vNodes);
        vStats.reserve(vNodes.size());
        BOOST_FOREACH(CNode* pnode, vNodes)
        {
            CNodeStats stats;
            pnode->copyStats(stats);
            vStats.push_back(stats);
        }
    }

    if (vStats.empty())
    {
        connectionsStatusLabel->setText(tr("No active connections"));
        connectionsStatusLabel->setStyleSheet("color: #e3b341; font-weight: 600;");
    }
    else
    {
        connectionsStatusLabel->setText(tr("Active connections: %1").arg(vStats.size()));
        connectionsStatusLabel->setStyleSheet("color: #3fb950; font-weight: 600;");
    }

    QString selectedAddr;
    int currentRow = peersTable->currentRow();
    if (currentRow >= 0 && peersTable->item(currentRow, 0))
        selectedAddr = peersTable->item(currentRow, 0)->text();

    peersTable->setRowCount((int)vStats.size());
    int restoreRow = -1;

    for (int i = 0; i < (int)vStats.size(); ++i)
    {
        const CNodeStats &stats = vStats[i];
        QString addrStr = QString::fromStdString(stats.addrName);
        if (!selectedAddr.isEmpty() && addrStr == selectedAddr)
            restoreRow = i;

        QString versionStr;
        if (!stats.strSubVer.empty())
            versionStr = QString("%1 (%2)").arg(QString::fromStdString(stats.strSubVer)).arg(stats.nVersion);
        else
            versionStr = QString::number(stats.nVersion);

        QString heightStr = stats.nStartingHeight >= 0 ? QString::number(stats.nStartingHeight) : tr("N/A");
        QString dirStr = stats.fInbound ? tr("Incoming") : tr("Outgoing");

        QTableWidgetItem *addrItem = new QTableWidgetItem(addrStr);
        QTableWidgetItem *verItem = new QTableWidgetItem(versionStr);
        QTableWidgetItem *heightItem = new QTableWidgetItem(heightStr);
        heightItem->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem *dirItem = new QTableWidgetItem(dirStr);
        dirItem->setTextAlignment(Qt::AlignCenter);

        peersTable->setItem(i, 0, addrItem);
        peersTable->setItem(i, 1, verItem);
        peersTable->setItem(i, 2, heightItem);
        peersTable->setItem(i, 3, dirItem);
    }

    if (restoreRow >= 0)
        peersTable->selectRow(restoreRow);
}

void AddNodesDialog::refreshCustomNodes()
{
    QString selectedNode;
    if (customNodesList->currentItem())
        selectedNode = customNodesList->currentItem()->text();

    std::vector<std::string> vCustomNodes = GetCustomNodes();

    customNodesList->clear();
    int restoreRow = -1;
    for (int i = 0; i < (int)vCustomNodes.size(); ++i)
    {
        QString qNode = QString::fromStdString(vCustomNodes[i]);
        customNodesList->addItem(qNode);
        if (!selectedNode.isEmpty() && qNode == selectedNode)
            restoreRow = i;
    }

    if (restoreRow >= 0)
        customNodesList->setCurrentRow(restoreRow);

    onCustomNodeSelectionChanged();
}

void AddNodesDialog::onFillRecoveryNode()
{
    nodeInput->setText("188.68.55.35:7890");
    nodeInput->setFocus();
    setStatusMessage(tr("Recovery peer address filled. Select Add and connect to connect."), false);
}

void AddNodesDialog::onAddNodeClicked()
{
    QString qNode = nodeInput->text().trimmed();
    if (qNode.isEmpty())
    {
        QString msg = tr("Enter a valid IP address or hostname, such as node.example.org:7890.");
        setStatusMessage(msg, true);
        QMessageBox::warning(this, tr("Invalid address"), msg);
        nodeInput->setFocus();
        return;
    }

    std::string strNode = qNode.toStdString();
    bool fSaveToConf = saveToConfCheck->isChecked();
    std::string strError;

    if (!AddCustomNode(strNode, fSaveToConf, strError))
    {
        QString errMsg = strError.empty() ? tr("Could not add the peer.") : QString::fromStdString(strError);
        setStatusMessage(errMsg, true);
        QMessageBox::warning(this, tr("Could not add peer"), errMsg);
        return;
    }

    nodeInput->clear();
    refreshCustomNodes();
    refreshPeers();
    setStatusMessage(tr("Peer %1 added. Connecting...").arg(qNode), false);
}

void AddNodesDialog::onRemoveNodeClicked()
{
    QListWidgetItem *item = customNodesList->currentItem();
    if (!item)
        return;

    QString qNode = item->text();
    std::string strNode = qNode.toStdString();

    if (RemoveCustomNode(strNode, true))
    {
        refreshCustomNodes();
        refreshPeers();
        setStatusMessage(tr("Peer %1 removed.").arg(qNode), false);
    }
    else
    {
        QString errMsg = tr("Could not remove peer %1.").arg(qNode);
        setStatusMessage(errMsg, true);
        QMessageBox::warning(this, tr("Could not remove peer"), errMsg);
    }
}

void AddNodesDialog::onCustomNodeSelectionChanged()
{
    removeButton->setEnabled(customNodesList->currentItem() != 0);
}

void AddNodesDialog::setStatusMessage(const QString &message, bool isError)
{
    statusLabel->setText(message);
    if (isError)
        statusLabel->setStyleSheet("color: #f85149; font-size: 12px;");
    else
        statusLabel->setStyleSheet("color: #3fb950; font-size: 12px;");
}
