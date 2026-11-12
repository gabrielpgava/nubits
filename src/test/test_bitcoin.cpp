#define BOOST_TEST_MODULE Bitcoin Test Suite
#include <boost/test/unit_test.hpp>

#include "main.h"
#include "wallet.h"
#include <boost/thread/tss.hpp>

CWallet* pwalletMain;
extern boost::thread_specific_ptr<CWallet*> threadWallet;

extern bool fPrintToConsole;
struct TestingSetup {
    TestingSetup() {
        fPrintToConsole = true; // don't want to write to debug.log file
        pwalletMain = new CWallet();
        pwalletMain->SetUnit('S');
        RegisterWallet(pwalletMain);
        threadWallet.reset(new CWallet*(pwalletMain));
    }
    ~TestingSetup()
    {
        UnregisterWallet(pwalletMain);
        delete pwalletMain;
        pwalletMain = NULL;
    }
};

BOOST_GLOBAL_FIXTURE(TestingSetup);

void Shutdown(void* parg)
{
  exit(0);
}

void StartShutdown()
{
  exit(0);
}

