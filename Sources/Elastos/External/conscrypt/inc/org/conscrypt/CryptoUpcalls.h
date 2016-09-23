
#ifndef __ORG_CONSCRYPT_CRYPTOUPCALLS_H__
#define __ORG_CONSCRYPT_CRYPTOUPCALLS_H__

#include <elastos/coredef.h>
#include <Elastos.CoreLibrary.Extensions.h>
#include <Elastos.CoreLibrary.Security.h>

using Elastos::Security::IProvider;
using Elastos::Security::IPrivateKey;

namespace Org {
namespace Conscrypt {

/**
 * Provides a place where NativeCrypto can call back up to do Java language
 * calls to work on delegated key types from native code.
 */
class CryptoUpcalls
{
public:
    /**
     * Finds the first provider which provides {@code algorithm} but is not from
     * the same ClassLoader as ours.
     */
    static CARAPI GetExternalProvider(
        /* [in] */ const String& algorithm,
        /* [out] */ IProvider** result);

    static CARAPI RawSignDigestWithPrivateKey(
        /* [in] */ IPrivateKey* key,
        /* [in] */ ArrayOf<Byte>* message,
        /* [out, callee] */ ArrayOf<Byte>** result);

    static CARAPI RawCipherWithPrivateKey(
        /* [in] */ IPrivateKey* key,
        /* [in] */ Boolean encrypt,
        /* [in] */ ArrayOf<Byte>* input,
        /* [out, callee] */ ArrayOf<Byte>** result);

private:
    static const String RSA_CRYPTO_ALGORITHM; // = "RSA/ECB/PKCS1Padding";
};

} // namespace Conscrypt
} // namespace Org

#endif //__ORG_CONSCRYPT_CRYPTOUPCALLS_H__
