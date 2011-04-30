#ifndef DEFAULT_H
#define	DEFAULT_H

#include "../strategy.h"

class DefaultStrategy : public IStrategy
{
public:
    DefaultStrategy ()
    {
    }
    
    ~DefaultStrategy ()
    {
    }
    
    bool ParseData ( TiXmlDocument&, l3m& )
    {
        return true;
    }
};

#endif	/* DEFAULT_H */
