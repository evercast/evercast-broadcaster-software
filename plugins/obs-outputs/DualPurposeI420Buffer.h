#pragma once

#include <stdint.h>
#include "api/video/video_frame_buffer.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/ref_counted_object.h"

using namespace webrtc;

class DualPurposeI420Buffer : public I420Buffer {
public:
	static rtc::scoped_refptr<DualPurposeI420Buffer> Create(int width, int height) {
		return new rtc::RefCountedObject<DualPurposeI420Buffer>(width, height);
	}

	DualPurposeI420Buffer(int width, int height) :
		I420Buffer(width, height),
		realFrame(nullptr)
	{}
	void setRealFrame(VideoFrameBuffer* frame);
	VideoFrameBuffer* getRealFrame();
private:
	VideoFrameBuffer* realFrame;
};

