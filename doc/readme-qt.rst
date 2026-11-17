Nu Qt wallet
============

The graphical wallet uses Qt Widgets and Qt Concurrent, C++17, OpenSSL 3,
Boost, and Berkeley DB 5.3. Qt 6.12 or newer is required. The Qt upgrade does
not change the network's staking rules or wallet serialization.

Back up existing wallet files before running an updated build. Do not run the
GUI and daemon against the same data directory at the same time.

macOS build
-----------

Install Xcode's command-line tools and the development dependencies. With
Homebrew, keep Qt 6 unlinked so an existing Qt 5 installation remains intact::

    brew install --skip-link qtbase qtsvg qtdeclarative qttools qttranslations
    brew install openssl@3 boost@1.85 berkeley-db@5
    bash ./build_gui_mac.sh
    open ./Nu.app

Qt Tools brings Qt Declarative and Qt SVG as dependencies; the wallet itself
does not use QML, WebEngine, or Qt5Compat. The script selects Qt 6 explicitly,
builds in ``build/qt6/``, generates translations there, and writes ``Nu.app``
at the repository root. Qt 5 objects and generated files are not reused.

The script detects Homebrew's Apple Silicon prefix or its Intel prefix.
``QT_PREFIX`` can select another Homebrew Qt Base prefix. ``BUILD_DIR``,
``DESTDIR``, and ``JOBS`` override the build directory, output directory,
and parallel job count respectively.

The minimum deployment target is macOS 14.4. The resulting app uses the local
Homebrew libraries and plugins and has a local ad-hoc signature, without
notarization or an online signing timestamp. It is not a standalone
redistributable bundle. Installed Homebrew libraries may require a newer
macOS release: the locally verified OpenSSL and Berkeley DB libraries target
macOS 27, and Boost targets macOS 26. The deployment target alone does not
establish compatibility with macOS 14.4.

Qt tests
--------

After the GUI build has generated ``build/qt6/qt6.conf``, use a separate test
build directory. On Apple Silicon::

    mkdir -p build/qt6-tests
    cd build/qt6-tests
    /opt/homebrew/opt/qtbase/bin/qmake -qtconf ../qt6/qt6.conf ../../nu.pro \
        BITCOIN_QT_TEST=1 QT_TOOL.lrelease.binary=/opt/homebrew/opt/qttools/bin/lrelease
    make -j2
    QT_QPA_PLATFORM=offscreen \
        QT_PLUGIN_PATH=/opt/homebrew/opt/qtbase/share/qt/plugins \
        ./bitcoin-qt_test.app/Contents/MacOS/bitcoin-qt_test

Use ``/usr/local/opt`` instead of ``/opt/homebrew/opt`` for Intel Homebrew.
These tests exercise URI handling, address and amount entry, embedded
translations, the overview's staking states and keyboard action, passphrase
cancellation and failure, and window construction without starting a node or
loading a real wallet. The core Boost test suite remains separate; see the
root README.

Local verification
------------------

Verified on 2026-11-17 with Qt 6.12.0 on macOS 27.2.1, Apple Silicon:

* The GUI build script and separate Qt test build completed with exit status 0.
* All eight functional Qt tests passed across four suites (16 passes including
  setup and cleanup), with exit status 0.
* The unchanged core suite passed all 86 cases and 44,908 assertions, with
  exit status 0.
* The production app constructed its window and printed version/help output
  with ``-?``. This help path returns status 1 by design. Plugin-path environment
  overrides were removed to check the bundle's own Qt configuration.
* Eight synthetic overview captures covered NSR locked, minting-only and
  startup-disabled states, plus NBT, at 750x500 and 1100x700.
* ``codesign --verify --deep --strict`` succeeded for the local ad-hoc bundle.

Execution used scratch home/data directories with networking denied. No real
wallet, historical-chain replay, live staking, or reward production was tested.
The offscreen plugin emitted font/size-hint warnings; legacy compiler warnings
and the newer Homebrew dependency targets noted above remain.

Other platforms and optional features
-------------------------------------

Linux and Windows need a Qt 6.12-compatible C++17 toolchain and the same
non-Qt dependencies. Configure a separate build directory with Qt 6's
``qmake`` and the repository's ``nu.pro``, then build with the platform's
make tool. Do not reuse Qt 5 build products.

The existing qmake options remain available: ``USE_UPNP=1`` requires
miniupnpc, ``USE_DBUS=1`` requires Qt DBus, and ``USE_QRCODE=1`` requires
libqrencode. UPnP is excluded by default. These optional combinations and
Linux/Windows GUI builds need their own platform verification.

Staking
-------

Staking applies to NuShares (NSR), not NuBits (NBT). Select NuShares in the
asset selector and open the overview.

* **Wallet locked** offers **Unlock for staking**. The existing passphrase
  dialog grants minting-only access; canceling or an incorrect passphrase
  must leave the wallet locked.
* **Unlocked for staking only** offers **Lock wallet**. Sending still needs
  normal wallet authorization.
* Fully unlocked and unencrypted wallets are identified separately; the
  overview does not pretend that they have a staking on/off switch.
* If ``-stakegen=0`` disabled the minter at startup, the overview explains
  that a restart is needed. Restart without that option or use
  ``-stakegen=1``. Unlocking does not start a disabled minter.

Keep the app running, connected, and synchronized. The displayed network
state is not proof of minter activity. Eligibility still depends on the
existing coin-age, input-value, maturity, and reserve rules. The overview
does not calculate eligible inputs, promise rewards, or change consensus.
The existing "Stake" balance describes immature stake, not eligible funds.

UX and toolkit references
-------------------------

The design keeps the existing wallet identity and advanced tools. It uses
an asset-context action and separate, truthful states rather than borrowing
another network's reward or delegation model:

* `Exodus staking UX <https://www.exodus.com/support/en/articles/8598638-does-exodus-pay-stakes-how-to-stake-crypto>`_
* `Nielsen Norman Group usability heuristics <https://www.nngroup.com/articles/ten-usability-heuristics/>`_
* `Qt 6.12 release <https://www.qt.io/blog/qt-6.12-released>`_
* `Qt 6.12 supported platforms <https://doc.qt.io/qt-6.12/supported-platforms.html>`_
