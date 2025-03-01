/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "WebDragClient.h"

#if ENABLE(DRAG_SUPPORT)

#import "DOMElementInternal.h"
#import "WebArchive.h"
#import "WebDOMOperations.h"
#import "WebFrame.h"
#import "WebFrameInternal.h"
#import "WebFrameView.h"
#import "WebHTMLViewInternal.h"
#import "WebKitLogging.h"
#import "WebKitNSStringExtras.h"
#import "WebNSURLExtras.h"
#import "WebUIDelegate.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"

#if PLATFORM(MAC)
#import "WebNSPasteboardExtras.h"
#endif

#import <WebCore/DataTransfer.h>
#import <WebCore/DragData.h>
#import <WebCore/Editor.h>
#import <WebCore/EditorClient.h>
#import <WebCore/EventHandler.h>
#import <WebCore/FrameView.h>
#import <WebCore/Image.h>
#import <WebCore/MainFrame.h>
#import <WebCore/Page.h>
#import <WebCore/Pasteboard.h>
#import <WebCore/PasteboardWriter.h>

using namespace WebCore;

WebDragClient::WebDragClient(WebView* webView)
    : m_webView(webView) 
{
    UNUSED_PARAM(m_webView);
}

#if PLATFORM(MAC)

bool WebDragClient::useLegacyDragClient()
{
    return false;
}

void WebDragClient::didConcludeEditDrag()
{
}

static WebHTMLView *getTopHTMLView(Frame* frame)
{
    ASSERT(frame);
    ASSERT(frame->page());
    return (WebHTMLView*)[[kit(&frame->page()->mainFrame()) frameView] documentView];
}

void WebDragClient::willPerformDragDestinationAction(WebCore::DragDestinationAction action, const WebCore::DragData& dragData)
{
    [[m_webView _UIDelegateForwarder] webView:m_webView willPerformDragDestinationAction:(WebDragDestinationAction)action forDraggingInfo:dragData.platformData()];
}


WebCore::DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint& rootViewPoint)
{
    NSPoint viewPoint = [m_webView _convertPointFromRootView:rootViewPoint];
    return (DragSourceAction)[[m_webView _UIDelegateForwarder] webView:m_webView dragSourceActionMaskForPoint:viewPoint];
}

void WebDragClient::willPerformDragSourceAction(WebCore::DragSourceAction action, const WebCore::IntPoint& mouseDownPoint, WebCore::DataTransfer& dataTransfer)
{
    [[m_webView _UIDelegateForwarder] webView:m_webView willPerformDragSourceAction:(WebDragSourceAction)action fromPoint:mouseDownPoint withPasteboard:[NSPasteboard pasteboardWithName:dataTransfer.pasteboard().name()]];
}

void WebDragClient::startDrag(DragImage dragImage, const IntPoint& at, const IntPoint& eventPos, const FloatPoint&, DataTransfer& dataTransfer, Frame& frame, DragSourceAction dragSourceAction)
{
    RetainPtr<WebHTMLView> htmlView = (WebHTMLView*)[[kit(&frame) frameView] documentView];
    if (![htmlView.get() isKindOfClass:[WebHTMLView class]])
        return;
    
    NSEvent *event = dragSourceAction == DragSourceActionLink ? frame.eventHandler().currentNSEvent() : [htmlView.get() _mouseDownEvent];
    WebHTMLView* topHTMLView = getTopHTMLView(&frame);
    RetainPtr<WebHTMLView> topViewProtector = topHTMLView;
    
    [topHTMLView _stopAutoscrollTimer];
    NSPasteboard *pasteboard = [NSPasteboard pasteboardWithName:dataTransfer.pasteboard().name()];

    NSImage *dragNSImage = dragImage.get().get();
    WebHTMLView *sourceHTMLView = htmlView.get();

    IntSize size([dragNSImage size]);
    size.scale(1 / frame.page()->deviceScaleFactor());
    [dragNSImage setSize:size];

    id delegate = [m_webView UIDelegate];
    SEL selector = @selector(webView:dragImage:at:offset:event:pasteboard:source:slideBack:forView:);
    if ([delegate respondsToSelector:selector]) {
        @try {
            [delegate webView:m_webView dragImage:dragNSImage at:at offset:NSZeroSize event:event pasteboard:pasteboard source:sourceHTMLView slideBack:YES forView:topHTMLView];
        } @catch (id exception) {
            ReportDiscardedDelegateException(selector, exception);
        }
    } else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [topHTMLView dragImage:dragNSImage at:at offset:NSZeroSize event:event pasteboard:pasteboard source:sourceHTMLView slideBack:YES];
#pragma clang diagnostic pop
}

