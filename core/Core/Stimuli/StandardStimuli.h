/**
 * StandardStimuli.h
 *
 * Description:
 *
 * NOTE:  All objects that inherit from Stimulus and contain pointer data 
 * must implement a copy constructor and a destructor.  It is also advised to
 * make a private = operator.
 *
 * History:
 * Dave Cox on ??/??/?? - Created.
 * Paul Jankunas on 05/23/05 - Fixed spacing.  Added copy constructors, 
 *                          destructors, and = operator to object that are
 *                          added to ExpandableLists.
 *
 * Copyright 2005 MIT.  All rights reserved.
 */

#ifndef _STANDARD_STIMULI_H
#define _STANDARD_STIMULI_H

#include "Stimulus.h"


BEGIN_NAMESPACE_MW


class BlankScreen : public Stimulus {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    explicit BlankScreen(const Map<ParameterValue> &parameters);
    
    void draw(shared_ptr<StimulusDisplay> display) override;
    Datum getCurrentAnnounceDrawData() override;
    
private:
    shared_ptr<Variable> r;
    shared_ptr<Variable> g;
    shared_ptr<Variable> b;
    float last_r,last_g,last_b;
    
};


class BasicTransformStimulus : public Stimulus, boost::noncopyable {
    
public:
    static const std::string X_SIZE;
    static const std::string Y_SIZE;
    static const std::string X_POSITION;
    static const std::string Y_POSITION;
    static const std::string ROTATION;
    static const std::string ALPHA_MULTIPLIER;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit BasicTransformStimulus(const Map<ParameterValue> &parameters);
    BasicTransformStimulus(const std::string &_tag,
                           const shared_ptr<Variable> &_xoffset,
                           const shared_ptr<Variable> &_yoffset,
                           const shared_ptr<Variable> &_xscale,
                           const shared_ptr<Variable> &_yscale,
                           const shared_ptr<Variable> &_rot,
                           const shared_ptr<Variable> &_alpha);
    
    void load(shared_ptr<StimulusDisplay> display) override;
    void unload(shared_ptr<StimulusDisplay> display) override;
    void draw(shared_ptr<StimulusDisplay> display) override;
    Datum getCurrentAnnounceDrawData() override;
    
protected:
    static constexpr GLint numVertices = 4;
    static constexpr GLint componentsPerVertex = 2;
    using VertexPositionArray = std::array<GLfloat, numVertices*componentsPerVertex>;
    
    virtual gl::Shader getVertexShader() const = 0;
    virtual gl::Shader getFragmentShader() const = 0;
    
    virtual VertexPositionArray getVertexPositions() const;
    virtual GLKMatrix4 getCurrentMVPMatrix(const GLKMatrix4 &projectionMatrix) const;
    
    virtual void prepare(const boost::shared_ptr<StimulusDisplay> &display) { }
    virtual void destroy(const boost::shared_ptr<StimulusDisplay> &display) { }
    virtual void preDraw(const boost::shared_ptr<StimulusDisplay> &display) { }
    virtual void postDraw(const boost::shared_ptr<StimulusDisplay> &display) { }
    
    const shared_ptr<Variable> xoffset;
    const shared_ptr<Variable> yoffset;
    
    const shared_ptr<Variable> xscale;
    const shared_ptr<Variable> yscale;
    
    const shared_ptr<Variable> rotation;
    const shared_ptr<Variable> alpha_multiplier;
    
    float current_posx, current_posy, current_sizex, current_sizey, current_rot, current_alpha;
    float last_posx, last_posy, last_sizex, last_sizey, last_rot, last_alpha;
    
    GLuint program = 0;
    GLint mvpMatrixUniformLocation = -1;
    GLuint vertexArray = 0;
    GLuint vertexPositionBuffer = 0;
    
};


class ColoredTransformStimulus : public BasicTransformStimulus {
    
public:
    static const std::string COLOR;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit ColoredTransformStimulus(const Map<ParameterValue> &parameters);
    
    void draw(shared_ptr<StimulusDisplay> display) override;
    Datum getCurrentAnnounceDrawData() override;
    
protected:
    void prepare(const boost::shared_ptr<StimulusDisplay> &display) override;
    void preDraw(const boost::shared_ptr<StimulusDisplay> &display) override;
    void postDraw(const boost::shared_ptr<StimulusDisplay> &display) override;
    
    shared_ptr<Variable> r;
    shared_ptr<Variable> g;
    shared_ptr<Variable> b;
    
    float current_r, current_g, current_b;
    float last_r, last_g, last_b;
    
    GLint colorUniformLocation = -1;
    
};


class RectangleStimulus : public ColoredTransformStimulus {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    using ColoredTransformStimulus::ColoredTransformStimulus;
    
    Datum getCurrentAnnounceDrawData() override;
    
private:
    gl::Shader getVertexShader() const override;
    gl::Shader getFragmentShader() const override;
    
};


class CircleStimulus : public ColoredTransformStimulus {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    using ColoredTransformStimulus::ColoredTransformStimulus;
    
    Datum getCurrentAnnounceDrawData() override;
    
private:
    gl::Shader getVertexShader() const override;
    gl::Shader getFragmentShader() const override;
    
};


class ImageStimulus : public BasicTransformStimulus {
    
public:
    static const std::string PATH;
    
    static VertexPositionArray getVertexPositions(double aspectRatio);
    
    static void describeComponent(ComponentInfo &info);
    
    explicit ImageStimulus(const Map<ParameterValue> &parameters);
    
    Datum getCurrentAnnounceDrawData() override;
    
private:
    gl::Shader getVertexShader() const override;
    gl::Shader getFragmentShader() const override;
    
    VertexPositionArray getVertexPositions() const override;
    
    void prepare(const boost::shared_ptr<StimulusDisplay> &display) override;
    void destroy(const boost::shared_ptr<StimulusDisplay> &display) override;
    void preDraw(const boost::shared_ptr<StimulusDisplay> &display) override;
    void postDraw(const boost::shared_ptr<StimulusDisplay> &display) override;
    
    std::string filename;
    std::string fileHash;
    double aspectRatio;
    
    GLint alphaUniformLocation = -1;
    GLuint texture = 0;
    GLuint texCoordsBuffer = 0;
    
};


END_NAMESPACE_MW


#endif























