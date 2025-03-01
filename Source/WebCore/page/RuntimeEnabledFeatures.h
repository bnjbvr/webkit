/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2013-2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "PlatformExportMacros.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

// A class that stores static enablers for all experimental features. Note that
// the method names must line up with the JavaScript method they enable for code
// generation to work properly.

class RuntimeEnabledFeatures {
    WTF_MAKE_NONCOPYABLE(RuntimeEnabledFeatures);
public:
    void setDisplayContentsEnabled(bool isEnabled) { m_isDisplayContentsEnabled = isEnabled; }
    bool displayContentsEnabled() const { return m_isDisplayContentsEnabled; }

    void setLinkPreloadEnabled(bool isEnabled) { m_isLinkPreloadEnabled = isEnabled; }
    bool linkPreloadEnabled() const { return m_isLinkPreloadEnabled; }

    void setMediaPreloadingEnabled(bool isEnabled) { m_isMediaPreloadingEnabled = isEnabled; }
    bool mediaPreloadingEnabled() const { return m_isMediaPreloadingEnabled; }

    void setResourceTimingEnabled(bool isEnabled) { m_isResourceTimingEnabled = isEnabled; }
    bool resourceTimingEnabled() const { return m_isResourceTimingEnabled; }

    void setUserTimingEnabled(bool isEnabled) { m_isUserTimingEnabled = isEnabled; }
    bool userTimingEnabled() const { return m_isUserTimingEnabled; }

    bool performanceTimelineEnabled() const { return resourceTimingEnabled() || userTimingEnabled(); }

    void setShadowDOMEnabled(bool isEnabled) { m_isShadowDOMEnabled = isEnabled; }
    bool shadowDOMEnabled() const { return m_isShadowDOMEnabled; }

    void setInputEventsEnabled(bool isEnabled) { m_inputEventsEnabled = isEnabled; }
    bool inputEventsEnabled() const { return m_inputEventsEnabled; }

    void setInteractiveFormValidationEnabled(bool isEnabled) { m_isInteractiveFormValidationEnabled = isEnabled; }
    bool interactiveFormValidationEnabled() const { return m_isInteractiveFormValidationEnabled; }

    void setCustomElementsEnabled(bool areEnabled) { m_areCustomElementsEnabled = areEnabled; }
    bool customElementsEnabled() const { return m_areCustomElementsEnabled; }

    void setModernMediaControlsEnabled(bool areEnabled) { m_areModernMediaControlsEnabled = areEnabled; }
    bool modernMediaControlsEnabled() const { return m_areModernMediaControlsEnabled; }

    void setCredentialManagementEnabled(bool isEnabled) { m_isCredentialManagementEnabled = isEnabled; }
    bool credentialManagementEnabled() const { return m_isCredentialManagementEnabled; }

