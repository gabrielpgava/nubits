#ifndef CUSTODIANADDRESS_H
#define CUSTODIANADDRESS_H

class CCustodianAddress // We cannot use a CBitcoinAddress because the unit may be unknown
{
public:
    unsigned char cUnit;
    bool fScript;
    uint160 hashAddress;

    CCustodianAddress() :
        cUnit('?'),
        fScript(false),
        hashAddress(0)
    {
    }

    CCustodianAddress(const CTxDestination& destination, unsigned char cUnit)
    {
        SetDestination(destination, cUnit);
    }

    IMPLEMENT_SERIALIZE
    (
        READWRITE(cUnit);
        if (nVersion >= 20200) // version 0.2.2
            READWRITE(fScript);
        else if (fRead)
            const_cast<CCustodianAddress*>(this)->fScript = false;
        READWRITE(hashAddress);
    )

    inline bool operator==(const CCustodianAddress& other) const
    {
        return (cUnit == other.cUnit &&
                fScript == other.fScript &&
                hashAddress == other.hashAddress);
    }
    inline bool operator!=(const CCustodianAddress& other) const
    {
        return !(*this == other);
    }

    class CDestinationVisitor : public boost::static_visitor<bool>
    {
        private:
            CCustodianAddress *custodianAddress;
        public:
            CDestinationVisitor(CCustodianAddress *custodianAddress) : custodianAddress(custodianAddress) { }

            bool operator()(const CNoDestination &dest) const {
                custodianAddress->fScript = false;
                custodianAddress->hashAddress = 0;
                return false;
            }

            bool operator()(const CKeyID &keyID) const {
                custodianAddress->fScript = false;
                custodianAddress->hashAddress = keyID;
                return true;
            }

            bool operator()(const CScriptID &scriptID) const {
                custodianAddress->fScript = true;
                custodianAddress->hashAddress = scriptID;
                return true;
            }
    };

    void SetAddress(const CBitcoinAddress& address)
    {
        cUnit = address.GetUnit();
        CTxDestination destination = address.Get();
        boost::apply_visitor(CDestinationVisitor(this), destination);
    }

    void SetDestination(const CTxDestination& destination, unsigned char cUnit)
    {
        this->cUnit = cUnit;
        boost::apply_visitor(CDestinationVisitor(this), destination);
    }

    CBitcoinAddress GetAddress() const
    {
        CBitcoinAddress address;
        if (fScript)
            address.Set(CScriptID(hashAddress), cUnit);
        else
            address.Set(CKeyID(hashAddress), cUnit);
        return address;
    }

    CTxDestination GetDestination() const
    {
        if (fScript)
            return CScriptID(hashAddress);
        else
            return CKeyID(hashAddress);
    }

    bool operator< (const CCustodianAddress& other) const
    {
        if (cUnit < other.cUnit)
            return true;
        if (cUnit > other.cUnit)
            return false;
        if (fScript < other.fScript)
            return true;
        if (fScript > other.fScript)
            return false;
        if (hashAddress < other.hashAddress)
            return true;
        return false;
    }

    bool operator> (const CCustodianAddress& other) const
    {
        if (cUnit > other.cUnit)
            return true;
        if (cUnit < other.cUnit)
            return false;
        if (fScript > other.fScript)
            return true;
        if (fScript < other.fScript)
            return false;
        if (hashAddress > other.hashAddress)
            return true;
        return false;
    }
};

#endif
