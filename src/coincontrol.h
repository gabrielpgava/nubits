// Copyright (c) 2014-2015 The Nu developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COINCONTROL_H
#define COINCONTROL_H

/** Coin Control Features. */
class CCoinControl
{
public:
    CTxDestination destChange;
    bool fUseOnlyRequired;
    bool fSendChangeToLastInput;

    CCoinControl()
    {
        SetNull();
    }
        
    void SetNull()
    {
        destChange = CNoDestination();
        setSelected.clear();
        setBlocked.clear();
        fUseOnlyRequired = false;
        fSendChangeToLastInput = false;
    }
    
    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }
    
    bool IsSelected(const uint256& hash, unsigned int n) const
    {
        COutPoint outpt(hash, n);
        return (setSelected.count(outpt) > 0);
    }
    
    void Select(const COutPoint& output)
    {
        setSelected.insert(output);
    }
    
    void UnSelect(const COutPoint& output)
    {
        setSelected.erase(output);
    }
    
    void UnSelectAll()
    {
        setSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }
        
    void AddBlocked(const COutPoint& output)
    {
        setBlocked.insert(output);
    }

    void RemoveBlocked(const COutPoint& output)
    {
        setBlocked.erase(output);
    }

    void ListBlocked(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setBlocked.begin(), setBlocked.end());
    }

    bool IsBlocked(const uint256& hash, unsigned int n) const
    {
        COutPoint outpt(hash, n);
        return (setBlocked.count(outpt) > 0);
    }

private:
    std::set<COutPoint> setSelected;
    std::set<COutPoint> setBlocked;

};

#endif // COINCONTROL_H
