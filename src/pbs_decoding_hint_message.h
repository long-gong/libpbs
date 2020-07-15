#ifndef _PBS_DECODING_HINT_MESSAGE_H_
#define _PBS_DECODING_HINT_MESSAGE_H_

#include <numeric>

#include "pbs_decoding_message.h"
#include "pbs_message.h"

namespace libpbs {

/**
 * @brief The PbsDecodingHintMessage class.
 *
 * This message should be used together with a PbsDecodingMessage instance.
 * It tells whether exceptions (exception I or II) happens in the corresponding
 * PbsDecodingMessage instance.
 *
 * Note that there is no need for the hint of BCH decoding failure.
 *
 */
class PbsDecodingHintMessage : public PbsMessage {
 public:
  explicit PbsDecodingHintMessage(size_t g)
      : PbsMessage(PbsMessageType::DECODING_HINT),
        bits_each_id(utils::CeilLog2(g)), groups_with_exceptions() {}
  ~PbsDecodingHintMessage() override = default;
  size_t bits_each_id;
  std::vector<uint32_t> groups_with_exceptions;

  ssize_t parse(const uint8_t *from, size_t msg_sz) override {
    if (msg_sz == 0) return 0;
    utils::BitReader reader(from);
    size_t count = msg_sz * 8 / bits_each_id;

    groups_with_exceptions.resize(count, 0);
    // read body
    for (auto &bi : groups_with_exceptions)
      bi = reader.Read<uint32_t>(bits_each_id);
    return msg_sz;
  }

  void addGroupId(uint32_t gid) {
    groups_with_exceptions.push_back(gid);
  }

  ssize_t write(uint8_t *to) const override {
    utils::BitWriter writer(to);
    // write to buffer
    for (uint32_t gid: groups_with_exceptions) {
      writer.Write<uint32_t>(gid, bits_each_id);
    }
    writer.Flush();
    return serializedSize();
  }

  [[nodiscard]] ssize_t serializedSize() const override {
    return utils::Bits2Bytes(bits_each_id * groups_with_exceptions.size());
  }
};
}  // end namespace libpbs

#endif  //_PBS_DECODING_HINT_MESSAGE_H_
