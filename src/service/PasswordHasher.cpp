#include "PasswordHasher.h"

#include <sodium.h>

#include <vector>

#include "../domain/Exceptions.h"

namespace lms {
namespace {

constexpr std::size_t kHashBytes = 32;

std::string toHex(const unsigned char* data, std::size_t len) {
    std::vector<char> buf(len * 2 + 1);
    sodium_bin2hex(buf.data(), buf.size(), data, len);
    return std::string(buf.data());
}

bool fromHex(const std::string& hex, unsigned char* out, std::size_t expected) {
    std::size_t written = 0;
    return sodium_hex2bin(out, expected, hex.c_str(), hex.size(), nullptr, &written, nullptr) == 0 &&
           written == expected;
}

}  // namespace

PasswordHasher::PasswordHasher(bool fastForTests) {
    if (sodium_init() < 0) throw LmsException("Could not initialise the cryptography library.");
    opsLimit_ = fastForTests ? crypto_pwhash_OPSLIMIT_MIN : crypto_pwhash_OPSLIMIT_INTERACTIVE;
    memLimit_ = fastForTests ? crypto_pwhash_MEMLIMIT_MIN : crypto_pwhash_MEMLIMIT_INTERACTIVE;
}

std::string PasswordHasher::generateSalt() const {
    unsigned char salt[crypto_pwhash_SALTBYTES];
    randombytes_buf(salt, sizeof salt);
    return toHex(salt, sizeof salt);
}

std::string PasswordHasher::hash(const std::string& password, const std::string& salt) const {
    unsigned char saltBytes[crypto_pwhash_SALTBYTES];
    if (!fromHex(salt, saltBytes, sizeof saltBytes)) throw LmsException("Stored salt is corrupt.");
    unsigned char out[kHashBytes];
    if (crypto_pwhash(out, sizeof out, password.c_str(), password.size(), saltBytes, opsLimit_, memLimit_,
                      crypto_pwhash_ALG_ARGON2ID13) != 0)
        throw LmsException("Password hashing failed (out of memory?).");
    return toHex(out, sizeof out);
}

bool PasswordHasher::verify(const std::string& password, const std::string& salt, const std::string& expected) const {
    unsigned char expectedBytes[kHashBytes];
    if (!fromHex(expected, expectedBytes, sizeof expectedBytes)) return false;
    unsigned char saltBytes[crypto_pwhash_SALTBYTES];
    if (!fromHex(salt, saltBytes, sizeof saltBytes)) return false;
    unsigned char out[kHashBytes];
    if (crypto_pwhash(out, sizeof out, password.c_str(), password.size(), saltBytes, opsLimit_, memLimit_,
                      crypto_pwhash_ALG_ARGON2ID13) != 0)
        return false;
    return sodium_memcmp(out, expectedBytes, sizeof out) == 0;   // constant-time comparison
}

}  // namespace lms
