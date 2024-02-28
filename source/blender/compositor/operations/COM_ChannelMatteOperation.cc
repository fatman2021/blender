/* SPDX-FileCopyrightText: 2012 Blender Authors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include "COM_ChannelMatteOperation.h"

namespace blender::compositor {

ChannelMatteOperation::ChannelMatteOperation()
{
  add_input_socket(DataType::Color);
  add_output_socket(DataType::Value);

  flags_.can_be_constant = true;
}

void ChannelMatteOperation::init_execution()
{
  limit_range_ = limit_max_ - limit_min_;

  switch (limit_method_) {
    /* SINGLE */
    case 0: {
      /* 123 / RGB / HSV / YUV / YCC */
      const int matte_channel = matte_channel_ - 1;
      const int limit_channel = limit_channel_ - 1;
      ids_[0] = matte_channel;
      ids_[1] = limit_channel;
      ids_[2] = limit_channel;
      break;
    }
    /* MAX */
    case 1: {
      switch (matte_channel_) {
        case 1: {
          ids_[0] = 0;
          ids_[1] = 1;
          ids_[2] = 2;
          break;
        }
        case 2: {
          ids_[0] = 1;
          ids_[1] = 0;
          ids_[2] = 2;
          break;
        }
        case 3: {
          ids_[0] = 2;
          ids_[1] = 0;
          ids_[2] = 1;
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

void ChannelMatteOperation::update_memory_buffer_partial(MemoryBuffer *output,
                                                         const rcti &area,
                                                         Span<MemoryBuffer *> inputs)
{
  for (BuffersIterator<float> it = output->iterate_with(inputs, area); !it.is_end(); ++it) {
    const float *color = it.in(0);

    /* Matte operation. */
    float alpha = color[ids_[0]] - std::max(color[ids_[1]], color[ids_[2]]);

    /* Flip because 0.0 is transparent, not 1.0. */
    alpha = 1.0f - alpha;

    /* Test range. */
    if (alpha > limit_max_) {
      alpha = color[3]; /* Whatever it was prior. */
    }
    else if (alpha < limit_min_) {
      alpha = 0.0f;
    }
    else { /* Blend. */
      alpha = (alpha - limit_min_) / limit_range_;
    }

    /* Store matte(alpha) value in [0] to go with
     * COM_SetAlphaMultiplyOperation and the Value output.
     */

    /* Don't make something that was more transparent less transparent. */
    *it.out = std::min(alpha, color[3]);
  }
}

}  // namespace blender::compositor
