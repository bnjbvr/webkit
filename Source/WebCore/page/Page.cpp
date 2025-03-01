/*
 * Copyright (C) 2006-2015  Apple Inc. All Rights Reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
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
#include "Page.h"

#include "ActivityStateChangeObserver.h"
#include "AlternativeTextClient.h"
#include "ApplicationCacheStorage.h"
#include "BackForwardClient.h"
#include "BackForwardController.h"
#include "CSSAnimationController.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "ConstantPropertyMap.h"
#include "ContextMenuClient.h"
#include "ContextMenuController.h"
#include "DOMRect.h"
#include "DOMRectList.h"
#include "DatabaseProvider.h"
#include "DiagnosticLoggingClient.h"
#include "DiagnosticLoggingKeys.h"
#include "DocumentLoader.h"
#include "DocumentMarkerController.h"
#include "DragController.h"
#include "Editor.h"
#include "EditorClient.h"
#include "EmptyClients.h"
#include "Event.h"
#include "EventNames.h"
#include "ExtensionStyleSheets.h"
#include "FileSystem.h"
#include "FocusController.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "HTMLElement.h"
#include "HistoryController.h"
#include "HistoryItem.h"
#include "InspectorController.h"
#include "InspectorInstrumentation.h"
#include "LibWebRTCProvider.h"
#include "Logging.h"
#include "LowPowerModeNotifier.h"
#include "MainFrame.h"
#include "MediaCanStartListener.h"
#include "Navigator.h"
#include "NetworkStateNotifier.h"
#include "PageCache.h"
#include "PageConfiguration.h"
#include "PageConsoleClient.h"
#include "PageDebuggable.h"
#include "PageGroup.h"
#include "PageOverlayController.h"
#include "PerformanceLoggingClient.h"
#include "PerformanceMonitor.h"
#include "PlatformMediaSessionManager.h"
#include "PlugInClient.h"
#include "PluginData.h"
#include "PluginInfoProvider.h"
#include "PluginViewBase.h"
#include "PointerLockController.h"
#include "ProgressTracker.h"
#include "PublicSuffix.h"
#include "RenderLayerCompositor.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "RenderWidget.h"
#include "ResourceUsageOverlay.h"
#include "RuntimeEnabledFeatures.h"
#include "SVGDocumentExtensions.h"
#include "SchemeRegistry.h"
#include "ScriptController.h"
#include "ScriptedAnimationController.h"
#include "ScrollingCoordinator.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "SocketProvider.h"
#include "StorageArea.h"
#include "StorageNamespace.h"
#include "StorageNamespaceProvider.h"
#include "StyleResolver.h"
#include "StyleScope.h"
#include "SubframeLoader.h"
#include "TextResourceDecoder.h"
#include "UserContentProvider.h"
#include "UserInputBridge.h"
#include "ValidationMessageClient.h"
#include "VisitedLinkState.h"
#include "VisitedLinkStore.h"
#include "VoidCallback.h"
#include "WebGLStateTracker.h"
#include "Widget.h"
#include <wtf/CurrentTime.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/Base64.h>
#include <wtf/text/StringHash.h>

#if ENABLE(WEB_REPLAY)
#include "ReplayController.h"
#include <replay/InputCursor.h>
#endif

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
#include "HTMLVideoElement.h"
#include "MediaPlaybackTarget.h"
#endif

#if ENABLE(MEDIA_SESSION)
#include "MediaSessionManager.h"
#endif

#if ENABLE(INDEXED_DATABASE)
#include "IDBConnectionToServer.h"
#include "InProcessIDBServer.h"
#endif

#if ENABLE(DATA_INTERACTION)
#include "SelectionRect.h"
#endif

namespace WebCore {

static HashSet<Page*>* allPages;
static unsigned nonUtilityPageCount { 0 };

static inline bool isUtilityPageChromeClient(ChromeClient& chromeClient)
{
    return chromeClient.isEmptyChromeClient() || chromeClient.isSVGImageChromeClient();
}

DEFINE_DEBUG_ONLY_GLOBAL(WTF::RefCountedLeakCounter, pageCounter, ("Page"));

void Page::forEachPage(const WTF::Function<void(Page&)>& function)
{
    if (!allPages)
        return;
    for (Page* page : *allPages)
        function(*page);
}

void Page::updateValidationBubbleStateIfNeeded()
{
    if (auto* client = validationMessageClient())
        client->updateValidationBubbleStateIfNeeded();
}

static void networkStateChanged(bool isOnLine)
{
    Vector<Ref<Frame>> frames;
    
    // Get all the frames of all the pages in all the page groups
    for (auto& page : *allPages) {
        for (Frame* frame = &page->mainFrame(); frame; frame = frame->tree().traverseNext())
            frames.append(*frame);
        InspectorInstrumentation::networkStateChanged(*page);
    }

    AtomicString eventName = isOnLine ? eventNames().onlineEvent : eventNames().offlineEvent;
    for (auto& frame : frames) {
        if (!frame->document())
            continue;
        frame->document()->dispatchWindowEvent(Event::create(eventName, false, false));
    }
}

static const ActivityState::Flags PageInitialActivityState = ActivityState::IsVisible | ActivityState::IsInWindow;

Page::Page(PageConfiguration&& pageConfiguration)
    : m_chrome(std::make_unique<Chrome>(*this, *pageConfiguration.chromeClient))
    , m_dragCaretController(std::make_unique<DragCaretController>())
#if ENABLE(DRAG_SUPPORT)
    , m_dragController(std::make_unique<DragController>(*this, *pageConfiguration.dragClient))
#endif
    , m_focusController(std::make_unique<FocusController>(*this, PageInitialActivityState))
#if ENABLE(CONTEXT_MENUS)
    , m_contextMenuController(std::make_unique<ContextMenuController>(*this, *pageConfiguration.contextMenuClient))
#endif
    , m_userInputBridge(std::make_unique<UserInputBridge>(*this))
#if ENABLE(WEB_REPLAY)
    , m_replayController(std::make_unique<ReplayController>(*this))
#endif
    , m_inspectorController(std::make_unique<InspectorController>(*this, pageConfiguration.inspectorClient))
#if ENABLE(POINTER_LOCK)
    , m_pointerLockController(std::make_unique<PointerLockController>(*this))
#endif
    , m_settings(Settings::create(this))
    , m_progress(std::make_unique<ProgressTracker>(*pageConfiguration.progressTrackerClient))
    , m_backForwardController(std::make_unique<BackForwardController>(*this, *WTFMove(pageConfiguration.backForwardClient)))
    , m_mainFrame(MainFrame::create(*this, pageConfiguration))
    , m_editorClient(WTFMove(pageConfiguration.editorClient))
    , m_plugInClient(pageConfiguration.plugInClient)
    , m_validationMessageClient(WTFMove(pageConfiguration.validationMessageClient))
    , m_diagnosticLoggingClient(WTFMove(pageConfiguration.diagnosticLoggingClient))
    , m_performanceLoggingClient(WTFMove(pageConfiguration.performanceLoggingClient))
    , m_webGLStateTracker(WTFMove(pageConfiguration.webGLStateTracker))
    , m_libWebRTCProvider(WTFMove(pageConfiguration.libWebRTCProvider))
    , m_openedByDOM(false)
    , m_tabKeyCyclesThroughElements(true)
    , m_defersLoading(false)
    , m_defersLoadingCallCount(0)
    , m_inLowQualityInterpolationMode(false)
    , m_areMemoryCacheClientCallsEnabled(true)
    , m_mediaVolume(1)
    , m_pageScaleFactor(1)
    , m_zoomedOutPageScaleFactor(0)
    , m_topContentInset(0)
#if ENABLE(TEXT_AUTOSIZING)
    , m_textAutosizingWidth(0)
#endif
    , m_suppressScrollbarAnimations(false)
    , m_verticalScrollElasticity(ScrollElasticityAllowed)
    , m_horizontalScrollElasticity(ScrollElasticityAllowed)
    , m_didLoadUserStyleSheet(false)
    , m_userStyleSheetModificationTime(0)
    , m_group(nullptr)
    , m_debugger(nullptr)
    , m_canStartMedia(true)
#if ENABLE(VIEW_MODE_CSS_MEDIA)
    , m_viewMode(ViewModeWindowed)
#endif // ENABLE(VIEW_MODE_CSS_MEDIA)
    , m_domTimerAlignmentInterval(DOMTimer::defaultAlignmentInterval())
    , m_domTimerAlignmentIntervalIncreaseTimer(*this, &Page::domTimerAlignmentIntervalIncreaseTimerFired)
    , m_isEditable(false)
    , m_isPrerender(false)
    , m_activityState(PageInitialActivityState)
    , m_requestedLayoutMilestones(0)
    , m_headerHeight(0)
    , m_footerHeight(0)
    , m_isCountingRelevantRepaintedObjects(false)
#ifndef NDEBUG
    , m_isPainting(false)
#endif
    , m_alternativeTextClient(pageConfiguration.alternativeTextClient)
    , m_scriptedAnimationsSuspended(false)
    , m_consoleClient(std::make_unique<PageConsoleClient>(*this))
#if ENABLE(REMOTE_INSPECTOR)
    , m_inspectorDebuggable(std::make_unique<PageDebuggable>(*this))
#endif
    , m_lastSpatialNavigationCandidatesCount(0) // NOTE: Only called from Internals for Spatial Navigation testing.
    , m_forbidPromptsDepth(0)
    , m_socketProvider(WTFMove(pageConfiguration.socketProvider))
    , m_applicationCacheStorage(*WTFMove(pageConfiguration.applicationCacheStorage))
    , m_databaseProvider(*WTFMove(pageConfiguration.databaseProvider))
    , m_pluginInfoProvider(*WTFMove(pageConfiguration.pluginInfoProvider))
    , m_storageNamespaceProvider(*WTFMove(pageConfiguration.storageNamespaceProvider))
    , m_userContentProvider(*WTFMove(pageConfiguration.userContentProvider))
    , m_visitedLinkStore(*WTFMove(pageConfiguration.visitedLinkStore))
    , m_sessionID(SessionID::defaultSessionID())
    , m_isClosing(false)
    , m_isUtilityPage(isUtilityPageChromeClient(chrome().client()))
    , m_performanceMonitor(isUtilityPage() ? nullptr : std::make_unique<PerformanceMonitor>(*this))
    , m_lowPowerModeNotifier(std::make_unique<LowPowerModeNotifier>([this](bool isLowPowerModeEnabled) { handleLowModePowerChange(isLowPowerModeEnabled); }))
{
    updateTimerThrottlingState();

    m_pluginInfoProvider->addPage(*this);
    m_storageNamespaceProvider->addPage(*this);
    m_userContentProvider->addPage(*this);
    m_visitedLinkStore->addPage(*this);

    if (!allPages) {
        allPages = new HashSet<Page*>;
        
        networkStateNotifier().addNetworkStateChangeListener(WTF::Function<void (bool)> { networkStateChanged });
    }

    ASSERT(!allPages->contains(this));
    allPages->add(this);
    if (!isUtilityPage()) {
        ++nonUtilityPageCount;
        MemoryPressureHandler::setPageCount(nonUtilityPageCount);
    }

#ifndef NDEBUG
    pageCounter.increment();
#endif

#if ENABLE(REMOTE_INSPECTOR)
    m_inspectorDebuggable->init();
#endif

#if PLATFORM(COCOA)
    platformInitialize();
#endif
}

Page::~Page()
{
    ASSERT(!m_nestedRunLoopCount);
    ASSERT(!m_unnestCallback);

    m_validationMessageClient = nullptr;
    m_diagnosticLoggingClient = nullptr;
    m_performanceLoggingClient = nullptr;
    m_mainFrame->setView(nullptr);
    setGroupName(String());
    allPages->remove(this);
    if (!isUtilityPage()) {
        --nonUtilityPageCount;
        MemoryPressureHandler::setPageCount(nonUtilityPageCount);
    }
    
    m_settings->pageDestroyed();

    m_inspectorController->inspectedPageDestroyed();

    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        frame->willDetachPage();
        frame->detachFromPage();
    }

    if (m_plugInClient)
        m_plugInClient->pageDestroyed();
    if (m_alternativeTextClient)
        m_alternativeTextClient->pageDestroyed();

    if (m_scrollingCoordinator)
        m_scrollingCoordinator->pageDestroyed();

    backForward().close();
    PageCache::singleton().removeAllItemsForPage(*this);

#ifndef NDEBUG
    pageCounter.decrement();
#endif

    m_pluginInfoProvider->removePage(*this);
    m_storageNamespaceProvider->removePage(*this);
    m_userContentProvider->removePage(*this);
    m_visitedLinkStore->removePage(*this);
}

void Page::clearPreviousItemFromAllPages(HistoryItem* item)
{
    if (!allPages)
        return;

    for (auto& page : *allPages) {
        HistoryController& controller = page->mainFrame().loader().history();
        if (item == controller.previousItem()) {
            controller.clearPreviousItem();
            return;
        }
    }
}

uint64_t Page::renderTreeSize() const
{
    uint64_t total = 0;
    for (const Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document() || !frame->document()->renderView())
            continue;
        total += frame->document()->renderView()->rendererCount();
    }
    return total;
}

ViewportArguments Page::viewportArguments() const
{
    return mainFrame().document() ? mainFrame().document()->viewportArguments() : ViewportArguments();
}

ScrollingCoordinator* Page::scrollingCoordinator()
{
    if (!m_scrollingCoordinator && m_settings->scrollingCoordinatorEnabled()) {
        m_scrollingCoordinator = chrome().client().createScrollingCoordinator(*this);
        if (!m_scrollingCoordinator)
            m_scrollingCoordinator = ScrollingCoordinator::create(this);
    }

    return m_scrollingCoordinator.get();
}

String Page::scrollingStateTreeAsText()
{
    if (Document* document = m_mainFrame->document())
        document->updateLayout();

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->scrollingStateTreeAsText();

    return String();
}

String Page::synchronousScrollingReasonsAsText()
{
    if (Document* document = m_mainFrame->document())
        document->updateLayout();

    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        return scrollingCoordinator->synchronousScrollingReasonsAsText();

    return String();
}

Ref<DOMRectList> Page::nonFastScrollableRects()
{
    if (Document* document = m_mainFrame->document())
        document->updateLayout();

    Vector<IntRect> rects;
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        const EventTrackingRegions& eventTrackingRegions = scrollingCoordinator->absoluteEventTrackingRegions();
        for (const auto& synchronousEventRegion : eventTrackingRegions.eventSpecificSynchronousDispatchRegions)
            rects.appendVector(synchronousEventRegion.value.rects());
    }

    Vector<FloatQuad> quads(rects.size());
    for (size_t i = 0; i < rects.size(); ++i)
        quads[i] = FloatRect(rects[i]);

    return DOMRectList::create(quads);
}

Ref<DOMRectList> Page::touchEventRectsForEvent(const String& eventName)
{
    if (Document* document = m_mainFrame->document()) {
        document->updateLayout();
#if ENABLE(IOS_TOUCH_EVENTS)
        document->updateTouchEventRegions();
#endif
    }

    Vector<IntRect> rects;
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator()) {
        const EventTrackingRegions& eventTrackingRegions = scrollingCoordinator->absoluteEventTrackingRegions();
        const auto& region = eventTrackingRegions.eventSpecificSynchronousDispatchRegions.get(eventName);
        rects.appendVector(region.rects());
    }

    Vector<FloatQuad> quads(rects.size());
    for (size_t i = 0; i < rects.size(); ++i)
        quads[i] = FloatRect(rects[i]);

    return DOMRectList::create(quads);
}

Ref<DOMRectList> Page::passiveTouchEventListenerRects()
{
    if (Document* document = m_mainFrame->document()) {
        document->updateLayout();
#if ENABLE(IOS_TOUCH_EVENTS)
        document->updateTouchEventRegions();
#endif  
    }

    Vector<IntRect> rects;
    if (ScrollingCoordinator* scrollingCoordinator = this->scrollingCoordinator())
        rects.appendVector(scrollingCoordinator->absoluteEventTrackingRegions().asynchronousDispatchRegion.rects());

    Vector<FloatQuad> quads(rects.size());
    for (size_t i = 0; i < rects.size(); ++i)
        quads[i] = FloatRect(rects[i]);

    return DOMRectList::create(quads);
}

#if ENABLE(VIEW_MODE_CSS_MEDIA)
struct ViewModeInfo {
    const char* name;
    Page::ViewMode type;
};
static const int viewModeMapSize = 5;
static const ViewModeInfo viewModeMap[viewModeMapSize] = {
    {"windowed", Page::ViewModeWindowed},
    {"floating", Page::ViewModeFloating},
    {"fullscreen", Page::ViewModeFullscreen},
    {"maximized", Page::ViewModeMaximized},
    {"minimized", Page::ViewModeMinimized}
};

Page::ViewMode Page::stringToViewMode(const String& text)
{
    for (auto& mode : viewModeMap) {
        if (text == mode.name)
            return mode.type;
    }
    return Page::ViewModeInvalid;
}

void Page::setViewMode(ViewMode viewMode)
{
    if (viewMode == m_viewMode || viewMode == ViewModeInvalid)
        return;

    m_viewMode = viewMode;


    if (m_mainFrame->view())
        m_mainFrame->view()->forceLayout();

    if (m_mainFrame->document())
        m_mainFrame->document()->styleScope().didChangeStyleSheetEnvironment();
}
#endif // ENABLE(VIEW_MODE_CSS_MEDIA)

bool Page::openedByDOM() const
{
    return m_openedByDOM;
}

void Page::setOpenedByDOM()
{
    m_openedByDOM = true;
}

void Page::goToItem(HistoryItem& item, FrameLoadType type)
{
    // stopAllLoaders may end up running onload handlers, which could cause further history traversals that may lead to the passed in HistoryItem
    // being deref()-ed. Make sure we can still use it with HistoryController::goToItem later.
    Ref<HistoryItem> protector(item);

    if (m_mainFrame->loader().history().shouldStopLoadingForHistoryItem(item))
        m_mainFrame->loader().stopAllLoaders();

    m_mainFrame->loader().history().goToItem(item, type);
}

void Page::setGroupName(const String& name)
{
    if (m_group && !m_group->name().isEmpty()) {
        ASSERT(m_group != m_singlePageGroup.get());
        ASSERT(!m_singlePageGroup);
        m_group->removePage(*this);
    }

    if (name.isEmpty())
        m_group = m_singlePageGroup.get();
    else {
        m_singlePageGroup = nullptr;
        m_group = PageGroup::pageGroup(name);
        m_group->addPage(*this);
    }
}

const String& Page::groupName() const
{
    return m_group ? m_group->name() : nullAtom.string();
}

void Page::initGroup()
{
    ASSERT(!m_singlePageGroup);
    ASSERT(!m_group);
    m_singlePageGroup = std::make_unique<PageGroup>(*this);
    m_group = m_singlePageGroup.get();
}

void Page::updateStyleForAllPagesAfterGlobalChangeInEnvironment()
{
    if (!allPages)
        return;
    for (auto& page : *allPages) {
        for (Frame* frame = &page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
            // If a change in the global environment has occurred, we need to
            // make sure all the properties a recomputed, therefore we invalidate
            // the properties cache.
            if (!frame->document())
                continue;
            if (StyleResolver* styleResolver = frame->document()->styleScope().resolverIfExists())
                styleResolver->invalidateMatchedPropertiesCache();
            frame->document()->scheduleForcedStyleRecalc();
        }
    }
}

void Page::setNeedsRecalcStyleInAllFrames()
{
    // FIXME: Figure out what this function is actually trying to add in different call sites.
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (Document* document = frame->document())
            document->styleScope().didChangeStyleSheetEnvironment();
    }
}

void Page::refreshPlugins(bool reload)
{
    if (!allPages)
        return;

    HashSet<PluginInfoProvider*> pluginInfoProviders;

    for (auto& page : *allPages)
        pluginInfoProviders.add(&page->pluginInfoProvider());

    for (auto& pluginInfoProvider : pluginInfoProviders)
        pluginInfoProvider->refresh(reload);
}

PluginData& Page::pluginData()
{
    if (!m_pluginData)
        m_pluginData = PluginData::create(*this);
    return *m_pluginData;
}

void Page::clearPluginData()
{
    m_pluginData = nullptr;
}

bool Page::showAllPlugins() const
{
    if (m_showAllPlugins)
        return true;

    if (Document* document = mainFrame().document())
        return document->securityOrigin().isLocal();

    return false;
}

inline std::optional<std::pair<MediaCanStartListener&, Document&>>  Page::takeAnyMediaCanStartListener()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        if (MediaCanStartListener* listener = frame->document()->takeAnyMediaCanStartListener())
            return { { *listener, *frame->document() } };
    }
    return std::nullopt;
}

void Page::setCanStartMedia(bool canStartMedia)
{
    if (m_canStartMedia == canStartMedia)
        return;

    m_canStartMedia = canStartMedia;

    while (m_canStartMedia) {
        auto listener = takeAnyMediaCanStartListener();
        if (!listener)
            break;
        listener->first.mediaCanStart(listener->second);
    }
}

static Frame* incrementFrame(Frame* curr, bool forward, CanWrap canWrap, DidWrap* didWrap = nullptr)
{
    return forward
        ? curr->tree().traverseNext(canWrap, didWrap)
        : curr->tree().traversePrevious(canWrap, didWrap);
}

bool Page::findString(const String& target, FindOptions options, DidWrap* didWrap)
{
    if (target.isEmpty())
        return false;

    CanWrap canWrap = options & WrapAround ? CanWrap::Yes : CanWrap::No;
    Frame* frame = &focusController().focusedOrMainFrame();
    Frame* startFrame = frame;
    do {
        if (frame->editor().findString(target, (options & ~WrapAround) | StartInSelection)) {
            if (frame != startFrame)
                startFrame->selection().clear();
            focusController().setFocusedFrame(frame);
            return true;
        }
        frame = incrementFrame(frame, !(options & Backwards), canWrap, didWrap);
    } while (frame && frame != startFrame);

    // Search contents of startFrame, on the other side of the selection that we did earlier.
    // We cheat a bit and just research with wrap on
    if (canWrap == CanWrap::Yes && !startFrame->selection().isNone()) {
        if (didWrap)
            *didWrap = DidWrap::Yes;
        bool found = startFrame->editor().findString(target, options | WrapAround | StartInSelection);
        focusController().setFocusedFrame(frame);
        return found;
    }

    return false;
}

void Page::findStringMatchingRanges(const String& target, FindOptions options, int limit, Vector<RefPtr<Range>>& matchRanges, int& indexForSelection)
{
    indexForSelection = 0;

    Frame* frame = &mainFrame();
    Frame* frameWithSelection = nullptr;
    do {
        frame->editor().countMatchesForText(target, 0, options, limit ? (limit - matchRanges.size()) : 0, true, &matchRanges);
        if (frame->selection().isRange())
            frameWithSelection = frame;
        frame = incrementFrame(frame, true, CanWrap::No);
    } while (frame);

    if (matchRanges.isEmpty())
        return;

    if (frameWithSelection) {
        indexForSelection = NoMatchAfterUserSelection;
        RefPtr<Range> selectedRange = frameWithSelection->selection().selection().firstRange();
        if (options & Backwards) {
            for (size_t i = matchRanges.size(); i > 0; --i) {
                auto result = selectedRange->compareBoundaryPoints(Range::END_TO_START, *matchRanges[i - 1]);
                if (!result.hasException() && result.releaseReturnValue() > 0) {
                    indexForSelection = i - 1;
                    break;
                }
            }
        } else {
            for (size_t i = 0, size = matchRanges.size(); i < size; ++i) {
                auto result = selectedRange->compareBoundaryPoints(Range::START_TO_END, *matchRanges[i]);
                if (!result.hasException() && result.releaseReturnValue() < 0) {
                    indexForSelection = i;
                    break;
                }
            }
        }
    } else {
        if (options & Backwards)
            indexForSelection = matchRanges.size() - 1;
        else
            indexForSelection = 0;
    }
}

RefPtr<Range> Page::rangeOfString(const String& target, Range* referenceRange, FindOptions options)
{
    if (target.isEmpty())
        return nullptr;

    if (referenceRange && referenceRange->ownerDocument().page() != this)
        return nullptr;

    CanWrap canWrap = options & WrapAround ? CanWrap::Yes : CanWrap::No;
    Frame* frame = referenceRange ? referenceRange->ownerDocument().frame() : &mainFrame();
    Frame* startFrame = frame;
    do {
        if (RefPtr<Range> resultRange = frame->editor().rangeOfString(target, frame == startFrame ? referenceRange : 0, options & ~WrapAround))
            return resultRange;

        frame = incrementFrame(frame, !(options & Backwards), canWrap);
    } while (frame && frame != startFrame);

    // Search contents of startFrame, on the other side of the reference range that we did earlier.
    // We cheat a bit and just search again with wrap on.
    if (canWrap == CanWrap::Yes && referenceRange) {
        if (RefPtr<Range> resultRange = startFrame->editor().rangeOfString(target, referenceRange, options | WrapAround | StartInSelection))
            return resultRange;
    }

    return nullptr;
}

unsigned Page::findMatchesForText(const String& target, FindOptions options, unsigned maxMatchCount, ShouldHighlightMatches shouldHighlightMatches, ShouldMarkMatches shouldMarkMatches)
{
    if (target.isEmpty())
        return 0;

    unsigned matchCount = 0;

    Frame* frame = &mainFrame();
    do {
        if (shouldMarkMatches == MarkMatches)
            frame->editor().setMarkedTextMatchesAreHighlighted(shouldHighlightMatches == HighlightMatches);
        matchCount += frame->editor().countMatchesForText(target, 0, options, maxMatchCount ? (maxMatchCount - matchCount) : 0, shouldMarkMatches == MarkMatches, 0);
        frame = incrementFrame(frame, true, CanWrap::No);
    } while (frame);

    return matchCount;
}

unsigned Page::markAllMatchesForText(const String& target, FindOptions options, bool shouldHighlight, unsigned maxMatchCount)
{
    return findMatchesForText(target, options, maxMatchCount, shouldHighlight ? HighlightMatches : DoNotHighlightMatches, MarkMatches);
}

unsigned Page::countFindMatches(const String& target, FindOptions options, unsigned maxMatchCount)
{
    return findMatchesForText(target, options, maxMatchCount, DoNotHighlightMatches, DoNotMarkMatches);
}

void Page::unmarkAllTextMatches()
{
    Frame* frame = &mainFrame();
    do {
        frame->document()->markers().removeMarkers(DocumentMarker::TextMatch);
        frame = incrementFrame(frame, true, CanWrap::No);
    } while (frame);
}

const VisibleSelection& Page::selection() const
{
    return focusController().focusedOrMainFrame().selection().selection();
}

void Page::setDefersLoading(bool defers)
{
    if (!m_settings->loadDeferringEnabled())
        return;

    if (m_settings->wantsBalancedSetDefersLoadingBehavior()) {
        ASSERT(defers || m_defersLoadingCallCount);
        if (defers && ++m_defersLoadingCallCount > 1)
            return;
        if (!defers && --m_defersLoadingCallCount)
            return;
    } else {
        ASSERT(!m_defersLoadingCallCount);
        if (defers == m_defersLoading)
            return;
    }

    m_defersLoading = defers;
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext())
        frame->loader().setDefersLoading(defers);
}

void Page::clearUndoRedoOperations()
{
    m_editorClient->clearUndoRedoOperations();
}

bool Page::inLowQualityImageInterpolationMode() const
{
    return m_inLowQualityInterpolationMode;
}

void Page::setInLowQualityImageInterpolationMode(bool mode)
{
    m_inLowQualityInterpolationMode = mode;
}

DiagnosticLoggingClient& Page::diagnosticLoggingClient() const
{
    if (!settings().diagnosticLoggingEnabled() || !m_diagnosticLoggingClient)
        return emptyDiagnosticLoggingClient();
    return *m_diagnosticLoggingClient;
}

void Page::setMediaVolume(float volume)
{
    if (volume < 0 || volume > 1)
        return;

    if (m_mediaVolume == volume)
        return;

    m_mediaVolume = volume;
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->mediaVolumeDidChange();
    }
}

void Page::setZoomedOutPageScaleFactor(float scale)
{
    if (m_zoomedOutPageScaleFactor == scale)
        return;
    m_zoomedOutPageScaleFactor = scale;

    mainFrame().deviceOrPageScaleFactorChanged();
}

void Page::setPageScaleFactor(float scale, const IntPoint& origin, bool inStableState)
{
    Document* document = mainFrame().document();
    FrameView* view = document->view();

    if (scale == m_pageScaleFactor) {
        if (view && view->scrollPosition() != origin) {
            if (!m_settings->delegatesPageScaling())
                document->updateLayoutIgnorePendingStylesheets();

            if (!view->delegatesScrolling())
                view->setScrollPosition(origin);
#if USE(COORDINATED_GRAPHICS)
            else
                view->requestScrollPositionUpdate(origin);
#endif
        }
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
        if (inStableState) {
            for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
                if (!frame->document())
                    continue;
                frame->document()->pageScaleFactorChangedAndStable();
            }
        }
#endif
        return;
    }

    m_pageScaleFactor = scale;

    if (!m_settings->delegatesPageScaling()) {
        if (document->renderView())
            document->renderView()->setNeedsLayout();

        document->resolveStyle(Document::ResolveStyleType::Rebuild);

        // Transform change on RenderView doesn't trigger repaint on non-composited contents.
        mainFrame().view()->invalidateRect(IntRect(LayoutRect::infiniteRect()));
    }

    mainFrame().deviceOrPageScaleFactorChanged();

    if (view && view->fixedElementsLayoutRelativeToFrame())
        view->setViewportConstrainedObjectsNeedLayout();

    if (view && view->scrollPosition() != origin) {
        if (!m_settings->delegatesPageScaling() && document->renderView() && document->renderView()->needsLayout() && view->didFirstLayout())
            view->layout();

        if (!view->delegatesScrolling())
            view->setScrollPosition(origin);
#if USE(COORDINATED_GRAPHICS)
        else
            view->requestScrollPositionUpdate(origin);
#endif
    }

#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    if (inStableState) {
        for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
            if (!frame->document())
                continue;
            frame->document()->pageScaleFactorChangedAndStable();
        }
    }
#else
    UNUSED_PARAM(inStableState);
#endif
}

void Page::setViewScaleFactor(float scale)
{
    if (m_viewScaleFactor == scale)
        return;

    m_viewScaleFactor = scale;
    PageCache::singleton().markPagesForDeviceOrPageScaleChanged(*this);
}

void Page::setDeviceScaleFactor(float scaleFactor)
{
    ASSERT(scaleFactor > 0);
    if (scaleFactor <= 0)
        return;
    
    if (m_deviceScaleFactor == scaleFactor)
        return;

    m_deviceScaleFactor = scaleFactor;
    setNeedsRecalcStyleInAllFrames();

    mainFrame().deviceOrPageScaleFactorChanged();
    PageCache::singleton().markPagesForDeviceOrPageScaleChanged(*this);

    GraphicsContext::updateDocumentMarkerResources();

    mainFrame().pageOverlayController().didChangeDeviceScaleFactor();
}

void Page::setUserInterfaceLayoutDirection(UserInterfaceLayoutDirection userInterfaceLayoutDirection)
{
    if (m_userInterfaceLayoutDirection == userInterfaceLayoutDirection)
        return;

    m_userInterfaceLayoutDirection = userInterfaceLayoutDirection;
#if ENABLE(MEDIA_CONTROLS_SCRIPT)
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->userInterfaceLayoutDirectionChanged();
    }
#endif
}

void Page::didStartProvisionalLoad()
{
    if (m_performanceMonitor)
        m_performanceMonitor->didStartProvisionalLoad();
}

void Page::didFinishLoad()
{
    resetRelevantPaintedObjectCounter();

    if (m_performanceMonitor)
        m_performanceMonitor->didFinishLoad();
}

bool Page::isOnlyNonUtilityPage() const
{
    return !isUtilityPage() && nonUtilityPageCount == 1;
}

bool Page::isLowPowerModeEnabled() const
{
    if (m_lowPowerModeEnabledOverrideForTesting)
        return m_lowPowerModeEnabledOverrideForTesting.value();

    return m_lowPowerModeNotifier->isLowPowerModeEnabled();
}

void Page::setLowPowerModeEnabledOverrideForTesting(std::optional<bool> isEnabled)
{
    m_lowPowerModeEnabledOverrideForTesting = isEnabled;
    handleLowModePowerChange(m_lowPowerModeEnabledOverrideForTesting.value_or(false));
}

void Page::setTopContentInset(float contentInset)
{
    if (m_topContentInset == contentInset)
        return;
    
    m_topContentInset = contentInset;
    
    if (FrameView* view = mainFrame().view())
        view->topContentInsetDidChange(m_topContentInset);
}

void Page::setShouldSuppressScrollbarAnimations(bool suppressAnimations)
{
    if (suppressAnimations == m_suppressScrollbarAnimations)
        return;

    lockAllOverlayScrollbarsToHidden(suppressAnimations);
    m_suppressScrollbarAnimations = suppressAnimations;
}

void Page::lockAllOverlayScrollbarsToHidden(bool lockOverlayScrollbars)
{
    FrameView* view = mainFrame().view();
    if (!view)
        return;

    view->lockOverlayScrollbarStateToHidden(lockOverlayScrollbars);
    
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        FrameView* frameView = frame->view();
        if (!frameView)
            continue;

        const HashSet<ScrollableArea*>* scrollableAreas = frameView->scrollableAreas();
        if (!scrollableAreas)
            continue;

        for (auto& scrollableArea : *scrollableAreas)
            scrollableArea->lockOverlayScrollbarStateToHidden(lockOverlayScrollbars);
    }
}
    
void Page::setVerticalScrollElasticity(ScrollElasticity elasticity)
{
    if (m_verticalScrollElasticity == elasticity)
        return;
    
    m_verticalScrollElasticity = elasticity;
    
    if (FrameView* view = mainFrame().view())
        view->setVerticalScrollElasticity(elasticity);
}
    
void Page::setHorizontalScrollElasticity(ScrollElasticity elasticity)
{
    if (m_horizontalScrollElasticity == elasticity)
        return;
    
    m_horizontalScrollElasticity = elasticity;
    
    if (FrameView* view = mainFrame().view())
        view->setHorizontalScrollElasticity(elasticity);
}

void Page::setPagination(const Pagination& pagination)
{
    if (m_pagination == pagination)
        return;

    m_pagination = pagination;

    setNeedsRecalcStyleInAllFrames();
}

void Page::setPaginationLineGridEnabled(bool enabled)
{
    if (m_paginationLineGridEnabled == enabled)
        return;
    
    m_paginationLineGridEnabled = enabled;
    
    setNeedsRecalcStyleInAllFrames();
}

unsigned Page::pageCount() const
{
    if (m_pagination.mode == Pagination::Unpaginated)
        return 0;

    if (Document* document = mainFrame().document())
        document->updateLayoutIgnorePendingStylesheets();

    RenderView* contentRenderer = mainFrame().contentRenderer();
    return contentRenderer ? contentRenderer->pageCount() : 0;
}

void Page::setIsInWindow(bool isInWindow)
{
    setActivityState(isInWindow ? m_activityState | ActivityState::IsInWindow : m_activityState & ~ActivityState::IsInWindow);
}

void Page::setIsInWindowInternal(bool isInWindow)
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (FrameView* frameView = frame->view())
            frameView->setIsInWindow(isInWindow);
    }

    if (isInWindow)
        resumeAnimatingImages();
}

void Page::addActivityStateChangeObserver(ActivityStateChangeObserver& observer)
{
    m_activityStateChangeObservers.add(&observer);
}

void Page::removeActivityStateChangeObserver(ActivityStateChangeObserver& observer)
{
    m_activityStateChangeObservers.remove(&observer);
}

void Page::suspendScriptedAnimations()
{
    m_scriptedAnimationsSuspended = true;
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (frame->document())
            frame->document()->suspendScriptedAnimationControllerCallbacks();
    }
}

void Page::resumeScriptedAnimations()
{
    m_scriptedAnimationsSuspended = false;
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (frame->document())
            frame->document()->resumeScriptedAnimationControllerCallbacks();
    }
}

enum class ThrottlingReasonOperation { Add, Remove };
static void updateScriptedAnimationsThrottlingReason(Page& page, ThrottlingReasonOperation operation, ScriptedAnimationController::ThrottlingReason reason)
{
    for (Frame* frame = &page.mainFrame(); frame; frame = frame->tree().traverseNext()) {
        auto* document = frame->document();
        if (!document)
            continue;
        auto* scriptedAnimationController = document->scriptedAnimationController();
        if (!scriptedAnimationController)
            continue;

        if (operation == ThrottlingReasonOperation::Add)
            scriptedAnimationController->addThrottlingReason(reason);
        else
            scriptedAnimationController->removeThrottlingReason(reason);
    }
}

void Page::setIsVisuallyIdleInternal(bool isVisuallyIdle)
{
    updateScriptedAnimationsThrottlingReason(*this, isVisuallyIdle ? ThrottlingReasonOperation::Add : ThrottlingReasonOperation::Remove, ScriptedAnimationController::ThrottlingReason::VisuallyIdle);
}

void Page::handleLowModePowerChange(bool isLowPowerModeEnabled)
{
    updateScriptedAnimationsThrottlingReason(*this, isLowPowerModeEnabled ? ThrottlingReasonOperation::Add : ThrottlingReasonOperation::Remove, ScriptedAnimationController::ThrottlingReason::LowPowerMode);
    mainFrame().animation().updateThrottlingState();
    updateDOMTimerAlignmentInterval();
}

void Page::userStyleSheetLocationChanged()
{
    // FIXME: Eventually we will move to a model of just being handed the sheet
    // text instead of loading the URL ourselves.
    URL url = m_settings->userStyleSheetLocation();
    
    // Allow any local file URL scheme to be loaded.
    if (SchemeRegistry::shouldTreatURLSchemeAsLocal(url.protocol().toStringWithoutCopying()))
        m_userStyleSheetPath = url.fileSystemPath();
    else
        m_userStyleSheetPath = String();

    m_didLoadUserStyleSheet = false;
    m_userStyleSheet = String();
    m_userStyleSheetModificationTime = 0;

    // Data URLs with base64-encoded UTF-8 style sheets are common. We can process them
    // synchronously and avoid using a loader. 
    if (url.protocolIsData() && url.string().startsWith("data:text/css;charset=utf-8;base64,")) {
        m_didLoadUserStyleSheet = true;

        Vector<char> styleSheetAsUTF8;
        if (base64Decode(decodeURLEscapeSequences(url.string().substring(35)), styleSheetAsUTF8, Base64IgnoreSpacesAndNewLines))
            m_userStyleSheet = String::fromUTF8(styleSheetAsUTF8.data(), styleSheetAsUTF8.size());
    }

    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (frame->document())
            frame->document()->extensionStyleSheets().updatePageUserSheet();
    }
}

const String& Page::userStyleSheet() const
{
    if (m_userStyleSheetPath.isEmpty())
        return m_userStyleSheet;

    time_t modTime;
    if (!getFileModificationTime(m_userStyleSheetPath, modTime)) {
        // The stylesheet either doesn't exist, was just deleted, or is
        // otherwise unreadable. If we've read the stylesheet before, we should
        // throw away that data now as it no longer represents what's on disk.
        m_userStyleSheet = String();
        return m_userStyleSheet;
    }

    // If the stylesheet hasn't changed since the last time we read it, we can
    // just return the old data.
    if (m_didLoadUserStyleSheet && modTime <= m_userStyleSheetModificationTime)
        return m_userStyleSheet;

    m_didLoadUserStyleSheet = true;
    m_userStyleSheet = String();
    m_userStyleSheetModificationTime = modTime;

    // FIXME: It would be better to load this asynchronously to avoid blocking
    // the process, but we will first need to create an asynchronous loading
    // mechanism that is not tied to a particular Frame. We will also have to
    // determine what our behavior should be before the stylesheet is loaded
    // and what should happen when it finishes loading, especially with respect
    // to when the load event fires, when Document::close is called, and when
    // layout/paint are allowed to happen.
    RefPtr<SharedBuffer> data = SharedBuffer::createWithContentsOfFile(m_userStyleSheetPath);
    if (!data)
        return m_userStyleSheet;

    m_userStyleSheet = TextResourceDecoder::create("text/css")->decodeAndFlush(data->data(), data->size());

    return m_userStyleSheet;
}

void Page::invalidateStylesForAllLinks()
{
    for (Frame* frame = &m_mainFrame.get(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->visitedLinkState().invalidateStyleForAllLinks();
    }
}

void Page::invalidateStylesForLink(LinkHash linkHash)
{
    for (Frame* frame = &m_mainFrame.get(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->visitedLinkState().invalidateStyleForLink(linkHash);
    }
}

void Page::invalidateInjectedStyleSheetCacheInAllFrames()
{
    for (Frame* frame = &m_mainFrame.get(); frame; frame = frame->tree().traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;
        document->extensionStyleSheets().invalidateInjectedStyleSheetCache();
    }
}

void Page::setDebugger(JSC::Debugger* debugger)
{
    if (m_debugger == debugger)
        return;

    m_debugger = debugger;

    for (Frame* frame = &m_mainFrame.get(); frame; frame = frame->tree().traverseNext())
        frame->script().attachDebugger(m_debugger);
}

StorageNamespace* Page::sessionStorage(bool optionalCreate)
{
    if (!m_sessionStorage && optionalCreate)
        m_sessionStorage = m_storageNamespaceProvider->createSessionStorageNamespace(*this, m_settings->sessionStorageQuota());

    return m_sessionStorage.get();
}

void Page::setSessionStorage(RefPtr<StorageNamespace>&& newStorage)
{
    m_sessionStorage = WTFMove(newStorage);
}

StorageNamespace* Page::ephemeralLocalStorage(bool optionalCreate)
{
    if (!m_ephemeralLocalStorage && optionalCreate)
        m_ephemeralLocalStorage = m_storageNamespaceProvider->createEphemeralLocalStorageNamespace(*this, m_settings->sessionStorageQuota());

    return m_ephemeralLocalStorage.get();
}

void Page::setEphemeralLocalStorage(RefPtr<StorageNamespace>&& newStorage)
{
    m_ephemeralLocalStorage = WTFMove(newStorage);
}

bool Page::hasCustomHTMLTokenizerTimeDelay() const
{
    return m_settings->maxParseDuration() != -1;
}

double Page::customHTMLTokenizerTimeDelay() const
{
    ASSERT(m_settings->maxParseDuration() != -1);
    return m_settings->maxParseDuration();
}

void Page::setMemoryCacheClientCallsEnabled(bool enabled)
{
    if (m_areMemoryCacheClientCallsEnabled == enabled)
        return;

    m_areMemoryCacheClientCallsEnabled = enabled;
    if (!enabled)
        return;

    for (RefPtr<Frame> frame = &mainFrame(); frame; frame = frame->tree().traverseNext())
        frame->loader().tellClientAboutPastMemoryCacheLoads();
}

void Page::hiddenPageDOMTimerThrottlingStateChanged()
{
    // Disable & reengage to ensure state is updated.
    setTimerThrottlingState(TimerThrottlingState::Disabled);
    updateTimerThrottlingState();
}

void Page::updateTimerThrottlingState()
{
    // Timer throttling disabled if page is visually active, or disabled by setting.
    if (!m_settings->hiddenPageDOMTimerThrottlingEnabled() || !(m_activityState & ActivityState::IsVisuallyIdle)) {
        setTimerThrottlingState(TimerThrottlingState::Disabled);
        return;
    }

    // If the page is visible (but idle), there is any activity (loading, media playing, etc), or per setting,
    // we allow timer throttling, but not increasing timer throttling.
    if (!m_settings->hiddenPageDOMTimerThrottlingAutoIncreases()
        || m_activityState & (ActivityState::IsVisible | ActivityState::IsAudible | ActivityState::IsLoading | ActivityState::IsCapturingMedia)) {
        setTimerThrottlingState(TimerThrottlingState::Enabled);
        return;
    }

    // If we get here increasing timer throttling is enabled.
    setTimerThrottlingState(TimerThrottlingState::EnabledIncreasing);
}

void Page::setTimerThrottlingState(TimerThrottlingState state)
{
    if (state == m_timerThrottlingState)
        return;

    m_timerThrottlingState = state;
    m_timerThrottlingStateLastChangedTime = MonotonicTime::now();

    updateDOMTimerAlignmentInterval();

    // When throttling is disabled, release all throttled timers.
    if (state == TimerThrottlingState::Disabled) {
        for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
            if (auto* document = frame->document())
                document->didChangeTimerAlignmentInterval();
        }
    }
}

void Page::setDOMTimerAlignmentIntervalIncreaseLimit(Seconds limit)
{
    m_domTimerAlignmentIntervalIncreaseLimit = limit;

    // If (m_domTimerAlignmentIntervalIncreaseLimit < m_domTimerAlignmentInterval) then we need
    // to update m_domTimerAlignmentInterval, if greater then need to restart the increase timer.
    if (m_timerThrottlingState == TimerThrottlingState::EnabledIncreasing)
        updateDOMTimerAlignmentInterval();
}

void Page::updateDOMTimerAlignmentInterval()
{
    bool needsIncreaseTimer = false;

    switch (m_timerThrottlingState) {
    case TimerThrottlingState::Disabled:
        m_domTimerAlignmentInterval = isLowPowerModeEnabled() ? DOMTimer::defaultAlignmentIntervalInLowPowerMode() : DOMTimer::defaultAlignmentInterval();
        break;

    case TimerThrottlingState::Enabled:
        m_domTimerAlignmentInterval = DOMTimer::hiddenPageAlignmentInterval();
        break;

    case TimerThrottlingState::EnabledIncreasing:
        // For pages in prerender state maximum throttling kicks in immediately.
        if (m_isPrerender)
            m_domTimerAlignmentInterval = m_domTimerAlignmentIntervalIncreaseLimit;
        else {
            ASSERT(!!m_timerThrottlingStateLastChangedTime);
            m_domTimerAlignmentInterval = MonotonicTime::now() - m_timerThrottlingStateLastChangedTime;
            // If we're below the limit, set the timer. If above, clamp to limit.
            if (m_domTimerAlignmentInterval < m_domTimerAlignmentIntervalIncreaseLimit)
                needsIncreaseTimer = true;
            else
                m_domTimerAlignmentInterval = m_domTimerAlignmentIntervalIncreaseLimit;
        }
        // Alignment interval should not be less than DOMTimer::hiddenPageAlignmentInterval().
        m_domTimerAlignmentInterval = std::max(m_domTimerAlignmentInterval, DOMTimer::hiddenPageAlignmentInterval());
    }

    // If throttling is enabled, auto-increasing of throttling is enabled, and the auto-increase
    // limit has not yet been reached, and then arm the timer to consider an increase. Time to wait
    // between increases is equal to the current throttle time. Since alinment interval increases
    // exponentially, time between steps is exponential too.
    if (!needsIncreaseTimer)
        m_domTimerAlignmentIntervalIncreaseTimer.stop();
    else if (!m_domTimerAlignmentIntervalIncreaseTimer.isActive())
        m_domTimerAlignmentIntervalIncreaseTimer.startOneShot(m_domTimerAlignmentInterval);
}

void Page::domTimerAlignmentIntervalIncreaseTimerFired()
{
    ASSERT(m_settings->hiddenPageDOMTimerThrottlingAutoIncreases());
    ASSERT(m_timerThrottlingState == TimerThrottlingState::EnabledIncreasing);
    ASSERT(m_domTimerAlignmentInterval < m_domTimerAlignmentIntervalIncreaseLimit);
    
    // Alignment interval is increased to equal the time the page has been throttled, to a limit.
    updateDOMTimerAlignmentInterval();
}

void Page::dnsPrefetchingStateChanged()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->initDNSPrefetch();
    }
}

Vector<Ref<PluginViewBase>> Page::pluginViews()
{
    Vector<Ref<PluginViewBase>> views;
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        auto* view = frame->view();
        if (!view)
            break;
        for (auto& widget : view->children()) {
            if (is<PluginViewBase>(widget.get()))
                views.append(downcast<PluginViewBase>(widget.get()));
        }
    }
    return views;
}

void Page::storageBlockingStateChanged()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->storageBlockingStateDidChange();
    }

    // Collect the PluginViews in to a vector to ensure that action the plug-in takes
    // from below storageBlockingStateChanged does not affect their lifetime.
    for (auto& view : pluginViews())
        view->storageBlockingStateChanged();
}

void Page::enableLegacyPrivateBrowsing(bool privateBrowsingEnabled)
{
    // Don't allow changing the legacy private browsing state if we have set a session ID.
    ASSERT(m_sessionID == SessionID::defaultSessionID() || m_sessionID == SessionID::legacyPrivateSessionID());

    setSessionID(privateBrowsingEnabled ? SessionID::legacyPrivateSessionID() : SessionID::defaultSessionID());
}

void Page::updateIsPlayingMedia(uint64_t sourceElementID)
{
    MediaProducer::MediaStateFlags state = MediaProducer::IsNotPlaying;
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (Document* document = frame->document())
            state |= document->mediaState();
    }

    if (state == m_mediaState)
        return;

    m_mediaState = state;

    chrome().client().isPlayingMediaDidChange(state, sourceElementID);
}

void Page::setMuted(MediaProducer::MutedStateFlags muted)
{
    if (m_mutedState == muted)
        return;

    m_mutedState = muted;

    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->pageMutedStateDidChange();
    }
}

void Page::stopMediaCapture()
{
#if ENABLE(MEDIA_STREAM)
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;

        frame->document()->stopMediaCapture();
    }
#endif
}

#if ENABLE(MEDIA_SESSION)
void Page::handleMediaEvent(MediaEventType eventType)
{
    switch (eventType) {
    case MediaEventType::PlayPause:
        MediaSessionManager::singleton().togglePlayback();
        break;
    case MediaEventType::TrackNext:
        MediaSessionManager::singleton().skipToNextTrack();
        break;
    case MediaEventType::TrackPrevious:
        MediaSessionManager::singleton().skipToPreviousTrack();
        break;
    }
}

void Page::setVolumeOfMediaElement(double volume, uint64_t elementID)
{
    if (HTMLMediaElement* element = HTMLMediaElement::elementWithID(elementID))
        element->setVolume(volume, ASSERT_NO_EXCEPTION);
}
#endif

#if !ASSERT_DISABLED
void Page::checkSubframeCountConsistency() const
{
    ASSERT(m_subframeCount >= 0);

    int subframeCount = 0;
    for (const Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext())
        ++subframeCount;

    ASSERT(m_subframeCount + 1 == subframeCount);
}
#endif

void Page::resumeAnimatingImages()
{
    // Drawing models which cache painted content while out-of-window (WebKit2's composited drawing areas, etc.)
    // require that we repaint animated images to kickstart the animation loop.
    if (FrameView* view = mainFrame().view())
        view->resumeVisibleImageAnimationsIncludingSubframes();
}

void Page::setActivityState(ActivityState::Flags activityState)
{
    ActivityState::Flags changed = m_activityState ^ activityState;
    if (!changed)
        return;

    ActivityState::Flags oldActivityState = m_activityState;

    bool wasVisibleAndActive = isVisibleAndActive();
    m_activityState = activityState;

    m_focusController->setActivityState(activityState);

    if (changed & ActivityState::IsVisible)
        setIsVisibleInternal(activityState & ActivityState::IsVisible);
    if (changed & ActivityState::IsInWindow)
        setIsInWindowInternal(activityState & ActivityState::IsInWindow);
    if (changed & ActivityState::IsVisuallyIdle)
        setIsVisuallyIdleInternal(activityState & ActivityState::IsVisuallyIdle);
    if (changed & ActivityState::WindowIsActive) {
        if (auto* view = m_mainFrame->view())
            view->updateTiledBackingAdaptiveSizing();
    }

    if (changed & (ActivityState::IsVisible | ActivityState::IsVisuallyIdle | ActivityState::IsAudible | ActivityState::IsLoading | ActivityState::IsCapturingMedia))
        updateTimerThrottlingState();

    for (auto* observer : m_activityStateChangeObservers)
        observer->activityStateDidChange(oldActivityState, m_activityState);

    if (wasVisibleAndActive != isVisibleAndActive())
        PlatformMediaSessionManager::updateNowPlayingInfoIfNecessary();

    if (m_performanceMonitor)
        m_performanceMonitor->activityStateChanged(oldActivityState, activityState);
}

bool Page::isVisibleAndActive() const
{
    return (m_activityState & ActivityState::IsVisible) && (m_activityState & ActivityState::WindowIsActive);
}

bool Page::isWindowActive() const
{
    return m_activityState & ActivityState::WindowIsActive;
}

void Page::setIsVisible(bool isVisible)
{
    if (isVisible)
        setActivityState((m_activityState & ~ActivityState::IsVisuallyIdle) | ActivityState::IsVisible | ActivityState::IsVisibleOrOccluded);
    else
        setActivityState((m_activityState & ~(ActivityState::IsVisible | ActivityState::IsVisibleOrOccluded)) | ActivityState::IsVisuallyIdle);
}

enum class SVGAnimationsState { Paused, Resumed };
static inline void setSVGAnimationsState(Page& page, SVGAnimationsState state)
{
    for (Frame* frame = &page.mainFrame(); frame; frame = frame->tree().traverseNext()) {
        auto* document = frame->document();
        if (!document)
            continue;

        if (!document->svgExtensions())
            continue;

        if (state == SVGAnimationsState::Paused)
            document->accessSVGExtensions().pauseAnimations();
        else
            document->accessSVGExtensions().unpauseAnimations();
    }
}

void Page::setIsVisibleInternal(bool isVisible)
{
    // FIXME: The visibility state should be stored on the top-level document.
    // https://bugs.webkit.org/show_bug.cgi?id=116769

    if (isVisible) {
        m_isPrerender = false;

        resumeScriptedAnimations();
#if PLATFORM(IOS)
        resumeDeviceMotionAndOrientationUpdates();
#endif

        if (FrameView* view = mainFrame().view())
            view->show();

        if (m_settings->hiddenPageCSSAnimationSuspensionEnabled())
            mainFrame().animation().resumeAnimations();

        setSVGAnimationsState(*this, SVGAnimationsState::Resumed);

        resumeAnimatingImages();

        if (m_navigationToLogWhenVisible) {
            logNavigation(m_navigationToLogWhenVisible.value());
            m_navigationToLogWhenVisible = std::nullopt;
        }
    }

    if (!isVisible) {
        if (m_settings->hiddenPageCSSAnimationSuspensionEnabled())
            mainFrame().animation().suspendAnimations();

        setSVGAnimationsState(*this, SVGAnimationsState::Paused);

#if PLATFORM(IOS)
        suspendDeviceMotionAndOrientationUpdates();
#endif

        suspendScriptedAnimations();

        if (FrameView* view = mainFrame().view())
            view->hide();
    }

    Vector<Ref<Document>> documents;
    for (Frame* frame = &m_mainFrame.get(); frame; frame = frame->tree().traverseNext())
        documents.append(*frame->document());

    for (auto& document : documents)
        document->visibilityStateChanged();
}

void Page::setIsPrerender()
{
    m_isPrerender = true;
    updateDOMTimerAlignmentInterval();
}

PageVisibilityState Page::visibilityState() const
{
    if (isVisible())
        return PageVisibilityState::Visible;
    if (m_isPrerender)
        return PageVisibilityState::Prerender;
    return PageVisibilityState::Hidden;
}

#if ENABLE(RUBBER_BANDING)
void Page::addHeaderWithHeight(int headerHeight)
{
    m_headerHeight = headerHeight;

    FrameView* frameView = mainFrame().view();
    if (!frameView)
        return;

    RenderView* renderView = frameView->renderView();
    if (!renderView)
        return;

    frameView->setHeaderHeight(m_headerHeight);
    renderView->compositor().updateLayerForHeader(m_headerHeight);
}

void Page::addFooterWithHeight(int footerHeight)
{
    m_footerHeight = footerHeight;

    FrameView* frameView = mainFrame().view();
    if (!frameView)
        return;

    RenderView* renderView = frameView->renderView();
    if (!renderView)
        return;

    frameView->setFooterHeight(m_footerHeight);
    renderView->compositor().updateLayerForFooter(m_footerHeight);
}
#endif

void Page::incrementNestedRunLoopCount()
{
    m_nestedRunLoopCount++;
}

void Page::decrementNestedRunLoopCount()
{
    ASSERT(m_nestedRunLoopCount);
    if (m_nestedRunLoopCount <= 0)
        return;

    m_nestedRunLoopCount--;

    if (!m_nestedRunLoopCount && m_unnestCallback) {
        callOnMainThread([this] {
            if (insideNestedRunLoop())
                return;

            // This callback may destruct the Page.
            if (m_unnestCallback) {
                auto callback = WTFMove(m_unnestCallback);
                callback();
            }
        });
    }
}

void Page::whenUnnested(WTF::Function<void()>&& callback)
{
    ASSERT(!m_unnestCallback);

    m_unnestCallback = WTFMove(callback);
}

#if ENABLE(REMOTE_INSPECTOR)
bool Page::remoteInspectionAllowed() const
{
    return m_inspectorDebuggable->remoteDebuggingAllowed();
}

void Page::setRemoteInspectionAllowed(bool allowed)
{
    m_inspectorDebuggable->setRemoteDebuggingAllowed(allowed);
}

String Page::remoteInspectionNameOverride() const
{
    return m_inspectorDebuggable->nameOverride();
}

void Page::setRemoteInspectionNameOverride(const String& name)
{
    m_inspectorDebuggable->setNameOverride(name);
}

void Page::remoteInspectorInformationDidChange() const
{
    m_inspectorDebuggable->update();
}
#endif

void Page::addLayoutMilestones(LayoutMilestones milestones)
{
    // In the future, we may want a function that replaces m_layoutMilestones instead of just adding to it.
    m_requestedLayoutMilestones |= milestones;
}

void Page::removeLayoutMilestones(LayoutMilestones milestones)
{
    m_requestedLayoutMilestones &= ~milestones;
}

Color Page::pageExtendedBackgroundColor() const
{
    FrameView* frameView = mainFrame().view();
    if (!frameView)
        return Color();

    RenderView* renderView = frameView->renderView();
    if (!renderView)
        return Color();

    return renderView->compositor().rootExtendedBackgroundColor();
}

// These are magical constants that might be tweaked over time.
static const double gMinimumPaintedAreaRatio = 0.1;
static const double gMaximumUnpaintedAreaRatio = 0.04;

bool Page::isCountingRelevantRepaintedObjects() const
{
    return m_isCountingRelevantRepaintedObjects && (m_requestedLayoutMilestones & DidHitRelevantRepaintedObjectsAreaThreshold);
}

void Page::startCountingRelevantRepaintedObjects()
{
    // Reset everything in case we didn't hit the threshold last time.
    resetRelevantPaintedObjectCounter();

    m_isCountingRelevantRepaintedObjects = true;
}

void Page::resetRelevantPaintedObjectCounter()
{
    m_isCountingRelevantRepaintedObjects = false;
    m_relevantUnpaintedRenderObjects.clear();
    m_topRelevantPaintedRegion = Region();
    m_bottomRelevantPaintedRegion = Region();
    m_relevantUnpaintedRegion = Region();
}

static LayoutRect relevantViewRect(RenderView* view)
{
    // DidHitRelevantRepaintedObjectsAreaThreshold is a LayoutMilestone intended to indicate that
    // a certain relevant amount of content has been drawn to the screen. This is the rect that
    // has been determined to be relevant in the context of this goal. We may choose to tweak
    // the rect over time, much like we may choose to tweak gMinimumPaintedAreaRatio and
    // gMaximumUnpaintedAreaRatio. But this seems to work well right now.
    LayoutRect relevantViewRect = LayoutRect(0, 0, 980, 1300);

    LayoutRect viewRect = view->viewRect();
    // If the viewRect is wider than the relevantViewRect, center the relevantViewRect.
    if (viewRect.width() > relevantViewRect.width())
        relevantViewRect.setX((viewRect.width() - relevantViewRect.width()) / 2);

    return relevantViewRect;
}

void Page::addRelevantRepaintedObject(RenderObject* object, const LayoutRect& objectPaintRect)
{
    if (!isCountingRelevantRepaintedObjects())
        return;

    // Objects inside sub-frames are not considered to be relevant.
    if (&object->frame() != &mainFrame())
        return;

    LayoutRect relevantRect = relevantViewRect(&object->view());

    // The objects are only relevant if they are being painted within the viewRect().
    if (!objectPaintRect.intersects(snappedIntRect(relevantRect)))
        return;

    IntRect snappedPaintRect = snappedIntRect(objectPaintRect);

    // If this object was previously counted as an unpainted object, remove it from that HashSet
    // and corresponding Region. FIXME: This doesn't do the right thing if the objects overlap.
    if (m_relevantUnpaintedRenderObjects.remove(object))
        m_relevantUnpaintedRegion.subtract(snappedPaintRect);

    // Split the relevantRect into a top half and a bottom half. Making sure we have coverage in
    // both halves helps to prevent cases where we have a fully loaded menu bar or masthead with
    // no content beneath that.
    LayoutRect topRelevantRect = relevantRect;
    topRelevantRect.contract(LayoutSize(0, relevantRect.height() / 2));
    LayoutRect bottomRelevantRect = topRelevantRect;
    bottomRelevantRect.setY(relevantRect.height() / 2);

    // If the rect straddles both Regions, split it appropriately.
    if (topRelevantRect.intersects(snappedPaintRect) && bottomRelevantRect.intersects(snappedPaintRect)) {
        IntRect topIntersection = snappedPaintRect;
        topIntersection.intersect(snappedIntRect(topRelevantRect));
        m_topRelevantPaintedRegion.unite(topIntersection);

        IntRect bottomIntersection = snappedPaintRect;
        bottomIntersection.intersect(snappedIntRect(bottomRelevantRect));
        m_bottomRelevantPaintedRegion.unite(bottomIntersection);
    } else if (topRelevantRect.intersects(snappedPaintRect))
        m_topRelevantPaintedRegion.unite(snappedPaintRect);
    else
        m_bottomRelevantPaintedRegion.unite(snappedPaintRect);

    float topPaintedArea = m_topRelevantPaintedRegion.totalArea();
    float bottomPaintedArea = m_bottomRelevantPaintedRegion.totalArea();
    float viewArea = relevantRect.width() * relevantRect.height();

    float ratioThatIsPaintedOnTop = topPaintedArea / viewArea;
    float ratioThatIsPaintedOnBottom = bottomPaintedArea / viewArea;
    float ratioOfViewThatIsUnpainted = m_relevantUnpaintedRegion.totalArea() / viewArea;

    if (ratioThatIsPaintedOnTop > (gMinimumPaintedAreaRatio / 2) && ratioThatIsPaintedOnBottom > (gMinimumPaintedAreaRatio / 2)
        && ratioOfViewThatIsUnpainted < gMaximumUnpaintedAreaRatio) {
        m_isCountingRelevantRepaintedObjects = false;
        resetRelevantPaintedObjectCounter();
        if (Frame* frame = &mainFrame())
            frame->loader().didReachLayoutMilestone(DidHitRelevantRepaintedObjectsAreaThreshold);
    }
}

void Page::addRelevantUnpaintedObject(RenderObject* object, const LayoutRect& objectPaintRect)
{
    if (!isCountingRelevantRepaintedObjects())
        return;

    // The objects are only relevant if they are being painted within the relevantViewRect().
    if (!objectPaintRect.intersects(snappedIntRect(relevantViewRect(&object->view()))))
        return;

    m_relevantUnpaintedRenderObjects.add(object);
    m_relevantUnpaintedRegion.unite(snappedIntRect(objectPaintRect));
}

void Page::suspendDeviceMotionAndOrientationUpdates()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (Document* document = frame->document())
            document->suspendDeviceMotionAndOrientationUpdates();
    }
}

void Page::resumeDeviceMotionAndOrientationUpdates()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (Document* document = frame->document())
            document->resumeDeviceMotionAndOrientationUpdates();
    }
}

void Page::suspendActiveDOMObjectsAndAnimations()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext())
        frame->suspendActiveDOMObjectsAndAnimations();
}

void Page::resumeActiveDOMObjectsAndAnimations()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext())
        frame->resumeActiveDOMObjectsAndAnimations();

    resumeAnimatingImages();
}

bool Page::hasSeenAnyPlugin() const
{
    return !m_seenPlugins.isEmpty();
}

bool Page::hasSeenPlugin(const String& serviceType) const
{
    return m_seenPlugins.contains(serviceType);
}

void Page::sawPlugin(const String& serviceType)
{
    m_seenPlugins.add(serviceType);
}

void Page::resetSeenPlugins()
{
    m_seenPlugins.clear();
}

bool Page::hasSeenAnyMediaEngine() const
{
    return !m_seenMediaEngines.isEmpty();
}

bool Page::hasSeenMediaEngine(const String& engineDescription) const
{
    return m_seenMediaEngines.contains(engineDescription);
}

void Page::sawMediaEngine(const String& engineDescription)
{
    m_seenMediaEngines.add(engineDescription);
}

void Page::resetSeenMediaEngines()
{
    m_seenMediaEngines.clear();
}

void Page::hiddenPageCSSAnimationSuspensionStateChanged()
{
    if (!isVisible()) {
        if (m_settings->hiddenPageCSSAnimationSuspensionEnabled())
            mainFrame().animation().suspendAnimations();
        else
            mainFrame().animation().resumeAnimations();
    }
}

#if ENABLE(VIDEO_TRACK)
void Page::captionPreferencesChanged()
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->captionPreferencesChanged();
    }
}
#endif

void Page::forbidPrompts()
{
    ++m_forbidPromptsDepth;
}

void Page::allowPrompts()
{
    ASSERT(m_forbidPromptsDepth);
    --m_forbidPromptsDepth;
}

bool Page::arePromptsAllowed()
{
    return !m_forbidPromptsDepth;
}

void Page::logNavigation(const Navigation& navigation)
{
    String navigationDescription;
    switch (navigation.type) {
    case FrameLoadType::Standard:
        navigationDescription = ASCIILiteral("standard");
        break;
    case FrameLoadType::Back:
        navigationDescription = ASCIILiteral("back");
        break;
    case FrameLoadType::Forward:
        navigationDescription = ASCIILiteral("forward");
        break;
    case FrameLoadType::IndexedBackForward:
        navigationDescription = ASCIILiteral("indexedBackForward");
        break;
    case FrameLoadType::Reload:
        navigationDescription = ASCIILiteral("reload");
        break;
    case FrameLoadType::Same:
        navigationDescription = ASCIILiteral("same");
        break;
    case FrameLoadType::ReloadFromOrigin:
        navigationDescription = ASCIILiteral("reloadFromOrigin");
        break;
    case FrameLoadType::ReloadExpiredOnly:
        navigationDescription = ASCIILiteral("reloadRevalidatingExpired");
        break;
    case FrameLoadType::Replace:
    case FrameLoadType::RedirectWithLockedBackForwardList:
        // Not logging those for now.
        return;
    }
    diagnosticLoggingClient().logDiagnosticMessage(DiagnosticLoggingKeys::navigationKey(), navigationDescription, ShouldSample::No);

    if (!navigation.domain.isEmpty())
        diagnosticLoggingClient().logDiagnosticMessageWithEnhancedPrivacy(DiagnosticLoggingKeys::domainVisitedKey(), navigation.domain, ShouldSample::No);
}

void Page::mainFrameLoadStarted(const URL& destinationURL, FrameLoadType type)
{
    String domain;
#if ENABLE(PUBLIC_SUFFIX_LIST)
    domain = topPrivatelyControlledDomain(destinationURL.host());
#else
    UNUSED_PARAM(destinationURL);
#endif

    Navigation navigation = { domain, type };

    // To avoid being too verbose, we only log navigations if the page is or becomes visible. This avoids logging non-user observable loads.
    if (!isVisible()) {
        m_navigationToLogWhenVisible = navigation;
        return;
    }

    m_navigationToLogWhenVisible = std::nullopt;
    logNavigation(navigation);
}

PluginInfoProvider& Page::pluginInfoProvider()
{
    return m_pluginInfoProvider;
}

UserContentProvider& Page::userContentProvider()
{
    return m_userContentProvider;
}

void Page::setUserContentProvider(Ref<UserContentProvider>&& userContentProvider)
{
    m_userContentProvider->removePage(*this);
    m_userContentProvider = WTFMove(userContentProvider);
    m_userContentProvider->addPage(*this);

    invalidateInjectedStyleSheetCacheInAllFrames();
}

void Page::setStorageNamespaceProvider(Ref<StorageNamespaceProvider>&& storageNamespaceProvider)
{
    m_storageNamespaceProvider->removePage(*this);
    m_storageNamespaceProvider = WTFMove(storageNamespaceProvider);
    m_storageNamespaceProvider->addPage(*this);

    // This needs to reset all the local storage namespaces of all the pages.
}

VisitedLinkStore& Page::visitedLinkStore()
{
    return m_visitedLinkStore;
}

void Page::setVisitedLinkStore(Ref<VisitedLinkStore>&& visitedLinkStore)
{
    m_visitedLinkStore->removePage(*this);
    m_visitedLinkStore = WTFMove(visitedLinkStore);
    m_visitedLinkStore->addPage(*this);

    invalidateStylesForAllLinks();
}

SessionID Page::sessionID() const
{
    return m_sessionID;
}

void Page::setSessionID(SessionID sessionID)
{
    ASSERT(sessionID.isValid());

#if ENABLE(INDEXED_DATABASE)
    if (sessionID != m_sessionID)
        m_idbIDBConnectionToServer = nullptr;
#endif

    bool privateBrowsingStateChanged = (sessionID.isEphemeral() != m_sessionID.isEphemeral());

    m_sessionID = sessionID;

    if (!privateBrowsingStateChanged)
        return;

    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->privateBrowsingStateDidChange();
    }

    // Collect the PluginViews in to a vector to ensure that action the plug-in takes
    // from below privateBrowsingStateChanged does not affect their lifetime.

    for (auto& view : pluginViews())
        view->privateBrowsingStateChanged(sessionID.isEphemeral());
}

#if ENABLE(WIRELESS_PLAYBACK_TARGET)
void Page::addPlaybackTargetPickerClient(uint64_t contextId)
{
    chrome().client().addPlaybackTargetPickerClient(contextId);
}

void Page::removePlaybackTargetPickerClient(uint64_t contextId)
{
    chrome().client().removePlaybackTargetPickerClient(contextId);
}

void Page::showPlaybackTargetPicker(uint64_t contextId, const WebCore::IntPoint& location, bool isVideo)
{
#if PLATFORM(IOS)
    // FIXME: refactor iOS implementation.
    UNUSED_PARAM(contextId);
    UNUSED_PARAM(location);
    chrome().client().showPlaybackTargetPicker(isVideo);
#else
    chrome().client().showPlaybackTargetPicker(contextId, location, isVideo);
#endif
}

void Page::playbackTargetPickerClientStateDidChange(uint64_t contextId, MediaProducer::MediaStateFlags state)
{
    chrome().client().playbackTargetPickerClientStateDidChange(contextId, state);
}

void Page::setMockMediaPlaybackTargetPickerEnabled(bool enabled)
{
    chrome().client().setMockMediaPlaybackTargetPickerEnabled(enabled);
}

void Page::setMockMediaPlaybackTargetPickerState(const String& name, MediaPlaybackTargetContext::State state)
{
    chrome().client().setMockMediaPlaybackTargetPickerState(name, state);
}

void Page::setPlaybackTarget(uint64_t contextId, Ref<MediaPlaybackTarget>&& target)
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->setPlaybackTarget(contextId, target.copyRef());
    }
}

void Page::playbackTargetAvailabilityDidChange(uint64_t contextId, bool available)
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->playbackTargetAvailabilityDidChange(contextId, available);
    }
}

void Page::setShouldPlayToPlaybackTarget(uint64_t clientId, bool shouldPlay)
{
    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->setShouldPlayToPlaybackTarget(clientId, shouldPlay);
    }
}
#endif

WheelEventTestTrigger& Page::ensureTestTrigger()
{
    if (!m_testTrigger) {
        m_testTrigger = adoptRef(new WheelEventTestTrigger());
        // We need to update the scrolling coordinator so that the mainframe scrolling node can expect wheel event test triggers.
        if (auto* frameView = mainFrame().view()) {
            if (m_scrollingCoordinator)
                m_scrollingCoordinator->updateExpectsWheelEventTestTriggerWithFrameView(*frameView);
        }
    }

    return *m_testTrigger;
}

#if ENABLE(VIDEO)
void Page::setAllowsMediaDocumentInlinePlayback(bool flag)
{
    if (m_allowsMediaDocumentInlinePlayback == flag)
        return;
    m_allowsMediaDocumentInlinePlayback = flag;

    Vector<Ref<Document>> documents;
    for (Frame* frame = &m_mainFrame.get(); frame; frame = frame->tree().traverseNext())
        documents.append(*frame->document());

    for (auto& document : documents)
        document->allowsMediaDocumentInlinePlaybackChanged();
}
#endif

#if ENABLE(INDEXED_DATABASE)
IDBClient::IDBConnectionToServer& Page::idbConnection()
{
    if (!m_idbIDBConnectionToServer)
        m_idbIDBConnectionToServer = &databaseProvider().idbConnectionToServerForSession(m_sessionID);
    
    return *m_idbIDBConnectionToServer;
}
#endif

#if ENABLE(RESOURCE_USAGE)
void Page::setResourceUsageOverlayVisible(bool visible)
{
    if (!visible) {
        m_resourceUsageOverlay = nullptr;
        return;
    }

    if (!m_resourceUsageOverlay && m_settings->acceleratedCompositingEnabled())
        m_resourceUsageOverlay = std::make_unique<ResourceUsageOverlay>(*this);
}
#endif

bool Page::isAlwaysOnLoggingAllowed() const
{
    return m_sessionID.isAlwaysOnLoggingAllowed();
}

String Page::captionUserPreferencesStyleSheet()
{
    return m_captionUserPreferencesStyleSheet;
}

void Page::setCaptionUserPreferencesStyleSheet(const String& styleSheet)
{
    if (m_captionUserPreferencesStyleSheet == styleSheet)
        return;

    m_captionUserPreferencesStyleSheet = styleSheet;
    
    invalidateInjectedStyleSheetCacheInAllFrames();
}

void Page::accessibilitySettingsDidChange()
{
    bool neededRecalc = false;

    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (Document* document = frame->document()) {
            auto* styleResolver = document->styleScope().resolverIfExists();
            if (styleResolver && styleResolver->hasMediaQueriesAffectedByAccessibilitySettingsChange()) {
                document->styleScope().didChangeStyleSheetEnvironment();
                document->evaluateMediaQueryList();
                neededRecalc = true;
                // FIXME: This instrumentation event is not strictly accurate since cached media query results do not persist across StyleResolver rebuilds.
                InspectorInstrumentation::mediaQueryResultChanged(*document);
            }
        }
    }

    if (neededRecalc)
        LOG(Layout, "hasMediaQueriesAffectedByAccessibilitySettingsChange, enqueueing style recalc");
}

void Page::setUnobscuredSafeAreaInsets(const FloatBoxExtent& insets)
{
    if (m_unobscuredSafeAreaInsets == insets)
        return;

    m_unobscuredSafeAreaInsets = insets;

    for (Frame* frame = &mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->document())
            continue;
        frame->document()->constantProperties().didChangeSafeAreaInsets();
    }
}

#if ENABLE(DATA_INTERACTION)

bool Page::hasSelectionAtPosition(const FloatPoint& position) const
{
    auto currentSelection = m_mainFrame->selection().selection();
    if (!currentSelection.isRange())
        return false;

    if (auto selectedRange = currentSelection.toNormalizedRange()) {
        Vector<SelectionRect> selectionRects;
        selectedRange->collectSelectionRects(selectionRects);
        for (auto selectionRect : selectionRects) {
            if (FloatRect(selectionRect.rect()).contains(position))
                return true;
        }
    }

    return false;
}

#endif

} // namespace WebCore
