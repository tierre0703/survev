// GENERATED FILE - do not edit. Run tools/gen_reference.mjs to regenerate.
#pragma once
#include <string>
#include <vector>
#include <utility>

namespace surv_reference {

inline const std::vector<std::pair<std::string, std::string>>& fixtures() {
    static const std::vector<std::pair<std::string, std::string>> data = {
        {"primitives", "575df6df7d01d89411ffffffffa10000807fac39908142f163ccd1cad8d8de00d08653d9d8de000000"},
        {"vecs", "20191032dada0000b0c00000c840"},
        {"string-fixed-align", "706c617965723100000000000000000007"},
        {"join", "04040000746f6b5f6162633132330054657374506c617965720000000000004b4fb7a4961240c410"},
        {"joined", "022a0005103904"},
        {"input", "07ecbfbf80ff0308380102034901"},
        {"edit", "05020000fc000004"},
        {"map", "6d61696e000000000000000000000000000000000000000040e20100000200020400020001030002800200058007000a01546f776e00008000800100001900325e5d22010140008000c0000001332211000000003f0000803e83"},
        {"update-full", "2f8203000100020003000100010500200370068c01e93535d5557f45488b65528f4ca40000000500831570ab1e00000700ec2f0fdd009202a816000100002841000000000019001940062003010500010054657374506c6179657200492d05010500200370068c0102841220e35f2c01e09dea5405000709"},
        {"kill", "007f75490180018001c080"},
        {"gameover", "010201010105007800030164003300"},
        {"emote", "800200058804"},
        {"pickup", "047f0500"},
        {"dropitem", "7f0500"},
        {"perkmoderoleselect", "c502"},
        {"roleannouncement", "01000200c506"},
        {"alivecounts", "0404020301"},
        {"spectate", "01"},
        {"msgstream-join", "0104040000746f6b00426f6200000000000000000000000000484fb7a4960200"},
    };
    return data;
}

} // namespace surv_reference
