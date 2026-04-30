#include "parser.h"
#include <openssl/md5.h>
#include <openssl/sha.h>
#include <fstream>
#include <iomanip>
#include <sstream>

Hashes computeHashes(const char* path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for hashing");
    }
    
    std::vector<char> data((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
    file.close();
    
    const unsigned char* buf = reinterpret_cast<const unsigned char*>(data.data());
    size_t len = data.size();
    
    Hashes hashes;
    
    unsigned char md5_out[MD5_DIGEST_LENGTH];
    MD5(buf, len, md5_out);
    {
        std::ostringstream oss;
        for (int i = 0; i < MD5_DIGEST_LENGTH; i++) oss << std::hex << std::setw(2) << std::setfill('0') << (int)md5_out[i];
        hashes.md5 = oss.str();
    }
    
    unsigned char sha1_out[SHA_DIGEST_LENGTH];
    SHA1(buf, len, sha1_out);
    {
        std::ostringstream oss;
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) oss << std::hex << std::setw(2) << std::setfill('0') << (int)sha1_out[i];
        hashes.sha1 = oss.str();
    }
    
    unsigned char sha256_out[SHA256_DIGEST_LENGTH];
    SHA256(buf, len, sha256_out);
    {
        std::ostringstream oss;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) oss << std::hex << std::setw(2) << std::setfill('0') << (int)sha256_out[i];
        hashes.sha256 = oss.str();
    }
    
    return hashes;
}