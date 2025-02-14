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
#include "CryptoAlgorithmECDH.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "CryptoAlgorithmEcKeyParams.h"
#include "CryptoAlgorithmEcdhKeyDeriveParams.h"
#include "CryptoKeyEC.h"
#include "ExceptionCode.h"

namespace WebCore {

Ref<CryptoAlgorithm> CryptoAlgorithmECDH::create()
{
    return adoptRef(*new CryptoAlgorithmECDH);
}

CryptoAlgorithmIdentifier CryptoAlgorithmECDH::identifier() const
{
    return s_identifier;
}

void CryptoAlgorithmECDH::generateKey(const CryptoAlgorithmParameters& parameters, bool extractable, CryptoKeyUsageBitmap usages, KeyOrKeyPairCallback&& callback, ExceptionCallback&& exceptionCallback, ScriptExecutionContext&)
{
    const auto& ecParameters = downcast<CryptoAlgorithmEcKeyParams>(parameters);

    if (usages & (CryptoKeyUsageEncrypt | CryptoKeyUsageDecrypt | CryptoKeyUsageSign | CryptoKeyUsageVerify | CryptoKeyUsageWrapKey | CryptoKeyUsageUnwrapKey)) {
        exceptionCallback(SYNTAX_ERR);
        return;
    }

    auto result = CryptoKeyEC::generatePair(CryptoAlgorithmIdentifier::ECDH, ecParameters.namedCurve, extractable, usages);
    if (result.hasException()) {
        exceptionCallback(result.releaseException().code());
        return;
    }

    auto pair = result.releaseReturnValue();
    pair.publicKey->setUsagesBitmap(0);
    pair.privateKey->setUsagesBitmap(pair.privateKey->usagesBitmap() & (CryptoKeyUsageDeriveKey | CryptoKeyUsageDeriveBits));
    callback(WTFMove(pair));
}

void CryptoAlgorithmECDH::deriveBits(std::unique_ptr<CryptoAlgorithmParameters>&& parameters, Ref<CryptoKey>&& baseKey, size_t length, VectorCallback&& callback, ExceptionCallback&& exceptionCallback, ScriptExecutionContext& context, WorkQueue& workQueue)
{
    // We only accept length that is a multiple of 8.
    if (length % 8) {
        exceptionCallback(OperationError);
        return;
    }

    ASSERT(parameters);
    auto& ecParameters = downcast<CryptoAlgorithmEcdhKeyDeriveParams>(*parameters);

    if (baseKey->type() != CryptoKey::Type::Private) {
        exceptionCallback(INVALID_ACCESS_ERR);
        return;
    }
    ASSERT(ecParameters.publicKey);
    if (ecParameters.publicKey->type() != CryptoKey::Type::Public) {
        exceptionCallback(INVALID_ACCESS_ERR);
        return;
    }
    if (baseKey->algorithmIdentifier() != ecParameters.publicKey->algorithmIdentifier()) {
        exceptionCallback(INVALID_ACCESS_ERR);
        return;
    }
    auto& ecBaseKey = downcast<CryptoKeyEC>(baseKey.get());
    auto& ecPublicKey = downcast<CryptoKeyEC>(*(ecParameters.publicKey.get()));
    if (ecBaseKey.namedCurve() != ecPublicKey.namedCurve()) {
        exceptionCallback(INVALID_ACCESS_ERR);
        return;
    }

    auto unifiedCallback = [callback = WTFMove(callback), exceptionCallback = WTFMove(exceptionCallback)](std::optional<Vector<uint8_t>>&& derivedKey, size_t length) mutable {
        if (!derivedKey) {
            exceptionCallback(OperationError);
            return;
        }
        if (!length) {
            callback(WTFMove(*derivedKey));
            return;
        }
        if (length / 8 > (*derivedKey).size()) {
            exceptionCallback(OperationError);
            return;
        }
        (*derivedKey).shrink(length / 8);
        callback(WTFMove(*derivedKey));
    };
    platformDeriveBits(WTFMove(baseKey), ecParameters.publicKey.releaseNonNull(), length, WTFMove(unifiedCallback), context, workQueue);
}

void CryptoAlgorithmECDH::importKey(SubtleCrypto::KeyFormat format, KeyData&& data, const std::unique_ptr<CryptoAlgorithmParameters>&& parameters, bool extractable, CryptoKeyUsageBitmap usages, KeyCallback&& callback, ExceptionCallback&& exceptionCallback)
{
    ASSERT(parameters);
    const auto& ecParameters = downcast<CryptoAlgorithmEcKeyParams>(*parameters);

    RefPtr<CryptoKeyEC> result;
    switch (format) {
    case SubtleCrypto::KeyFormat::Jwk: {
        JsonWebKey key = WTFMove(WTF::get<JsonWebKey>(data));

        bool isUsagesAllowed = false;
        if (!key.d.isNull()) {
            isUsagesAllowed = isUsagesAllowed || !(usages ^ CryptoKeyUsageDeriveKey);
            isUsagesAllowed = isUsagesAllowed || !(usages ^ CryptoKeyUsageDeriveBits);
            isUsagesAllowed = isUsagesAllowed || !(usages ^ (CryptoKeyUsageDeriveKey | CryptoKeyUsageDeriveBits));
        }
        isUsagesAllowed = isUsagesAllowed || !usages;
        if (!isUsagesAllowed) {
            exceptionCallback(SYNTAX_ERR);
            return;
        }

        if (usages && !key.use.isNull() && key.use != "enc") {
            exceptionCallback(DataError);
            return;
        }

        result = CryptoKeyEC::importJwk(ecParameters.identifier, ecParameters.namedCurve, WTFMove(key), extractable, usages);
        break;
    }
    case SubtleCrypto::KeyFormat::Raw:
        if (usages) {
            exceptionCallback(SYNTAX_ERR);
            return;
        }
        result = CryptoKeyEC::importRaw(ecParameters.identifier, ecParameters.namedCurve, WTFMove(WTF::get<Vector<uint8_t>>(data)), extractable, usages);
        break;
    case SubtleCrypto::KeyFormat::Spki:
        if (usages) {
            exceptionCallback(SYNTAX_ERR);
            return;
        }
        result = CryptoKeyEC::importSpki(ecParameters.identifier, ecParameters.namedCurve, WTFMove(WTF::get<Vector<uint8_t>>(data)), extractable, usages);
        break;
    case SubtleCrypto::KeyFormat::Pkcs8:
        if (usages && (usages ^ CryptoKeyUsageDeriveKey) && (usages ^ CryptoKeyUsageDeriveBits) && (usages ^ (CryptoKeyUsageDeriveKey | CryptoKeyUsageDeriveBits))) {
            exceptionCallback(SYNTAX_ERR);
            return;
        }
        result = CryptoKeyEC::importPkcs8(ecParameters.identifier, ecParameters.namedCurve, WTFMove(WTF::get<Vector<uint8_t>>(data)), extractable, usages);
        break;
    }
    if (!result) {
        exceptionCallback(DataError);
        return;
    }

    callback(*result);
}

void CryptoAlgorithmECDH::exportKey(SubtleCrypto::KeyFormat format, Ref<CryptoKey>&& key, KeyDataCallback&& callback, ExceptionCallback&& exceptionCallback)
{
    const auto& ecKey = downcast<CryptoKeyEC>(key.get());

    if (!ecKey.keySizeInBits()) {
        exceptionCallback(OperationError);
        return;
    }

    KeyData result;
    switch (format) {
    case SubtleCrypto::KeyFormat::Jwk:
        result = ecKey.exportJwk();
        break;
    case SubtleCrypto::KeyFormat::Raw: {
        auto raw = ecKey.exportRaw();
        if (raw.hasException()) {
            exceptionCallback(raw.releaseException().code());
            return;
        }
        result = raw.releaseReturnValue();
        break;
    }
    case SubtleCrypto::KeyFormat::Spki: {
        auto spki = ecKey.exportSpki();
        if (spki.hasException()) {
            exceptionCallback(spki.releaseException().code());
            return;
        }
        result = spki.releaseReturnValue();
        break;
    }
    case SubtleCrypto::KeyFormat::Pkcs8: {
        auto pkcs8 = ecKey.exportPkcs8();
        if (pkcs8.hasException()) {
            exceptionCallback(pkcs8.releaseException().code());
            return;
        }
        result = pkcs8.releaseReturnValue();
        break;
    }
    }

    callback(format, WTFMove(result));
}

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