void WebDragClient::beginDrag(DragItem dragItem, Frame& frame, const IntPoint& mouseDownPosition, const IntPoint& mouseDraggedPosition, DataTransfer& dataTransfer, DragSourceAction dragSourceAction)
{
    ASSERT(!dataTransfer.pasteboard().hasData());

    RetainPtr<WebHTMLView> topWebHTMLView = dynamic_objc_cast<WebHTMLView>(m_webView.mainFrame.frameView.documentView);
    ASSERT(topWebHTMLView);

    [topWebHTMLView _stopAutoscrollTimer];

    auto draggingItem = adoptNS([[NSDraggingItem alloc] initWithPasteboardWriter:createPasteboardWriter(dragItem.data).get()]);

    auto dragImageSize = IntSize { [dragItem.image.get() size] };

    dragImageSize.scale(1 / frame.page()->deviceScaleFactor());
    [dragItem.image.get() setSize:dragImageSize];

    NSRect draggingFrame = NSMakeRect(mouseDraggedPosition.x() - dragImageSize.width() * dragItem.imageAnchorPoint.x(), mouseDraggedPosition.y() - dragImageSize.height() * dragItem.imageAnchorPoint.y(), dragImageSize.width(), dragImageSize.height());
    [draggingItem setDraggingFrame:draggingFrame contents:dragItem.image.get().get()];

    // FIXME: We should be able to make a fake event with the mosue dragged coordinates.
    NSEvent *event = frame.eventHandler().currentNSEvent();
    [topWebHTMLView.get() beginDraggingSessionWithItems:@[ draggingItem.get() ] event:event source:topWebHTMLView.get()];
}

void WebDragClient::declareAndWriteDragImage(const String& pasteboardName, Element& element, const URL& url, const String& title, WebCore::Frame* frame)
{
    ASSERT(pasteboardName);
    [[NSPasteboard pasteboardWithName:pasteboardName] _web_declareAndWriteDragImageForElement:kit(&element) URL:url title:title archive:[kit(&element) webArchive] source:getTopHTMLView(frame)];
}

#if ENABLE(ATTACHMENT_ELEMENT)
void WebDragClient::declareAndWriteAttachment(const String& pasteboardName, Element& element, const URL& url, const String& path, WebCore::Frame* frame)
{
    ASSERT(pasteboardName);
    
    [[NSPasteboard pasteboardWithName:pasteboardName] _web_declareAndWriteDragImageForElement:kit(&element) URL:url title:path archive:nil source:getTopHTMLView(frame)];
}
#endif

#elif !ENABLE(DATA_INTERACTION)

bool WebDragClient::useLegacyDragClient()
{
    return false;
}

void WebDragClient::didConcludeEditDrag()
{
}

void WebDragClient::willPerformDragDestinationAction(WebCore::DragDestinationAction, const WebCore::DragData&)
{
}

WebCore::DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint&)
{
    return DragSourceActionNone;
}

void WebDragClient::willPerformDragSourceAction(WebCore::DragSourceAction, const WebCore::IntPoint&, WebCore::DataTransfer&)
{
}

void WebDragClient::startDrag(WebCore::DragImage, const IntPoint&, const IntPoint&, const FloatPoint&, DataTransfer&, Frame&, WebCore::DragSourceAction)
{
}

void WebDragClient::beginDrag(DragItem, Frame&, const IntPoint&, const IntPoint&, DataTransfer&, DragSourceAction)
{
}

void WebDragClient::declareAndWriteDragImage(const String&, Element&, const URL&, const String&, WebCore::Frame*)
{
}

#if ENABLE(ATTACHMENT_ELEMENT)
void WebDragClient::declareAndWriteAttachment(const String&, Element&, const URL&, const String&, WebCore::Frame*)
{
}
#endif

#endif

#if PLATFORM(IOS)

bool WebDragClient::useLegacyDragClient()
{
    // FIXME: Move the iOS drag and drop implementation for WebKit1 off of the legacy drag client.
    return true;
}

void WebDragClient::willPerformDragDestinationAction(DragDestinationAction, const DragData&)
{
}

WebCore::DragSourceAction WebDragClient::dragSourceActionMaskForPoint(const IntPoint&)
{
    return DragSourceActionAny;
}

void WebDragClient::willPerformDragSourceAction(WebCore::DragSourceAction, const IntPoint&, DataTransfer&)
{
}

void WebDragClient::startDrag(DragImage dragImage, const IntPoint& point, const IntPoint& eventLocation, const FloatPoint& dragImageAnchor, DataTransfer& dataTransfer, Frame& frame, DragSourceAction dragSourceAction)
{
    [m_webView _setDataInteractionData:dragImage.get().get() textIndicator:dragImage.indicatorData() atClientPosition:eventLocation anchorPoint:dragImageAnchor action:dragSourceAction];
}

void WebDragClient::beginDrag(DragItem, Frame&, const IntPoint&, const IntPoint&, DataTransfer&, DragSourceAction)
{
}

void WebDragClient::declareAndWriteDragImage(const String& pasteboardName, Element& element, const URL& url, const String& label, Frame*)
{
    if (auto* frame = element.document().frame())
        frame->editor().writeImageToPasteboard(*Pasteboard::createForDragAndDrop(), element, url, label);
}

#if ENABLE(ATTACHMENT_ELEMENT)
void WebDragClient::declareAndWriteAttachment(const String&, Element&, const URL&, const String&, Frame*)
{
}
#endif

void WebDragClient::didConcludeEditDrag()
{
    [m_webView _didConcludeEditDataInteraction];
}

#endif // PLATFORM(IOS)

void WebDragClient::dragControllerDestroyed() 
{
    delete this;
}

#endif // ENABLE(DRAG_SUPPORT)
