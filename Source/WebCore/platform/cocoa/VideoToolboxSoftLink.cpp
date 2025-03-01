/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "config.h"

#include "SoftLinking.h"
#include <VideoToolbox/VideoToolbox.h>

SOFT_LINK_FRAMEWORK_FOR_SOURCE(WebCore, VideoToolbox)

SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, VideoToolbox, VTDecompressionSessionCreate, OSStatus, (CFAllocatorRef allocator, CMVideoFormatDescriptionRef videoFormatDescription, CFDictionaryRef videoDecoderSpecification, CFDictionaryRef destinationImageBufferAttributes, const VTDecompressionOutputCallbackRecord* outputCallback, VTDecompressionSessionRef* decompressionSessionOut), (allocator, videoFormatDescription, videoDecoderSpecification, destinationImageBufferAttributes, outputCallback, decompressionSessionOut))
#define VTDecompressionSessionCreate softLink_VideoToolbox_VTDecompressionSessionCreate
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, VideoToolbox, VTDecompressionSessionCanAcceptFormatDescription, Boolean, (VTDecompressionSessionRef session, CMFormatDescriptionRef newFormatDesc), (session, newFormatDesc))
#define VTDecompressionSessionCanAcceptFormatDescription softLink_VideoToolbox_VTDecompressionSessionCanAcceptFormatDescription
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, VideoToolbox, VTDecompressionSessionWaitForAsynchronousFrames, OSStatus, (VTDecompressionSessionRef session), (session))
#define VTDecompressionSessionWaitForAsynchronousFrames softLink_VideoToolbox_VTDecompressionSessionWaitForAsynchronousFrames
SOFT_LINK_FUNCTION_FOR_SOURCE(WebCore, VideoToolbox, VTDecompressionSessionDecodeFrame, OSStatus, (VTDecompressionSessionRef session, CMSampleBufferRef sampleBuffer, VTDecodeFrameFlags decodeFlags, void* sourceFrameRefCon, VTDecodeInfoFlags* infoFlagsOut), (session, sampleBuffer, decodeFlags, sourceFrameRefCon, infoFlagsOut))
#define VTDecompressionSessionDecodeFrame softLink_VideoToolbox_VTDecompressionSessionDecodeFrame
SOFT_LINK_FUNCTION_MAY_FAIL_FOR_SOURCE(WebCore, VideoToolbox, VTIsHardwareDecodeSupported, Boolean, (CMVideoCodecType codecType), (codecType))
SOFT_LINK_CONSTANT_FOR_SOURCE(WebCore, VideoToolbox, kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder, CFStringRef)
#define kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder get_VideoToolbox_kVTVideoDecoderSpecification_EnableHardwareAcceleratedVideoDecoder()