    void setIsSecureContextAttributeEnabled(bool isEnabled) { m_isSecureContextAttributeEnabled = isEnabled; }
    bool isSecureContextAttributeEnabled() const { return m_isSecureContextAttributeEnabled; }

#if ENABLE(INDEXED_DATABASE_IN_WORKERS)
    void setIndexedDBWorkersEnabled(bool isEnabled) { m_isIndexedDBWorkersEnabled = isEnabled; }
    bool indexedDBWorkersEnabled() const { return m_isIndexedDBWorkersEnabled; }
#endif

#if ENABLE(MEDIA_STREAM)
    bool mediaDevicesEnabled() const { return m_isMediaDevicesEnabled; }
    void setMediaDevicesEnabled(bool isEnabled) { m_isMediaDevicesEnabled = isEnabled; }
    bool mediaStreamEnabled() const { return m_isMediaStreamEnabled; }
    void setMediaStreamEnabled(bool isEnabled) { m_isMediaStreamEnabled = isEnabled; }
#endif

#if ENABLE(WEB_RTC)
    bool peerConnectionEnabled() const { return m_isPeerConnectionEnabled; }
    void setPeerConnectionEnabled(bool isEnabled) { m_isPeerConnectionEnabled = isEnabled; }
    bool webRTCLegacyAPIEnabled() const { return m_webRTCLegacyAPIEnabled; }
    void setWebRTCLegacyAPIEnabled(bool isEnabled) { m_webRTCLegacyAPIEnabled = isEnabled; }
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    void setLegacyCSSVendorPrefixesEnabled(bool isEnabled) { m_isLegacyCSSVendorPrefixesEnabled = isEnabled; }
    bool legacyCSSVendorPrefixesEnabled() const { return m_isLegacyCSSVendorPrefixesEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATE)
    bool inputTypeDateEnabled() const { return m_isInputTypeDateEnabled; }
    void setInputTypeDateEnabled(bool isEnabled) { m_isInputTypeDateEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    bool inputTypeDateTimeEnabled() const { return m_isInputTypeDateTimeEnabled; }
    void setInputTypeDateTimeEnabled(bool isEnabled) { m_isInputTypeDateTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    bool inputTypeDateTimeLocalEnabled() const { return m_isInputTypeDateTimeLocalEnabled; }
    void setInputTypeDateTimeLocalEnabled(bool isEnabled) { m_isInputTypeDateTimeLocalEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    bool inputTypeMonthEnabled() const { return m_isInputTypeMonthEnabled; }
    void setInputTypeMonthEnabled(bool isEnabled) { m_isInputTypeMonthEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_TIME)
    bool inputTypeTimeEnabled() const { return m_isInputTypeTimeEnabled; }
    void setInputTypeTimeEnabled(bool isEnabled) { m_isInputTypeTimeEnabled = isEnabled; }
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    bool inputTypeWeekEnabled() const { return m_isInputTypeWeekEnabled; }
    void setInputTypeWeekEnabled(bool isEnabled) { m_isInputTypeWeekEnabled = isEnabled; }
#endif

#if ENABLE(GAMEPAD)
    void setGamepadsEnabled(bool areEnabled) { m_areGamepadsEnabled = areEnabled; }
    bool gamepadsEnabled() const { return m_areGamepadsEnabled; }
#endif

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    // FIXME: This is not used.
    void setAnimationTriggersEnabled(bool areEnabled) { m_areAnimationTriggersEnabled = areEnabled; }
    bool animationTriggersEnabled() const { return m_areAnimationTriggersEnabled; }
#endif

#if ENABLE(WEB_ANIMATIONS)
    void setWebAnimationsEnabled(bool areEnabled) { m_areWebAnimationsEnabled = areEnabled; }
    bool webAnimationsEnabled() const { return m_areWebAnimationsEnabled; }
#endif

#if ENABLE(WEBGL2)
    void setWebGL2Enabled(bool isEnabled) { m_isWebGL2Enabled = isEnabled; }
    bool webGL2Enabled() const { return m_isWebGL2Enabled; }
#endif

#if ENABLE(WEBGPU)
    void setWebGPUEnabled(bool isEnabled) { m_isWebGPUEnabled = isEnabled; }
    bool webGPUEnabled() const { return m_isWebGPUEnabled; }
#endif

#if ENABLE(FETCH_API)
    void setFetchAPIEnabled(bool isEnabled) { m_isFetchAPIEnabled = isEnabled; }
    bool fetchAPIEnabled() const { return m_isFetchAPIEnabled; }
#endif
    
#if ENABLE(STREAMS_API)
    void setReadableByteStreamAPIEnabled(bool isEnabled) { m_isReadableByteStreamAPIEnabled = isEnabled; }
    bool readableByteStreamAPIEnabled() const { return m_isReadableByteStreamAPIEnabled; }
    void setWritableStreamAPIEnabled(bool isEnabled) { m_isWritableStreamAPIEnabled = isEnabled; }
    bool writableStreamAPIEnabled() const { return m_isWritableStreamAPIEnabled; }
#endif
    
#if ENABLE(DOWNLOAD_ATTRIBUTE)
    void setDownloadAttributeEnabled(bool isEnabled) { m_isDownloadAttributeEnabled = isEnabled; }
    bool downloadAttributeEnabled() const { return m_isDownloadAttributeEnabled; }
#endif

    void setCSSGridLayoutEnabled(bool isEnabled) { m_cssGridLayoutEnabled = isEnabled; }
    bool isCSSGridLayoutEnabled() const { return m_cssGridLayoutEnabled; }

#if ENABLE(INTERSECTION_OBSERVER)
    void setIntersectionObserverEnabled(bool isEnabled) { m_intersectionObserverEnabled = isEnabled; }
    bool intersectionObserverEnabled() const { return m_intersectionObserverEnabled; }
#endif

#if ENABLE(ENCRYPTED_MEDIA)
    void setEncryptedMediaAPIEnabled(bool isEnabled) { m_encryptedMediaAPIEnabled = isEnabled; }
    bool encryptedMediaAPIEnabled() const { return m_encryptedMediaAPIEnabled; }
#endif

#if ENABLE(VIDEO)
    bool audioEnabled() const;
#endif

#if ENABLE(WEB_SOCKETS)
    bool webSocketEnabled() const;
#endif

    WEBCORE_EXPORT static RuntimeEnabledFeatures& sharedFeatures();

private:
    // Never instantiate.
    RuntimeEnabledFeatures();

    bool m_areModernMediaControlsEnabled { false };
    bool m_isLinkPreloadEnabled { true };
    bool m_isMediaPreloadingEnabled { false };
    bool m_isResourceTimingEnabled { false };
    bool m_isUserTimingEnabled { false };
    bool m_isInteractiveFormValidationEnabled { false };
    bool m_isCredentialManagementEnabled { false };
    bool m_isSecureContextAttributeEnabled { false };

    bool m_isDisplayContentsEnabled { false };
    bool m_isShadowDOMEnabled { true };
    bool m_areCustomElementsEnabled { true };
    bool m_inputEventsEnabled { true };

#if ENABLE(INDEXED_DATABASE_IN_WORKERS)
    bool m_isIndexedDBWorkersEnabled { true };
#endif

#if ENABLE(MEDIA_STREAM)
    bool m_isMediaDevicesEnabled { false };
    bool m_isMediaStreamEnabled { true };
#endif

#if ENABLE(WEB_RTC)
    bool m_isPeerConnectionEnabled { true };
    bool m_webRTCLegacyAPIEnabled { true };
#endif

#if ENABLE(LEGACY_CSS_VENDOR_PREFIXES)
    bool m_isLegacyCSSVendorPrefixesEnabled { false };
#endif

#if ENABLE(INPUT_TYPE_DATE)
    bool m_isInputTypeDateEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_DATETIME_INCOMPLETE)
    bool m_isInputTypeDateTimeEnabled { false };
#endif

#if ENABLE(INPUT_TYPE_DATETIMELOCAL)
    bool m_isInputTypeDateTimeLocalEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_MONTH)
    bool m_isInputTypeMonthEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_TIME)
    bool m_isInputTypeTimeEnabled { true };
#endif

#if ENABLE(INPUT_TYPE_WEEK)
    bool m_isInputTypeWeekEnabled { true };
#endif

#if ENABLE(GAMEPAD)
    bool m_areGamepadsEnabled { false };
#endif

#if ENABLE(CSS_ANIMATIONS_LEVEL_2)
    bool m_areAnimationTriggersEnabled { false };
#endif

#if ENABLE(STREAMS_API)
    bool m_isReadableByteStreamAPIEnabled { false };
    bool m_isWritableStreamAPIEnabled { false };
#endif

#if ENABLE(WEB_ANIMATIONS)
    bool m_areWebAnimationsEnabled { false };
#endif

#if ENABLE(WEBGL2)
    bool m_isWebGL2Enabled { false };
#endif

#if ENABLE(WEBGPU)
    bool m_isWebGPUEnabled { false };
#endif

#if ENABLE(FETCH_API)
    bool m_isFetchAPIEnabled { true };
#endif

#if ENABLE(DOWNLOAD_ATTRIBUTE)
    bool m_isDownloadAttributeEnabled { false };
#endif

    bool m_cssGridLayoutEnabled { true };

#if ENABLE(ENCRYPTED_MEDIA)
    bool m_encryptedMediaAPIEnabled { false };
#endif

#if ENABLE(INTERSECTION_OBSERVER)
    bool m_intersectionObserverEnabled { false };
#endif

    friend class WTF::NeverDestroyed<RuntimeEnabledFeatures>;
};

} // namespace WebCore
