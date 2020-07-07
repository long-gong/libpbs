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
 * Note that we can only tell whether exception I happens within a group/subset,
 * whereas we can tell in which specific bins do exception II happens.
 *
 */
class PbsDecodingHintMessage : public PbsMessage {
 public:
  explicit PbsDecodingHintMessage(const PbsDecodingMessage& message)
      : PbsMessage(PbsMessageType::DECODING_HINT),
        pbs_decoding_message(message),
        exception_i_flags(message.num_groups, 0),
        exception_ii_counts(message.num_groups, 0) {}
  ~PbsDecodingHintMessage() override = default;

  // associated PbsDecodingMessage instance
  const PbsDecodingMessage& pbs_decoding_message;
  // flags for indicating whether exception I happens in each group
  // ONLY two values are allowed (but we will not validate): 0 for "no",
  // 1 for "yes"
  std::vector<uint8_t> exception_i_flags;
  // counters for counting how many bins have exception II in each group
  std::vector<uint32_t> exception_ii_counts;
  // indices of bins (in each group) in which exception II happens
  std::vector<uint32_t> exception_ii_bins;

  ssize_t parse(const uint8_t* from, size_t msg_sz) override {
    utils::BitReader reader(from);
    size_t count_sz = std::ceil(std::log2(pbs_decoding_message.capacity + 1));
    size_t count_bins = 0;
    // reader header
    for (size_t g = 0; g < pbs_decoding_message.num_groups; ++g) {
      exception_i_flags[g] = reader.Read<uint8_t>(1);
      exception_ii_counts[g] = reader.Read<uint32_t>(count_sz);
      count_bins += exception_ii_counts[g];
    }

    exception_ii_bins.resize(count_bins, 0);
    size_t total_bytes = serializedSize();
    if (total_bytes > msg_sz) return -1;
    // read body
    for (auto& bi : exception_ii_bins)
      bi = reader.Read<uint32_t>(pbs_decoding_message.field_sz);

    return total_bytes;
  }

  ssize_t write(uint8_t* to) const override {
    utils::BitWriter writer(to);

    size_t count_sz = std::ceil(std::log2(pbs_decoding_message.capacity + 1));
    // write header
    for (size_t g = 0; g < pbs_decoding_message.num_groups; ++g) {
      writer.Write<uint8_t>(exception_i_flags[g], 1);
      writer.Write<uint32_t>(exception_ii_counts[g], count_sz);
    }

    // write bin ids
    for (uint32_t bi : exception_ii_bins)
      writer.Write<uint32_t>(bi, pbs_decoding_message.field_sz);
    writer.Flush();

    return serializedSize();
  }

  [[nodiscard]] ssize_t serializedSize() const override {
    size_t header_each =
        std::ceil(std::log2(pbs_decoding_message.capacity + 1)) + 1;
    auto header_sz = header_each * pbs_decoding_message.num_groups;
    size_t body_sz = std::accumulate(exception_ii_counts.cbegin(),
                                     exception_ii_counts.cend(), (size_t)0) *
                     pbs_decoding_message.field_sz;
    return utils::Bits2Bytes(header_sz + body_sz);
  }
};
}  // end namespace libpbs

#endif  //_PBS_DECODING_HINT_MESSAGE_H_
