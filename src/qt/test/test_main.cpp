#include <QTest>
#include <QObject>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QFont>
#include <QStyleFactory>
#include <QPalette>
#include <QColor>

#include "uritests.h"

// This is all you need to run all the tests
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(13, 17, 23));
    darkPalette.setColor(QPalette::WindowText, QColor(230, 237, 243));
    darkPalette.setColor(QPalette::Base, QColor(22, 27, 34));
    darkPalette.setColor(QPalette::AlternateBase, QColor(13, 17, 23));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(22, 27, 34));
    darkPalette.setColor(QPalette::ToolTipText, QColor(240, 246, 252));
    darkPalette.setColor(QPalette::Text, QColor(230, 237, 243));
    darkPalette.setColor(QPalette::Button, QColor(33, 38, 45));
    darkPalette.setColor(QPalette::ButtonText, QColor(240, 246, 252));
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(88, 166, 255));
    darkPalette.setColor(QPalette::Highlight, QColor(245, 166, 35));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    QFile stylesheet(":/styles/S.qss");
    if (stylesheet.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    const int fontId = QFontDatabase::addApplicationFont(":/fonts/rob_r");
    if (fontId >= 0) {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty())
            app.setFont(QFont(families.first(), 12));
    }
    URITests test1;
    OverviewPageTests test2;
    PassphraseDialogTests test3;
    BitcoinGUITests test4;
    const int uriResult = QTest::qExec(&test1, argc, argv);
    const int overviewResult = QTest::qExec(&test2, argc, argv);
    const int passphraseResult = QTest::qExec(&test3, argc, argv);
    const int bitcoinGuiResult = QTest::qExec(&test4, argc, argv);
    return uriResult || overviewResult || passphraseResult || bitcoinGuiResult;
}
