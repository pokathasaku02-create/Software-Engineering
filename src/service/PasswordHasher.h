// Salted password hashing with libsodium's Argon2id (crypto_pwhash).
// Plain-text passwords are never stored or logged; only (salt, hash) are kept.
#pragma once
#include <string>

namespace lms {

class PasswordHasher {
public:
    // fastForTests lowers the Argon2 cost so unit tests run quickly; never use it in production.
    explicit PasswordHasher(bool fastForTests = false);
    std::string generateSalt() const;                                               // 16 random bytes, hex
    std::string hash(const std::string& password, const std::string& salt) const;   // 32 bytes, hex
    bool verify(const std::string& password, const std::string& salt, const std::string& hash) const;
private:
    unsigned long long opsLimit_;
    std::size_t memLimit_;
};

}  // namespace lms
