#pragma once

#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_opengl/juce_opengl.h>

namespace murmur8
{

class MurmurVisualizerComponent;

/** One OpenGL context on the editor host — GL draws behind child components, viewports via scissor. */
class SharedGlVisualizerRoot : private juce::OpenGLRenderer,
                                 private juce::Timer
{
public:
    SharedGlVisualizerRoot();
    ~SharedGlVisualizerRoot() override;

    void attachTo(juce::Component& host);
    void detach();

    static SharedGlVisualizerRoot* getInstance() noexcept { return instance_; }

    void registerView(MurmurVisualizerComponent& view);
    void unregisterView(MurmurVisualizerComponent& view);
    void requestRender();

    [[nodiscard]] juce::OpenGLContext& getContext() noexcept;
    [[nodiscard]] int getHostHeight() const noexcept;

private:
    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;
    void timerCallback() override;

    static SharedGlVisualizerRoot* instance_;
    juce::Component* host_ = nullptr;
    juce::OpenGLContext openGLContext;
    std::vector<MurmurVisualizerComponent*> views_;
    bool needsRender_ = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedGlVisualizerRoot)
};

} // namespace murmur8
