#ifndef _TIMER_H
#define _TIMER_H

/*
 *  untitled.h
 *  ExperimentControlCocoa
 *
 *  Created by David Cox on 10/12/04.
 *  Copyright 2004 __MyCompanyName__. All rights reserved.
 *
 */


#include "Scheduler.h"
#include "Lockable.h"
#include "GenericVariable.h"
#include "Component.h"
#include <boost/thread/mutex.hpp>


BEGIN_NAMESPACE_MW


class TimeBase : public mw::Component {

private:
	MWTime time_us;

public:
	TimeBase();
	
	void setTime(MWTime _time);
	void setNow();
	MWTime getTime() const;
    
};


class Timer : public ReadOnlyVariable {

private:
    const boost::shared_ptr<Clock> clock;
    MWTime expirationTimeUS;
    
    mutable boost::mutex mutex;
    using scoped_lock = boost::mutex::scoped_lock;

public:
	Timer(VariableProperties *props = 0);

	void startMS(MWTime howlongms);
	void startUS(MWTime howlongus);
	bool hasExpired() const;
    
    Datum getValue() override;
	
};


END_NAMESPACE_MW


#endif


























