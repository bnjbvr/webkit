/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SVGResources.h"

#include "FilterOperation.h"
#include "RenderSVGResourceClipper.h"
#include "RenderSVGResourceFilter.h"
#include "RenderSVGResourceMarker.h"
#include "RenderSVGResourceMasker.h"
#include "RenderSVGRoot.h"
#include "SVGGradientElement.h"
#include "SVGNames.h"
#include "SVGPatternElement.h"
#include "SVGRenderStyle.h"
#include "SVGURIReference.h"

#if ENABLE(TREE_DEBUGGING)
#include <stdio.h>
#endif

namespace WebCore {

SVGResources::SVGResources()
    : m_linkedResource(0)
{
}

static HashSet<AtomicString>& clipperFilterMaskerTags()
{
    static NeverDestroyed<HashSet<AtomicString>> s_tagList;
    if (s_tagList.get().isEmpty()) {
        // "container elements": http://www.w3.org/TR/SVG11/intro.html#TermContainerElement
        // "graphics elements" : http://www.w3.org/TR/SVG11/intro.html#TermGraphicsElement
        s_tagList.get().add(SVGNames::aTag.localName());
        s_tagList.get().add(SVGNames::circleTag.localName());
        s_tagList.get().add(SVGNames::ellipseTag.localName());
        s_tagList.get().add(SVGNames::glyphTag.localName());
        s_tagList.get().add(SVGNames::gTag.localName());
        s_tagList.get().add(SVGNames::imageTag.localName());
        s_tagList.get().add(SVGNames::lineTag.localName());
        s_tagList.get().add(SVGNames::markerTag.localName());
        s_tagList.get().add(SVGNames::maskTag.localName());
        s_tagList.get().add(SVGNames::missing_glyphTag.localName());
        s_tagList.get().add(SVGNames::pathTag.localName());
        s_tagList.get().add(SVGNames::polygonTag.localName());
        s_tagList.get().add(SVGNames::polylineTag.localName());
        s_tagList.get().add(SVGNames::rectTag.localName());
        s_tagList.get().add(SVGNames::svgTag.localName());
        s_tagList.get().add(SVGNames::textTag.localName());
        s_tagList.get().add(SVGNames::useTag.localName());

        // Not listed in the definitions is the clipPath element, the SVG spec says though:
        // The "clipPath" element or any of its children can specify property "clip-path".
        // So we have to add clipPathTag here, otherwhise clip-path on clipPath will fail.
        // (Already mailed SVG WG, waiting for a solution)
        s_tagList.get().add(SVGNames::clipPathTag.localName());

        // Not listed in the definitions are the text content elements, though filter/clipper/masker on tspan/text/.. is allowed.
        // (Already mailed SVG WG, waiting for a solution)
        s_tagList.get().add(SVGNames::altGlyphTag.localName());
        s_tagList.get().add(SVGNames::textPathTag.localName());
        s_tagList.get().add(SVGNames::trefTag.localName());
        s_tagList.get().add(SVGNames::tspanTag.localName());

        // Not listed in the definitions is the foreignObject element, but clip-path
        // is a supported attribute.
        s_tagList.get().add(SVGNames::foreignObjectTag.localName());

        // Elements that we ignore, as it doesn't make any sense.
        // defs, pattern, switch (FIXME: Mail SVG WG about these)
        // symbol (is converted to a svg element, when referenced by use, we can safely ignore it.)
    }

    return s_tagList;
}

static HashSet<AtomicString>& markerTags()
{
    static NeverDestroyed<HashSet<AtomicString>> s_tagList;
    if (s_tagList.get().isEmpty()) {
        s_tagList.get().add(SVGNames::lineTag.localName());
        s_tagList.get().add(SVGNames::pathTag.localName());
        s_tagList.get().add(SVGNames::polygonTag.localName());
        s_tagList.get().add(SVGNames::polylineTag.localName());
    }

    return s_tagList;
}

static HashSet<AtomicString>& fillAndStrokeTags()
{
    static NeverDestroyed<HashSet<AtomicString>> s_tagList;
    if (s_tagList.get().isEmpty()) {
        s_tagList.get().add(SVGNames::altGlyphTag.localName());
        s_tagList.get().add(SVGNames::circleTag.localName());
        s_tagList.get().add(SVGNames::ellipseTag.localName());
        s_tagList.get().add(SVGNames::lineTag.localName());
        s_tagList.get().add(SVGNames::pathTag.localName());
        s_tagList.get().add(SVGNames::polygonTag.localName());
        s_tagList.get().add(SVGNames::polylineTag.localName());
        s_tagList.get().add(SVGNames::rectTag.localName());
        s_tagList.get().add(SVGNames::textTag.localName());
        s_tagList.get().add(SVGNames::textPathTag.localName());
        s_tagList.get().add(SVGNames::trefTag.localName());
        s_tagList.get().add(SVGNames::tspanTag.localName());
    }

    return s_tagList;
}

static HashSet<AtomicString>& chainableResourceTags()
{
    static NeverDestroyed<HashSet<AtomicString>> s_tagList;
    if (s_tagList.get().isEmpty()) {
        s_tagList.get().add(SVGNames::linearGradientTag.localName());
        s_tagList.get().add(SVGNames::filterTag.localName());
        s_tagList.get().add(SVGNames::patternTag.localName());
        s_tagList.get().add(SVGNames::radialGradientTag.localName());
    }

    return s_tagList;
}

static inline String targetReferenceFromResource(SVGElement& element)
{
    String target;
    if (is<SVGPatternElement>(element))
        target = downcast<SVGPatternElement>(element).href();
    else if (is<SVGGradientElement>(element))
        target = downcast<SVGGradientElement>(element).href();
    else if (is<SVGFilterElement>(element))
        target = downcast<SVGFilterElement>(element).href();
    else
        ASSERT_NOT_REACHED();

    return SVGURIReference::fragmentIdentifierFromIRIString(target, element.document());
}

static inline RenderSVGResourceContainer* paintingResourceFromSVGPaint(Document& document, const SVGPaintType& paintType, const String& paintUri, AtomicString& id, bool& hasPendingResource)
{
    if (paintType != SVG_PAINTTYPE_URI && paintType != SVG_PAINTTYPE_URI_RGBCOLOR && paintType != SVG_PAINTTYPE_URI_CURRENTCOLOR)
        return 0;

    id = SVGURIReference::fragmentIdentifierFromIRIString(paintUri, document);
    RenderSVGResourceContainer* container = getRenderSVGResourceContainerById(document, id);
    if (!container) {
        hasPendingResource = true;
        return 0;
    }

    RenderSVGResourceType resourceType = container->resourceType();
    if (resourceType != PatternResourceType && resourceType != LinearGradientResourceType && resourceType != RadialGradientResourceType)
        return 0;

    return container;
}

static inline void registerPendingResource(SVGDocumentExtensions& extensions, const AtomicString& id, SVGElement& element)
{
    extensions.addPendingResource(id, &element);
}

bool SVGResources::buildCachedResources(const RenderElement& renderer, const RenderStyle& style)
{
    ASSERT(renderer.element());
    ASSERT_WITH_SECURITY_IMPLICATION(renderer.element()->isSVGElement());

    if (!renderer.element())
        return false;

    auto& element = downcast<SVGElement>(*renderer.element());

    Document& document = element.document();

    SVGDocumentExtensions& extensions = document.accessSVGExtensions();

    const AtomicString& tagName = element.localName();
    if (tagName.isNull())
        return false;

    const SVGRenderStyle& svgStyle = style.svgStyle();

    bool foundResources = false;
    if (clipperFilterMaskerTags().contains(tagName)) {
        if (svgStyle.hasClipper()) {
            AtomicString id(svgStyle.clipperResource());
            if (setClipper(getRenderSVGResourceById<RenderSVGResourceClipper>(document, id)))
                foundResources = true;
            else
                registerPendingResource(extensions, id, element);
        }

        if (style.hasFilter()) {
            const FilterOperations& filterOperations = style.filter();
            if (filterOperations.size() == 1) {
                const FilterOperation& filterOperation = *filterOperations.at(0);
                if (filterOperation.type() == FilterOperation::REFERENCE) {
                    const auto& referenceFilterOperation = downcast<ReferenceFilterOperation>(filterOperation);
                    AtomicString id = SVGURIReference::fragmentIdentifierFromIRIString(referenceFilterOperation.url(), element.document());
                    if (setFilter(getRenderSVGResourceById<RenderSVGResourceFilter>(document, id)))
                        foundResources = true;
                    else
                        registerPendingResource(extensions, id, element);
                }
            }
        }

        if (svgStyle.hasMasker()) {
            AtomicString id(svgStyle.maskerResource());
            if (setMasker(getRenderSVGResourceById<RenderSVGResourceMasker>(document, id)))
                foundResources = true;
            else
                registerPendingResource(extensions, id, element);
        }
    }

    if (markerTags().contains(tagName) && svgStyle.hasMarkers()) {
        AtomicString markerStartId(svgStyle.markerStartResource());
        if (setMarkerStart(getRenderSVGResourceById<RenderSVGResourceMarker>(document, markerStartId)))
            foundResources = true;
        else
            registerPendingResource(extensions, markerStartId, element);

        AtomicString markerMidId(svgStyle.markerMidResource());
        if (setMarkerMid(getRenderSVGResourceById<RenderSVGResourceMarker>(document, markerMidId)))
            foundResources = true;
        else
            registerPendingResource(extensions, markerMidId, element);

        AtomicString markerEndId(svgStyle.markerEndResource());
        if (setMarkerEnd(getRenderSVGResourceById<RenderSVGResourceMarker>(document, markerEndId)))
            foundResources = true;
        else
            registerPendingResource(extensions, markerEndId, element);
    }

    if (fillAndStrokeTags().contains(tagName)) {
        if (svgStyle.hasFill()) {
            bool hasPendingResource = false;
            AtomicString id;
            if (setFill(paintingResourceFromSVGPaint(document, svgStyle.fillPaintType(), svgStyle.fillPaintUri(), id, hasPendingResource)))
                foundResources = true;
            else if (hasPendingResource)
                registerPendingResource(extensions, id, element);
        }

        if (svgStyle.hasStroke()) {
            bool hasPendingResource = false;
            AtomicString id;
            if (setStroke(paintingResourceFromSVGPaint(document, svgStyle.strokePaintType(), svgStyle.strokePaintUri(), id, hasPendingResource)))
                foundResources = true;
            else if (hasPendingResource)
                registerPendingResource(extensions, id, element);
        }
    }

    if (chainableResourceTags().contains(tagName)) {
        AtomicString id(targetReferenceFromResource(element));
        if (setLinkedResource(getRenderSVGResourceContainerById(document, id)))
            foundResources = true;
        else
            registerPendingResource(extensions, id, element);
    }

    return foundResources;
}

void SVGResources::layoutDifferentRootIfNeeded(const RenderSVGRoot* svgRoot)
{
    if (clipper() && svgRoot != SVGRenderSupport::findTreeRootObject(*clipper()))
        clipper()->layoutIfNeeded();

    if (masker() && svgRoot != SVGRenderSupport::findTreeRootObject(*masker()))
        masker()->layoutIfNeeded();

    if (filter() && svgRoot != SVGRenderSupport::findTreeRootObject(*filter()))
        filter()->layoutIfNeeded();

    if (markerStart() && svgRoot != SVGRenderSupport::findTreeRootObject(*markerStart()))
        markerStart()->layoutIfNeeded();

    if (markerMid() && svgRoot != SVGRenderSupport::findTreeRootObject(*markerMid()))
        markerMid()->layoutIfNeeded();

    if (markerEnd() && svgRoot != SVGRenderSupport::findTreeRootObject(*markerEnd()))
        markerEnd()->layoutIfNeeded();
}

bool SVGResources::markerReverseStart() const
{
    return m_markerData
        && m_markerData->markerStart
        && m_markerData->markerStart->markerElement().orientType() == SVGMarkerOrientAutoStartReverse;
}

void SVGResources::removeClientFromCache(RenderElement& renderer, bool markForInvalidation) const
{
    if (!m_clipperFilterMaskerData && !m_markerData && !m_fillStrokeData && !m_linkedResource)
        return;

    if (m_linkedResource) {
        ASSERT(!m_clipperFilterMaskerData);
        ASSERT(!m_markerData);
        ASSERT(!m_fillStrokeData);
        m_linkedResource->removeClientFromCache(renderer, markForInvalidation);
        return;
    }

    if (m_clipperFilterMaskerData) {
        if (m_clipperFilterMaskerData->clipper)
            m_clipperFilterMaskerData->clipper->removeClientFromCache(renderer, markForInvalidation);
        if (m_clipperFilterMaskerData->filter)
            m_clipperFilterMaskerData->filter->removeClientFromCache(renderer, markForInvalidation);
        if (m_clipperFilterMaskerData->masker)
            m_clipperFilterMaskerData->masker->removeClientFromCache(renderer, markForInvalidation);
    }

    if (m_markerData) {
        if (m_markerData->markerStart)
            m_markerData->markerStart->removeClientFromCache(renderer, markForInvalidation);
        if (m_markerData->markerMid)
            m_markerData->markerMid->removeClientFromCache(renderer, markForInvalidation);
        if (m_markerData->markerEnd)
            m_markerData->markerEnd->removeClientFromCache(renderer, markForInvalidation);
    }

    if (m_fillStrokeData) {
        if (m_fillStrokeData->fill)
            m_fillStrokeData->fill->removeClientFromCache(renderer, markForInvalidation);
        if (m_fillStrokeData->stroke)
            m_fillStrokeData->stroke->removeClientFromCache(renderer, markForInvalidation);
    }
}

void SVGResources::resourceDestroyed(RenderSVGResourceContainer& resource)
{
    if (!m_clipperFilterMaskerData && !m_markerData && !m_fillStrokeData && !m_linkedResource)
        return;

    if (m_linkedResource == &resource) {
        ASSERT(!m_clipperFilterMaskerData);
        ASSERT(!m_markerData);
        ASSERT(!m_fillStrokeData);
        m_linkedResource->removeAllClientsFromCache();
        m_linkedResource = 0;
        return;
    }

    switch (resource.resourceType()) {
    case MaskerResourceType:
        if (!m_clipperFilterMaskerData)
            break;
        if (m_clipperFilterMaskerData->masker == &resource) {
            m_clipperFilterMaskerData->masker->removeAllClientsFromCache();
            m_clipperFilterMaskerData->masker = 0;
        }
        break;
    case MarkerResourceType:
        if (!m_markerData)
            break;
        if (m_markerData->markerStart == &resource) {
            m_markerData->markerStart->removeAllClientsFromCache();
            m_markerData->markerStart = 0;
        }
        if (m_markerData->markerMid == &resource) {
            m_markerData->markerMid->removeAllClientsFromCache();
            m_markerData->markerMid = 0;
        }
        if (m_markerData->markerEnd == &resource) {
            m_markerData->markerEnd->removeAllClientsFromCache();
            m_markerData->markerEnd = 0;
        }
        break;
    case PatternResourceType:
    case LinearGradientResourceType:
    case RadialGradientResourceType:
        if (!m_fillStrokeData)
            break;
        if (m_fillStrokeData->fill == &resource) {
            m_fillStrokeData->fill->removeAllClientsFromCache();
            m_fillStrokeData->fill = 0;
        }
        if (m_fillStrokeData->stroke == &resource) {
            m_fillStrokeData->stroke->removeAllClientsFromCache();
            m_fillStrokeData->stroke = 0;
        }
        break;
    case FilterResourceType:
        if (!m_clipperFilterMaskerData)
            break;
        if (m_clipperFilterMaskerData->filter == &resource) {
            m_clipperFilterMaskerData->filter->removeAllClientsFromCache();
            m_clipperFilterMaskerData->filter = 0;
        }
        break;
    case ClipperResourceType:
        if (!m_clipperFilterMaskerData)
            break; 
        if (m_clipperFilterMaskerData->clipper == &resource) {
            m_clipperFilterMaskerData->clipper->removeAllClientsFromCache();
            m_clipperFilterMaskerData->clipper = 0;
        }
        break;
    case SolidColorResourceType:
        ASSERT_NOT_REACHED();
    }
}

void SVGResources::buildSetOfResources(HashSet<RenderSVGResourceContainer*>& set)
{
    if (!m_clipperFilterMaskerData && !m_markerData && !m_fillStrokeData && !m_linkedResource)
        return;

    if (m_linkedResource) {
        ASSERT(!m_clipperFilterMaskerData);
        ASSERT(!m_markerData);
        ASSERT(!m_fillStrokeData);
        set.add(m_linkedResource);
        return;
    }

    if (m_clipperFilterMaskerData) {
        if (m_clipperFilterMaskerData->clipper)
            set.add(m_clipperFilterMaskerData->clipper);
        if (m_clipperFilterMaskerData->filter)
            set.add(m_clipperFilterMaskerData->filter);
        if (m_clipperFilterMaskerData->masker)
            set.add(m_clipperFilterMaskerData->masker);
    }

    if (m_markerData) {
        if (m_markerData->markerStart)
            set.add(m_markerData->markerStart);
        if (m_markerData->markerMid)
            set.add(m_markerData->markerMid);
        if (m_markerData->markerEnd)
            set.add(m_markerData->markerEnd);
    }

    if (m_fillStrokeData) {
        if (m_fillStrokeData->fill)
            set.add(m_fillStrokeData->fill);
        if (m_fillStrokeData->stroke)
            set.add(m_fillStrokeData->stroke);
    }
}

bool SVGResources::setClipper(RenderSVGResourceClipper* clipper)
{
    if (!clipper)
        return false;

    ASSERT(clipper->resourceType() == ClipperResourceType);

    if (!m_clipperFilterMaskerData)
        m_clipperFilterMaskerData = std::make_unique<ClipperFilterMaskerData>();

    m_clipperFilterMaskerData->clipper = clipper;
    return true;
}

void SVGResources::resetClipper()
{
    ASSERT(m_clipperFilterMaskerData);
    ASSERT(m_clipperFilterMaskerData->clipper);
    m_clipperFilterMaskerData->clipper = 0;
}

bool SVGResources::setFilter(RenderSVGResourceFilter* filter)
{
    if (!filter)
        return false;

    ASSERT(filter->resourceType() == FilterResourceType);

    if (!m_clipperFilterMaskerData)
        m_clipperFilterMaskerData = std::make_unique<ClipperFilterMaskerData>();

    m_clipperFilterMaskerData->filter = filter;
    return true;
}

void SVGResources::resetFilter()
{
    ASSERT(m_clipperFilterMaskerData);
    ASSERT(m_clipperFilterMaskerData->filter);
    m_clipperFilterMaskerData->filter = 0;
}

bool SVGResources::setMarkerStart(RenderSVGResourceMarker* markerStart)
{
    if (!markerStart)
        return false;

    ASSERT(markerStart->resourceType() == MarkerResourceType);

    if (!m_markerData)
        m_markerData = std::make_unique<MarkerData>();

    m_markerData->markerStart = markerStart;
    return true;
}

void SVGResources::resetMarkerStart()
{
    ASSERT(m_markerData);
    ASSERT(m_markerData->markerStart);
    m_markerData->markerStart = 0;
}

bool SVGResources::setMarkerMid(RenderSVGResourceMarker* markerMid)
{
    if (!markerMid)
        return false;

    ASSERT(markerMid->resourceType() == MarkerResourceType);

    if (!m_markerData)
        m_markerData = std::make_unique<MarkerData>();

    m_markerData->markerMid = markerMid;
    return true;
}

void SVGResources::resetMarkerMid()
{
    ASSERT(m_markerData);
    ASSERT(m_markerData->markerMid);
    m_markerData->markerMid = 0;
}

bool SVGResources::setMarkerEnd(RenderSVGResourceMarker* markerEnd)
{
    if (!markerEnd)
        return false;

    ASSERT(markerEnd->resourceType() == MarkerResourceType);

    if (!m_markerData)
        m_markerData = std::make_unique<MarkerData>();

    m_markerData->markerEnd = markerEnd;
    return true;
}

void SVGResources::resetMarkerEnd()
{
    ASSERT(m_markerData);
    ASSERT(m_markerData->markerEnd);
    m_markerData->markerEnd = 0;
}

bool SVGResources::setMasker(RenderSVGResourceMasker* masker)
{
    if (!masker)
        return false;

    ASSERT(masker->resourceType() == MaskerResourceType);

    if (!m_clipperFilterMaskerData)
        m_clipperFilterMaskerData = std::make_unique<ClipperFilterMaskerData>();

    m_clipperFilterMaskerData->masker = masker;
    return true;
}

void SVGResources::resetMasker()
{
    ASSERT(m_clipperFilterMaskerData);
    ASSERT(m_clipperFilterMaskerData->masker);
    m_clipperFilterMaskerData->masker = 0;
}

bool SVGResources::setFill(RenderSVGResourceContainer* fill)
{
    if (!fill)
        return false;

    ASSERT(fill->resourceType() == PatternResourceType
           || fill->resourceType() == LinearGradientResourceType
           || fill->resourceType() == RadialGradientResourceType);

    if (!m_fillStrokeData)
        m_fillStrokeData = std::make_unique<FillStrokeData>();

    m_fillStrokeData->fill = fill;
    return true;
}

void SVGResources::resetFill()
{
    ASSERT(m_fillStrokeData);
    ASSERT(m_fillStrokeData->fill);
    m_fillStrokeData->fill = 0;
}

bool SVGResources::setStroke(RenderSVGResourceContainer* stroke)
{
    if (!stroke)
        return false;

    ASSERT(stroke->resourceType() == PatternResourceType
           || stroke->resourceType() == LinearGradientResourceType
           || stroke->resourceType() == RadialGradientResourceType);

    if (!m_fillStrokeData)
        m_fillStrokeData = std::make_unique<FillStrokeData>();

    m_fillStrokeData->stroke = stroke;
    return true;
}

void SVGResources::resetStroke()
{
    ASSERT(m_fillStrokeData);
    ASSERT(m_fillStrokeData->stroke);
    m_fillStrokeData->stroke = 0;
}

bool SVGResources::setLinkedResource(RenderSVGResourceContainer* linkedResource)
{
    if (!linkedResource)
        return false;

    m_linkedResource = linkedResource;
    return true;
}

void SVGResources::resetLinkedResource()
{
    ASSERT(m_linkedResource);
    m_linkedResource = 0;
}

#if ENABLE(TREE_DEBUGGING)
void SVGResources::dump(const RenderObject* object)
{
    ASSERT(object);
    ASSERT(object->node());

    WTFLogAlways("-> this=%p, SVGResources(renderer=%p, node=%p)\n", this, object, object->node());
    WTFLogAlways(" | DOM Tree:\n");
    object->node()->showTreeForThis();

    WTFLogAlways("\n | List of resources:\n");
    if (m_clipperFilterMaskerData) {
        if (RenderSVGResourceClipper* clipper = m_clipperFilterMaskerData->clipper)
            WTFLogAlways(" |-> Clipper    : %p (node=%p)\n", clipper, &clipper->clipPathElement());
        if (RenderSVGResourceFilter* filter = m_clipperFilterMaskerData->filter)
            WTFLogAlways(" |-> Filter     : %p (node=%p)\n", filter, &filter->filterElement());
        if (RenderSVGResourceMasker* masker = m_clipperFilterMaskerData->masker)
            WTFLogAlways(" |-> Masker     : %p (node=%p)\n", masker, &masker->maskElement());
    }

    if (m_markerData) {
        if (RenderSVGResourceMarker* markerStart = m_markerData->markerStart)
            WTFLogAlways(" |-> MarkerStart: %p (node=%p)\n", markerStart, &markerStart->markerElement());
        if (RenderSVGResourceMarker* markerMid = m_markerData->markerMid)
            WTFLogAlways(" |-> MarkerMid  : %p (node=%p)\n", markerMid, &markerMid->markerElement());
        if (RenderSVGResourceMarker* markerEnd = m_markerData->markerEnd)
            WTFLogAlways(" |-> MarkerEnd  : %p (node=%p)\n", markerEnd, &markerEnd->markerElement());
    }

    if (m_fillStrokeData) {
        if (RenderSVGResourceContainer* fill = m_fillStrokeData->fill)
            WTFLogAlways(" |-> Fill       : %p (node=%p)\n", fill, &fill->element());
        if (RenderSVGResourceContainer* stroke = m_fillStrokeData->stroke)
            WTFLogAlways(" |-> Stroke     : %p (node=%p)\n", stroke, &stroke->element());
    }

    if (m_linkedResource)
        WTFLogAlways(" |-> xlink:href : %p (node=%p)\n", m_linkedResource, &m_linkedResource->element());
}
#endif

}
