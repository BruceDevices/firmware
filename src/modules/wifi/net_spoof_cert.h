#ifndef __NET_SPOOF_CERT_H__
#define __NET_SPOOF_CERT_H__

/**
 * @file net_spoof_cert.h
 * @brief Self-signed TLS identity for the MITM engine's HTTPS endpoint.
 *
 * This is deliberately a SELF-SIGNED certificate: the T-Embed cannot present a
 * certificate that a victim's browser trusts, so an HTTPS redirect always
 * starts with the browser's certificate warning. After the user continues (or
 * for a client that does not validate certificates, e.g. curl -k or an IoT
 * device), the HTTPS server answers with the same 302/injected page that the
 * plain-HTTP path uses.
 *
 * The private key is embedded on purpose: it is a throwaway identity for this
 * attack tool, not a secret. Regenerate both with:
 *
 *   openssl ecparam -name prime256v1 -genkey -noout -out key.pem
 *   openssl req -new -x509 -key key.pem -out cert.pem -days 3650 \\
 *     -subj "/CN=Vandeta MITM" \\
 *     -addext "subjectAltName=DNS:vandeta.local,DNS:localhost,IP:127.0.0.1,IP:192.168.4.1"
 *
 * Identity: CN=Vandeta MITM, EC P-256, valid 2026-10-23 -> 2036-10-20.
 */

#if !defined(LITE_VERSION)

static const char NETSPOOF_HTTPS_CERT[] = R"CERT(
-----BEGIN CERTIFICATE-----
MIIB2DCCAX6gAwIBAgIUBLqww52hcIaS6WneWzkbL/ByUhgwCgYIKoZIzj0EAwIw
FzEVMBMGA1UEAwwMVmFuZGV0YSBNSVRNMB4XDTI2MTAyMzA2MTkyMloXDTM2MTAy
MDA2MTkyMlowFzEVMBMGA1UEAwwMVmFuZGV0YSBNSVRNMFkwEwYHKoZIzj0CAQYI
KoZIzj0DAQcDQgAEjWE5Q5aE8RqbtQF9JmUyruyp5o7RFAjALkWPntuzWIeMGbmz
p9X0iinz5uLpYzthmDPWqzVVhPgy0C+rMvPhvKOBpzCBpDAdBgNVHQ4EFgQUf1Eg
BvuwpHy1yVwJ3fosNYYkdBAwHwYDVR0jBBgwFoAUf1EgBvuwpHy1yVwJ3fosNYYk
dBAwDwYDVR0TAQH/BAUwAwEB/zAvBgNVHREEKDAmgg12YW5kZXRhLmxvY2Fsggls
b2NhbGhvc3SHBH8AAAGHBMCoBAEwCwYDVR0PBAQDAgWgMBMGA1UdJQQMMAoGCCsG
AQUFBwMBMAoGCCqGSM49BAMCA0gAMEUCIGSYqAu2OI4uQd6SQ+g7kfEBspZZ6XxI
k1uyLnu1WoxuAiEA4LEQrglAElqis0l6SsMgakncx0UwUJvlAhU+5GhEkwY=
-----END CERTIFICATE-----
)CERT";

static const char NETSPOOF_HTTPS_KEY[] = R"KEY(
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIGG7C89WERXcaZB1ZYHeXMehGOgjCFPEfoHFpjSPgP+poAoGCCqGSM49
AwEHoUQDQgAEjWE5Q5aE8RqbtQF9JmUyruyp5o7RFAjALkWPntuzWIeMGbmzp9X0
iinz5uLpYzthmDPWqzVVhPgy0C+rMvPhvA==
-----END EC PRIVATE KEY-----
)KEY";

#endif // !LITE_VERSION
#endif // __NET_SPOOF_CERT_H__
