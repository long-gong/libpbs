/**
 * @file pbs_message.h
 * @author Long Gong <long.github@gmail.com>
 * @brief The PBS Message base.
 * @version 0.1
 * @date 2020-07-06
 *
 * @copyright Copyright (c) 2020
 *
 */
#ifndef _PBS_MESSAGE_H_
#define _PBS_MESSAGE_H_

#include <stdint.h>
#include <unistd.h>

namespace libpbs {
/**
 * @brief PBS message types
 */
enum class PbsMessageType : uint8_t {
  ENCODING = 1,
  DECODING = 2,
  ENCODING_HINT = 3
};

class PbsMessage {
 public:
  PbsMessage(PbsMessageType t) : type(t) {}
  virtual ~PbsMessage() {}

  /**
   * @brief Deserialize a PBS message.
   *
   * @param from              Pointer to message buffer.
   * @param msg_sz            Size of message.
   * @return ssize_t          Bytes read.
   * @retval -1               Deserialization failed.
   */
  virtual ssize_t parse(const uint8_t *from, size_t msg_sz) = 0;

  /**
   * @brief Serialize a PBS message.
   *
   * @param to                Pointer to message buffer.
   * @param msg_sz            Size of message.
   * @return ssize_t          Bytes write.
   * @retval -1               Serialization failed.
   */
  virtual ssize_t write(uint8_t *to) const = 0;

  /**
   * @brief The expected size of the serialized message.
   *
   * @return ssize_t          Bytes of serialized message.
   * @retval -1               Get serialized size failed.
   */
  virtual ssize_t serializedSize() const = 0;

  PbsMessageType type;
};
}  // namespace libpbs
#endif  //_PBS_MESSAGE_H_
