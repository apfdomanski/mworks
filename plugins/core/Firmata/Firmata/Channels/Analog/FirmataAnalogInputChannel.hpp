//
//  FirmataAnalogInputChannel.hpp
//  Firmata
//
//  Created by Christopher Stawarz on 4/7/17.
//  Copyright © 2017 The MWorks Project. All rights reserved.
//

#ifndef FirmataAnalogInputChannel_hpp
#define FirmataAnalogInputChannel_hpp

#include "FirmataAnalogChannel.hpp"


BEGIN_NAMESPACE_MW


class FirmataAnalogInputChannel : public FirmataAnalogChannel {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    using FirmataAnalogChannel::FirmataAnalogChannel;
    
    Direction getDirection() const override { return Direction::Input; }
    
};


END_NAMESPACE_MW


#endif /* FirmataAnalogInputChannel_hpp */
