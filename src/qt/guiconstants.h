#ifndef GUICONSTANTS_H
#define GUICONSTANTS_H

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 500;

/* Maximum  passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(227, 179, 65)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(248, 81, 73)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(139, 148, 158)

#endif // GUICONSTANTS_H
