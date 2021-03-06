/**
 * ScarabConnection.cpp
 *
 * Copyright 2004 MIT. All rights reserved.
 */

#include "ScarabConnection.h"
#include "ScarabServices.h"
#include "boost/bind.hpp"
#include <string>
#include "Event.h"


BEGIN_NAMESPACE_MW


ScarabConnection::ScarabConnection(shared_ptr<EventBuffer> _event_buffer, std::string _uri) {
    pipe = NULL;
    
	uri = _uri;
	connected = false;
    connecting = false;
    servicing = false;
    interrupt = false;
    term = false;
    cid = random();
	
	event_buffer = _event_buffer;
}

ScarabConnection::~ScarabConnection() {
    //mdebug("Called connection destructor on id %d", cid);
    if(connected) {
        mdebug("Still connected, disconnecting");
        disconnect();
    }
   
}

shared_ptr<NetworkReturn>  ScarabConnection::accept(ScarabSession * listener) {
    shared_ptr<NetworkReturn>  rc(new NetworkReturn());
    if(connected) {
        rc->setMWorksCode(NR_FAILED);
        rc->setInformation("Connection already connected.");
        return rc;
    }
    connecting = true;
    pipe = scarab_session_accept(listener);
    
    if(pipe == NULL){
        rc->setMWorksCode(NR_ERROR);
        rc->setOSErrorCode(-1);
        rc->setPackageCode(-1);
        rc->setInformation("Unable to accept session (returned NULL)");
        pipe = NULL;
        connecting = false;
        return rc;
    }
    
    if(getScarabError(pipe)) {
        //logDescriptiveScarabMessage(pipe);
        rc->setMWorksCode(NR_ERROR);
        int os = getScarabOSError(pipe);
        int sc = getScarabError(pipe);
        rc->setOSErrorCode(os);
        rc->setPackageCode(sc);
        rc->setInformation(getScarabErrorName(sc));
        rc->appendInformation(getScarabErrorDescription(sc));
        rc->appendInformation(getOSErrorDescription(os));
        pipe = NULL;
        connecting = false;
        return rc;
    }
	
	shared_ptr <Clock> clock = Clock::instance();
    connecting = false;
    connected = true;
    return rc;
}

shared_ptr<NetworkReturn>  ScarabConnection::connect() {
    shared_ptr<NetworkReturn>  rc(new NetworkReturn());
    if(connected) {
        rc->setMWorksCode(NR_FAILED);
        rc->setInformation("Connection already connected.");
        return rc;
    }
	boost::mutex::scoped_lock lock(connectLock);
    connecting = true;
    //mdebug("Trying to connect to %s", uri);

    if(!(pipe = scarab_session_connect(uri.c_str()))) {
        rc->setMWorksCode(NR_FAILED);
		return rc;
	}

    if(getScarabError(pipe)) {
        connecting = false;
        logDescriptiveScarabMessage(pipe);
        rc->setMWorksCode(NR_ERROR);
        int os = getScarabOSError(pipe);
        int sc = getScarabError(pipe);
        rc->setOSErrorCode(os);
        rc->setPackageCode(sc);
        rc->setInformation(getScarabErrorName(sc));
        rc->appendInformation(getScarabErrorDescription(sc));
        rc->appendInformation(getOSErrorDescription(os));
        pipe = NULL;
        return rc;
    }
	
	shared_ptr <Clock> clock = Clock::instance();
    connecting = false;
    connected = true;
    return rc;
}

void ScarabConnection::stopThread() {
	boost::mutex::scoped_lock lock(connectLock);
    if(thread) {
        thread->cancel();
    }
}

void ScarabConnection::disconnect() {
    if(!connected)  return; 
	{
		boost::mutex::scoped_lock lock(interruptLock);
		interrupt = true;
	}
	
	shared_ptr<ScarabConnection> this_one = shared_from_this();
	
	shared_ptr<Scheduler> scheduler = Scheduler::instance();
	
    mdebug("Disconnect called on ID %ld", cid);
    // schedule the connection for termination
    // this gives the socket a chance to send the termination sequence
    scheduler->scheduleMS(FILELINE,
						  0, 
						  200000, 
						  1, 
						  boost::bind(&ScarabConnection::terminate, this_one),
						  M_DEFAULT_NETWORK_PRIORITY, 
						  (MWTime)0, // No warnings 
						  M_DEFAULT_NETWORK_FAIL_SLOP_MS,
						  M_MISSED_EXECUTION_CATCH_UP);
}

char * ScarabConnection::getHostName(bool local) {
    if(local) {
        return scarab_session_local_address(pipe);
    } else {
        return scarab_session_foreign_address(pipe);
    }
}

int ScarabConnection::getPortNumber(bool local) {
    if(local) {
        return scarab_session_local_port(pipe);
    } else {
        return scarab_session_foreign_port(pipe);
    }
}

void ScarabConnection::startThread(int interval) {
    mwarning(M_NETWORK_MESSAGE_DOMAIN, 
					"Illegal call to ScarabConnection::startThread()");
}

int ScarabConnection::service() {
    mwarning(M_NETWORK_MESSAGE_DOMAIN, 
					"Illegal call to ScarabConnection::service()");
    return -1;
}

void ScarabConnection::kill() {
	boost::mutex::scoped_lock lock(connectLock);
    if(thread) {
		thread->cancel();
    }
    if((connected) && (pipe)) {
        mnetwork("Disconnecting on connection with ID %ld", cid);
        scarab_session_close(pipe);
        connected = false;
        pipe = NULL;
		mnetwork("Disconnected on connection with ID %ld", cid);
    } else {
		mnetwork("Broken connection with ID %ld", cid);
		connected = false; // DDC added
	}
}

void* ScarabConnection::terminate() {
    // do nothing until we have sent the termination sequence
    while (!canTerminate()) {
        Clock::instance()->sleepUS(500);
    }
    kill();
	//connected = false; // DDC added
    return NULL;
}


END_NAMESPACE_MW
