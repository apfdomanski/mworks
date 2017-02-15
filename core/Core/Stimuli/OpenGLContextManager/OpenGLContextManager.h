/**
 *  OpenGLContextManager.h
 *
 *  Created by David Cox on Thu Dec 05 2002.
 *  Copyright (c) 2002 MIT. All rights reserved.
 */

#ifndef OPENGL_CONTEXT_MANAGER__
#define OPENGL_CONTEXT_MANAGER__

#include "OpenGLContextLock.h"
#include "RegisteredSingleton.h"

#if TARGET_OS_OSX
#  include <Cocoa/Cocoa.h>
#elif TARGET_OS_IPHONE
#  include <UIKit/UIKit.h>
#  include <OpenGLES/EAGL.h>
#endif


BEGIN_NAMESPACE_MW


class OpenGLContextManager : public Component {
    
public:
#if TARGET_OS_OSX
    using PlatformOpenGLContextPtr = NSOpenGLContext *;
    using PlatformOpenGLViewPtr = NSOpenGLView *;
#elif TARGET_OS_IPHONE
    using PlatformOpenGLContextPtr = EAGLContext *;
    using PlatformOpenGLViewPtr = UIView *;
#else
#   error Unsupported platform
#endif
    
    static boost::shared_ptr<OpenGLContextManager> createPlatformOpenGLContextManager();
    
    OpenGLContextManager();
    ~OpenGLContextManager();
    
    // Create a fullscreen context on a particular display
    virtual int newFullscreenContext(int screen_number) = 0;
    
    // Create a "mirror" window (smaller, movable window that displays whatever
    // is on the "main" display) and return its context index
    virtual int newMirrorContext() = 0;
    
    // Release all contexts and associated resources
    virtual void releaseContexts() = 0;
    
    virtual int getNumDisplays() const = 0;
    
    virtual OpenGLContextLock setCurrent(int context_id) = 0;
    virtual void clearCurrent() = 0;
    
    virtual void bindDefaultFramebuffer(int context_id) = 0;
    virtual void flush(int context_id) = 0;
    
    PlatformOpenGLContextPtr getContext(int context_id) const;
    PlatformOpenGLViewPtr getView(int context_id) const;
    PlatformOpenGLViewPtr getFullscreenView() const;
    PlatformOpenGLViewPtr getMirrorView() const;
    
    REGISTERED_SINGLETON_CODE_INJECTION(OpenGLContextManager)
    
protected:
    NSMutableArray *contexts;
    NSMutableArray *views;
    NSMutableArray *windows;
    
};


END_NAMESPACE_MW


#endif
























