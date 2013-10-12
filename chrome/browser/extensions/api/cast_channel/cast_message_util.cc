// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/cast_channel/cast_message_util.h"

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/extensions/api/cast_channel/cast_channel.pb.h"
#include "chrome/common/extensions/api/cast_channel.h"

namespace extensions {
namespace api {
namespace cast_channel {

bool MessageInfoToCastMessage(const MessageInfo& message,
                              CastMessage* message_proto) {
  DCHECK(message_proto);
  message_proto->set_protocol_version(CastMessage_ProtocolVersion_CASTV2_1_0);
  message_proto->set_source_id(message.source_id);
  message_proto->set_destination_id(message.destination_id);
  message_proto->set_namespace_(message.namespace_);
  // Determine the type of the base::Value and set the message payload
  // appropriately.
  std::string data;
  base::BinaryValue* real_value;
  switch (message.data->GetType()) {
  // JS string
  case base::Value::TYPE_STRING:
    if (message.data->GetAsString(&data)) {
      message_proto->set_payload_type(CastMessage_PayloadType_STRING);
      message_proto->set_payload_utf8(data);
    }
    break;
  // JS ArrayBuffer
  case base::Value::TYPE_BINARY:
    real_value = static_cast<base::BinaryValue*>(message.data.get());
    if (real_value->GetBuffer()) {
      message_proto->set_payload_type(CastMessage_PayloadType_BINARY);
      message_proto->set_payload_binary(real_value->GetBuffer(),
                                        real_value->GetSize());
    }
    break;
  default:
    // Unknown value type.  message_proto will remain uninitialized because
    // payload_type is unset.
    break;
  }
  return message_proto->IsInitialized();
}

bool CastMessageToMessageInfo(const CastMessage& message_proto,
                              MessageInfo* message) {
  DCHECK(message);
  message->source_id = message_proto.source_id();
  message->destination_id = message_proto.destination_id();
  message->namespace_ = message_proto.namespace_();
  // Determine the type of the payload and fill base::Value appropriately.
  scoped_ptr<base::Value> value;
  switch (message_proto.payload_type()) {
  case CastMessage_PayloadType_STRING:
    if (message_proto.has_payload_utf8())
      value.reset(new base::StringValue(message_proto.payload_utf8()));
    break;
  case CastMessage_PayloadType_BINARY:
    if (message_proto.has_payload_binary())
      value.reset(base::BinaryValue::CreateWithCopiedBuffer(
        message_proto.payload_binary().data(),
        message_proto.payload_binary().size()));
    break;
  default:
    // Unknown payload type. value will remain unset.
    break;
  }
  if (value.get()) {
    DCHECK(!message->data.get());
    message->data.reset(value.release());
    return true;
  } else {
    return false;
  }
}

const std::string MessageProtoToString(const CastMessage& message_proto) {
  std::string out("{");
  out += "namespace = " + message_proto.namespace_();
  out += ", sourceId = " + message_proto.source_id();
  out += ", destId = " + message_proto.destination_id();
  out += ", type = " + base::IntToString(message_proto.payload_type());
  out += ", str = \"" + message_proto.payload_utf8() + "\"}";
  return out;
}

}  // namespace cast_channel
}  // namespace api
}  // namespace extensions
