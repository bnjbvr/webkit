/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "MessageReceiver.h"

#if PLATFORM(COCOA)
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>
OBJC_CLASS WKCustomProtocolLoader;
#endif

namespace IPC {
class DataReference;
}

namespace WebCore {
class ResourceError;
class ResourceRequest;
class ResourceResponse;
} // namespace WebCore

namespace WebKit {

class ChildProcessProxy;
class WebProcessPool;

class LegacyCustomProtocolManagerProxy : public IPC::MessageReceiver {
public:
    LegacyCustomProtocolManagerProxy(ChildProcessProxy*, WebProcessPool&);
    ~LegacyCustomProtocolManagerProxy();

    void startLoading(uint64_t customProtocolID, const WebCore::ResourceRequest&);
    void stopLoading(uint64_t customProtocolID);

    void processDidClose();

    void wasRedirectedToRequest(uint64_t customProtocolID, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);
    void didReceiveResponse(uint64_t customProtocolID, const WebCore::ResourceResponse&, uint32_t cacheStoragePolicy);
    void didLoadData(uint64_t customProtocolID, const IPC::DataReference&);
    void didFailWithError(uint64_t customProtocolID, const WebCore::ResourceError&);
    void didFinishLoading(uint64_t customProtocolID);

private:
    // IPC::MessageReceiver
    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) override;

    ChildProcessProxy* m_childProcessProxy;
    WebProcessPool& m_processPool;

#if PLATFORM(COCOA)
    typedef HashMap<uint64_t, RetainPtr<WKCustomProtocolLoader>> LoaderMap;
    LoaderMap m_loaderMap;
#endif
};

} // namespace WebKit
