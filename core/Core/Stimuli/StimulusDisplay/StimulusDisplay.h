/*
 *  StimulusDisplay.h
 *  MWorksCore
 *
 *  Created by David Cox on 6/10/07.
 *  Copyright 2007 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef STIMULUS_DISPLAY_H_
#define STIMULUS_DISPLAY_H_

#include <map>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

#include "Clock.h"
#include "LinkedList.h"
#include "OpenGLContextLock.h"
#include "OpenGLUtilities.hpp"


BEGIN_NAMESPACE_MW


	// forward declarations
    class Datum;
	class Stimulus;
	class StimulusNode;
    class OpenGLContextManager;
    class VariableCallbackNotification;
	
	/**
	 * StimulusDisplay represents a single abstracted stimulus display.
	 * The idea is that after initialization, you can use routines in here
	 * to set up a display chain, configure high-level changes in the 
	 * OpenGL context, control the drawing to the display.
	 *
	 * All of the details of actually creating OpenGL contexts (i.e. platform
	 * specific stuff) is handled under the hood by the OpenGLContextManager.
	 * This includes actual Cocoa or AGL calls to create, attach, and configure
	 * the contexts.  OpenGLContextManager will handle the details of doing its
	 * best in terms of producing an optimally accelerated and meeting the color
	 * demands of the user (also full-screen AA).  Functions will be provided for 
	 * reporting back exactly what StimulusDisplay is getting.  Eventually, 
	 * a ContextViewer utility will display the status and stats for every
	 * context, and possibly allow users to create and change them. 
	 *
	 * May want to move the actual context itself into this class, at the risk of 
	 * losing perfect portability, for performance's sake.  Right now, context
	 * changes and flushes go through the ContextManager, which involves several
	 * function calls and pointer dereferences.
	 */
		
  
	
	class StimulusDisplay : public boost::enable_shared_from_this<StimulusDisplay>, boost::noncopyable {
    protected:
        std::vector<int> context_ids;
		int current_context_index;
		shared_ptr< LinkedList<StimulusNode> > display_stack;
        
        shared_ptr<Clock> clock;
        shared_ptr<OpenGLContextManager> opengl_context_manager;
		
		boost::mutex display_lock;
        typedef boost::mutex::scoped_lock unique_lock;
        boost::condition_variable refreshCond;
        bool waitingForRefresh;

        bool needDraw;
		
		double left, right, top, bottom; // display bounds
        GLKMatrix4 projectionMatrix;
        GLclampf backgroundRed, backgroundGreen, backgroundBlue;  // background color
        
        shared_ptr<VariableCallbackNotification> stateSystemNotification;
        std::atomic_bool displayUpdatesStarted;
        double mainDisplayRefreshRate;
        MWTime currentOutputTimeUS;
        
        const bool announceIndividualStimuli;
        bool announceStimuliOnImplicitUpdates;
        std::vector< shared_ptr<StimulusNode> > stimsToAnnounce;
        std::vector<Datum> stimAnnouncements;
        
        std::map<int, GLuint> programs, vertexArrays;
        GLuint framebuffer = 0;
        GLuint framebufferTexture = 0;
        
        virtual void prepareContext(int contextIndex);
        virtual void setMainDisplayRefreshRate() = 0;
        void allocateFramebufferStorage();
        void drawStoredFramebuffer(int contextIndex) const;
		
        void glInit();
		void setDisplayBounds();
        void refreshMainDisplay();
        void refreshMirrorDisplay(int contextIndex) const;
        void drawDisplayStack();
        void ensureRefresh(unique_lock &lock);

        void announceDisplayUpdate(bool updateIsExplicit);
        void announceDisplayStack(MWTime time);
        Datum getAnnounceData(bool updateIsExplicit);
        bool shouldAnnounceStimuli(bool updateIsExplicit) { return updateIsExplicit || announceStimuliOnImplicitUpdates; }
        
        void reportSkippedFrames(double numSkippedFrames) const;

        void stateSystemCallback(const Datum &data, MWorksTime time);
        virtual void startDisplayUpdates() = 0;
        virtual void stopDisplayUpdates() = 0;
		
    public:
        static void getDisplayBounds(const Datum &mainScreenInfo,
                                     double &left,
                                     double &right,
                                     double &bottom,
                                     double &top);
		
		explicit StimulusDisplay(bool announceIndividualStimuli);
		virtual ~StimulusDisplay();
		
		void addContext(int _context_id);
		
        // These are meant to be used by stimulus classes that need to manage per-context GL resources.
        // They should *not* be used internally by StimulusDisplay.
        int getNContexts() { return (context_ids.empty() ? 0 : 1); }
		OpenGLContextLock setCurrent(int i);	
		int getCurrentContextIndex() { return current_context_index; }
        
        void getCurrentViewportSize(GLint &width, GLint &height);
		
        shared_ptr<StimulusNode> addStimulus(shared_ptr<Stimulus> stim);
		void addStimulusNode(shared_ptr<StimulusNode> stimnode);
		
        void setBackgroundColor(GLclampf red, GLclampf green, GLclampf blue);
        void setAnnounceStimuliOnImplicitUpdates(bool announceStimuliOnImplicitUpdates);
		MWTime updateDisplay();
		void clearDisplay();
        void getDisplayBounds(double &left, double &right, double &bottom, double &top);
        const GLKMatrix4& getProjectionMatrix() const { return projectionMatrix; }
        double getMainDisplayRefreshRate() const { return mainDisplayRefreshRate; }
        MWTime getCurrentOutputTimeUS() const { return currentOutputTimeUS; }
        
        static boost::shared_ptr<StimulusDisplay> createPlatformStimulusDisplay(bool announceIndividualStimuli);
        static shared_ptr<StimulusDisplay> getCurrentStimulusDisplay();
	};


END_NAMESPACE_MW


#endif


