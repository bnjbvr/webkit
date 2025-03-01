/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "APIData.h"
#include "PluginModuleInfo.h"
#include "ProcessTerminationReason.h"
#include "SameDocumentNavigationType.h"
#include "WebEvent.h"
#include "WebFramePolicyListenerProxy.h"
#include "WebsitePolicies.h"
#include <WebCore/FrameLoaderTypes.h>
#include <WebCore/LayoutMilestones.h>
#include <wtf/Forward.h>

namespace WebCore {
class ResourceError;
class ResourceRequest;
class ResourceResponse;
struct SecurityOriginData;
}

namespace WebKit {
class AuthenticationChallengeProxy;
class QuickLookDocumentData;
class WebBackForwardListItem;
class WebFramePolicyListenerProxy;
class WebFrameProxy;
class WebPageProxy;
class WebProtectionSpace;
struct NavigationActionData;
struct WebNavigationDataStore;
}

namespace API {

class Dictionary;
class Navigation;
class NavigationAction;
class NavigationResponse;
class Object;

class NavigationClient {
public:
    virtual ~NavigationClient() { }

    virtual void didStartProvisionalNavigation(WebKit::WebPageProxy&, API::Navigation*, API::Object*) { }
    virtual void didReceiveServerRedirectForProvisionalNavigation(WebKit::WebPageProxy&, API::Navigation*, API::Object*) { }
    virtual void didFailProvisionalNavigationWithError(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, API::Navigation*, const WebCore::ResourceError&, API::Object*) { }
    virtual void didFailProvisionalLoadInSubframeWithError(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, const WebCore::SecurityOriginData&, API::Navigation*, const WebCore::ResourceError&, API::Object*) { }
    virtual void didCommitNavigation(WebKit::WebPageProxy&, API::Navigation*, API::Object*) { }
    virtual void didFinishDocumentLoad(WebKit::WebPageProxy&, API::Navigation*, API::Object*) { }
    virtual void didFinishNavigation(WebKit::WebPageProxy&, API::Navigation*, API::Object*) { }
    virtual void didFailNavigationWithError(WebKit::WebPageProxy&, WebKit::WebFrameProxy&, API::Navigation*, const WebCore::ResourceError&, API::Object*) { }
    virtual void didSameDocumentNavigation(WebKit::WebPageProxy&, API::Navigation*, WebKit::SameDocumentNavigationType, API::Object*) { }

    virtual void renderingProgressDidChange(WebKit::WebPageProxy&, WebCore::LayoutMilestones) { }

    virtual bool canAuthenticateAgainstProtectionSpace(WebKit::WebPageProxy&, WebKit::WebProtectionSpace*) { return false; }
    virtual void didReceiveAuthenticationChallenge(WebKit::WebPageProxy&, WebKit::AuthenticationChallengeProxy*) { }

    // FIXME: These function should not be part of this client.
    virtual void processDidTerminate(WebKit::WebPageProxy&, WebKit::ProcessTerminationReason) { }
    virtual void processDidBecomeResponsive(WebKit::WebPageProxy&) { }
    virtual void processDidBecomeUnresponsive(WebKit::WebPageProxy&) { }

    virtual RefPtr<Data> webCryptoMasterKey(WebKit::WebPageProxy&) { return nullptr; }

#if USE(QUICK_LOOK)
    virtual void didStartLoadForQuickLookDocumentInMainFrame(const WTF::String& fileName, const WTF::String& uti) { }
    virtual void didFinishLoadForQuickLookDocumentInMainFrame(const WebKit::QuickLookDocumentData&) { }
#endif

    virtual void decidePolicyForNavigationAction(WebKit::WebPageProxy&, API::NavigationAction&, Ref<WebKit::WebFramePolicyListenerProxy>&& listener, API::Object*)
    {
        listener->use({ });
    }

    virtual void decidePolicyForNavigationResponse(WebKit::WebPageProxy&, API::NavigationResponse&, Ref<WebKit::WebFramePolicyListenerProxy>&& listener, API::Object*)
    {
        listener->use({ });
    }
    
#if ENABLE(NETSCAPE_PLUGIN_API)
    virtual WebKit::PluginModuleLoadPolicy decidePolicyForPluginLoad(WebKit::WebPageProxy&, WebKit::PluginModuleLoadPolicy currentPluginLoadPolicy, API::Dictionary*, WTF::String&)
    {
        return currentPluginLoadPolicy;
    }
#endif

    virtual void didBeginNavigationGesture(WebKit::WebPageProxy&) { }
    virtual void willEndNavigationGesture(WebKit::WebPageProxy&, bool willNavigate, WebKit::WebBackForwardListItem&) { }
    virtual void didEndNavigationGesture(WebKit::WebPageProxy&, bool willNavigate, WebKit::WebBackForwardListItem&) { }
    virtual void didRemoveNavigationGestureSnapshot(WebKit::WebPageProxy&) { }
};

} // namespace API
