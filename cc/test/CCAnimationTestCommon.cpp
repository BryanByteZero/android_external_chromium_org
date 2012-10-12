// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "config.h"

#include "CCAnimationTestCommon.h"

#include "CCKeyframedAnimationCurve.h"
#include "CCLayerAnimationController.h"
#include "CCLayerImpl.h"
#include "LayerChromium.h"
#include <public/WebTransformOperations.h>

using namespace cc;

namespace {

template <class Target>
void addOpacityTransition(Target& target, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    scoped_ptr<CCKeyframedFloatAnimationCurve> curve(CCKeyframedFloatAnimationCurve::create());

    scoped_ptr<CCTimingFunction> func;
    if (!useTimingFunction)
        func = CCEaseTimingFunction::create();
    if (duration > 0)
        curve->addKeyframe(CCFloatKeyframe::create(0, startOpacity, func.Pass()));
    curve->addKeyframe(CCFloatKeyframe::create(duration, endOpacity, scoped_ptr<cc::CCTimingFunction>()));

    scoped_ptr<CCActiveAnimation> animation(CCActiveAnimation::create(curve.PassAs<CCAnimationCurve>(), 0, 0, CCActiveAnimation::Opacity));
    animation->setNeedsSynchronizedStartTime(true);

    target.addAnimation(animation.Pass());
}

template <class Target>
void addAnimatedTransform(Target& target, double duration, int deltaX, int deltaY)
{
    static int id = 0;
    scoped_ptr<CCKeyframedTransformAnimationCurve> curve(CCKeyframedTransformAnimationCurve::create());

    if (duration > 0) {
        WebKit::WebTransformOperations startOperations;
        startOperations.appendTranslate(deltaX, deltaY, 0);
        curve->addKeyframe(CCTransformKeyframe::create(0, startOperations, scoped_ptr<cc::CCTimingFunction>()));
    }

    WebKit::WebTransformOperations operations;
    operations.appendTranslate(deltaX, deltaY, 0);
    curve->addKeyframe(CCTransformKeyframe::create(duration, operations, scoped_ptr<cc::CCTimingFunction>()));

    scoped_ptr<CCActiveAnimation> animation(CCActiveAnimation::create(curve.PassAs<CCAnimationCurve>(), id++, 0, CCActiveAnimation::Transform));
    animation->setNeedsSynchronizedStartTime(true);

    target.addAnimation(animation.Pass());
}

} // namespace

namespace WebKitTests {

FakeFloatAnimationCurve::FakeFloatAnimationCurve()
    : m_duration(1)
{
}

FakeFloatAnimationCurve::FakeFloatAnimationCurve(double duration)
    : m_duration(duration)
{
}

FakeFloatAnimationCurve::~FakeFloatAnimationCurve()
{
}

double FakeFloatAnimationCurve::duration() const
{
    return m_duration;
}

float FakeFloatAnimationCurve::getValue(double now) const
{
    return 0;
}

scoped_ptr<cc::CCAnimationCurve> FakeFloatAnimationCurve::clone() const
{
    return make_scoped_ptr(new FakeFloatAnimationCurve).PassAs<cc::CCAnimationCurve>();
}

FakeTransformTransition::FakeTransformTransition(double duration)
    : m_duration(duration)
{
}

FakeTransformTransition::~FakeTransformTransition()
{
}

double FakeTransformTransition::duration() const
{
    return m_duration;
}

WebKit::WebTransformationMatrix FakeTransformTransition::getValue(double time) const
{
    return WebKit::WebTransformationMatrix();
}

scoped_ptr<cc::CCAnimationCurve> FakeTransformTransition::clone() const
{
    return make_scoped_ptr(new FakeTransformTransition(*this)).PassAs<cc::CCAnimationCurve>();
}


FakeFloatTransition::FakeFloatTransition(double duration, float from, float to)
    : m_duration(duration)
    , m_from(from)
    , m_to(to)
{
}

FakeFloatTransition::~FakeFloatTransition()
{
}

double FakeFloatTransition::duration() const
{
    return m_duration;
}

float FakeFloatTransition::getValue(double time) const
{
    time /= m_duration;
    if (time >= 1)
        time = 1;
    return (1 - time) * m_from + time * m_to;
}

FakeLayerAnimationControllerClient::FakeLayerAnimationControllerClient()
    : m_opacity(0)
{
}

FakeLayerAnimationControllerClient::~FakeLayerAnimationControllerClient()
{
}

int FakeLayerAnimationControllerClient::id() const
{
    return 0;
}

void FakeLayerAnimationControllerClient::setOpacityFromAnimation(float opacity)
{
    m_opacity = opacity;
}

float FakeLayerAnimationControllerClient::opacity() const
{
    return m_opacity;
}

void FakeLayerAnimationControllerClient::setTransformFromAnimation(const WebKit::WebTransformationMatrix& transform)
{
    m_transform = transform;
}

const WebKit::WebTransformationMatrix& FakeLayerAnimationControllerClient::transform() const
{
    return m_transform;
}

scoped_ptr<cc::CCAnimationCurve> FakeFloatTransition::clone() const
{
    return make_scoped_ptr(new FakeFloatTransition(*this)).PassAs<cc::CCAnimationCurve>();
}

void addOpacityTransitionToController(cc::CCLayerAnimationController& controller, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    addOpacityTransition(controller, duration, startOpacity, endOpacity, useTimingFunction);
}

void addAnimatedTransformToController(cc::CCLayerAnimationController& controller, double duration, int deltaX, int deltaY)
{
    addAnimatedTransform(controller, duration, deltaX, deltaY);
}

void addOpacityTransitionToLayer(cc::LayerChromium& layer, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    addOpacityTransition(layer, duration, startOpacity, endOpacity, useTimingFunction);
}

void addOpacityTransitionToLayer(cc::CCLayerImpl& layer, double duration, float startOpacity, float endOpacity, bool useTimingFunction)
{
    addOpacityTransition(*layer.layerAnimationController(), duration, startOpacity, endOpacity, useTimingFunction);
}

void addAnimatedTransformToLayer(cc::LayerChromium& layer, double duration, int deltaX, int deltaY)
{
    addAnimatedTransform(layer, duration, deltaX, deltaY);
}

void addAnimatedTransformToLayer(cc::CCLayerImpl& layer, double duration, int deltaX, int deltaY)
{
    addAnimatedTransform(*layer.layerAnimationController(), duration, deltaX, deltaY);
}

} // namespace WebKitTests
