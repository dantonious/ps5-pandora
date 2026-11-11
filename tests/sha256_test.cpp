#include <algorithm>

#include "pandora/sha256.hpp"
#include "support/check.hpp"

int main() {
    // The published vectors from FIPS 180-4 / RFC 6234.
    EXPECT_STR_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                  pandora::sha256_hex(""));
    EXPECT_STR_EQ("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
                  pandora::sha256_hex("abc"));
    EXPECT_STR_EQ("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
                  pandora::sha256_hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"));

    // The interesting boundary is the block padding: 55, 56, 63, 64 and 65 bytes
    // all take different branches, so all of them are checked.
    for (const std::size_t length :
         {std::size_t{55}, std::size_t{56}, std::size_t{57}, std::size_t{63}, std::size_t{64},
          std::size_t{65}, std::size_t{119}, std::size_t{120}}) {
        pandora::Sha256 one_shot;
        one_shot.update(std::string(length, 'a'));
        const std::string expected = one_shot.hex_digest();

        // The same bytes delivered in awkward chunk sizes must agree.
        pandora::Sha256 streamed;
        const std::string data(length, 'a');
        std::size_t offset = 0;
        for (std::size_t chunk = 1; offset < data.size(); ++chunk) {
            const std::size_t take = std::min(chunk, data.size() - offset);
            streamed.update(data.data() + offset, take);
            offset += take;
        }
        EXPECT_STR_EQ(expected, streamed.hex_digest());
    }

    // A million 'a', the classic long-message vector, built incrementally.
    pandora::Sha256 million;
    const std::string block(1000, 'a');
    for (int i = 0; i < 1000; ++i) {
        million.update(block);
    }
    EXPECT_STR_EQ("cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0",
                  million.hex_digest());

    return pandora::test::summary("sha256");
}
