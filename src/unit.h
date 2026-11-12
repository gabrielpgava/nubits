#ifndef UNIT_H
#define UNIT_H

inline std::string UnitName(unsigned char cUnit)
{
    if (std::isprint(cUnit))
        return std::string(1, cUnit);
    else
        return "??";
}

#endif
