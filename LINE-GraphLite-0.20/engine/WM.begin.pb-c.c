/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: WM.begin.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "WM.begin.pb-c.h"
void   wm__begin__init
                     (Wm__Begin         *message)
{
  static Wm__Begin init_value = WM__BEGIN__INIT;
  *message = init_value;
}
size_t wm__begin__get_packed_size
                     (const Wm__Begin *message)
{
  assert(message->base.descriptor == &wm__begin__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t wm__begin__pack
                     (const Wm__Begin *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &wm__begin__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t wm__begin__pack_to_buffer
                     (const Wm__Begin *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &wm__begin__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Wm__Begin *
       wm__begin__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Wm__Begin *)
     protobuf_c_message_unpack (&wm__begin__descriptor,
                                allocator, len, data);
}
void   wm__begin__free_unpacked
                     (Wm__Begin *message,
                      ProtobufCAllocator *allocator)
{
  assert(message->base.descriptor == &wm__begin__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor wm__begin__field_descriptors[3] =
{
  {
    "s_id",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(Wm__Begin, s_id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "d_id",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(Wm__Begin, d_id),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "state",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(Wm__Begin, state),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned wm__begin__field_indices_by_name[] = {
  1,   /* field[1] = d_id */
  0,   /* field[0] = s_id */
  2,   /* field[2] = state */
};
static const ProtobufCIntRange wm__begin__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor wm__begin__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "wm.begin",
  "Begin",
  "Wm__Begin",
  "wm",
  sizeof(Wm__Begin),
  3,
  wm__begin__field_descriptors,
  wm__begin__field_indices_by_name,
  1,  wm__begin__number_ranges,
  (ProtobufCMessageInit) wm__begin__init,
  NULL,NULL,NULL    /* reserved[123] */
};
