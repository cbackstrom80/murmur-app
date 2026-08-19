#include "SharedGlVisualizerRoot.h"

#include "MurmurVisualizerComponent.h"

#include <algorithm>

namespace murmur8
{
    SharedGlVisualizerRoot* SharedGlVisualizerRoot::instance_ = nullptr;

    SharedGlVisualizerRoot::SharedGlVisualizerRoot()
    {
    }

    SharedGlVisualizerRoot::~SharedGlVisualizerRoot()
    {
        detach();
        if (instance_ == this)
            instance_ = nullptr;
    }

    void SharedGlVisualizerRoot::attachTo(juce::Component& host)
    {
        if (host_ == &host)
            return;

        detach();

        host_ = &host;
        instance_ = this;
        openGLContext.setRenderer(this);
        openGLContext.setComponentPaintingEnabled(true);
        openGLContext.setContinuousRepainting(false);
        openGLContext.attachTo(host);
        needsRender_ = true;
        startTimerHz(60);
    }

    void SharedGlVisualizerRoot::detach()
    {
        stopTimer();
        openGLContext.detach();
        if (instance_ == this)
            instance_ = nullptr;
        host_ = nullptr;
    }

    void SharedGlVisualizerRoot::registerView(MurmurVisualizerComponent& view)
    {
        if (std::find(views_.begin(), views_.end(), &view) == views_.end())
            views_.push_back(&view);
        needsRender_ = true;
    }

    void SharedGlVisualizerRoot::unregisterView(MurmurVisualizerComponent& view)
    {
        views_.erase(std::remove(views_.begin(), views_.end(), &view), views_.end());
    }

    void SharedGlVisualizerRoot::requestRender()
    {
        needsRender_ = true;
    }

    void SharedGlVisualizerRoot::newOpenGLContextCreated()
    {
        needsRender_ = true;
    }

    void SharedGlVisualizerRoot::renderOpenGL()
    {
        if (host_ == nullptr)
            return;

        juce::OpenGLHelpers::clear(juce::Colours::transparentBlack);

        for (auto* view : views_)
        {
            if (view == nullptr || !view->isShowing())
                continue;

            auto bounds = host_->getLocalArea(view, view->getLocalBounds());
            if (bounds.isEmpty())
                continue;

            view->renderSharedViewport(bounds);
        }

        needsRender_ = false;
    }

    void SharedGlVisualizerRoot::openGLContextClosing()
    {
        for (auto* view : views_)
        {
            if (view != nullptr)
                view->releaseGlResources();
        }
    }

    void SharedGlVisualizerRoot::timerCallback()
    {
        if (needsRender_ && host_ != nullptr)
            openGLContext.triggerRepaint();
    }

    juce::OpenGLContext& SharedGlVisualizerRoot::getContext() noexcept
    {
        return openGLContext;
    }

    int SharedGlVisualizerRoot::getHostHeight() const noexcept
    {
        return host_ != nullptr ? host_->getHeight() : 0;
    }

} // namespace murmur8
