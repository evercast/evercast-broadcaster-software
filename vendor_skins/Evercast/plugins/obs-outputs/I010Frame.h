#pragma once

#include "obs.h"
#include "stdint.h"
#include "api/video/i010_buffer.h"

using namespace webrtc;

struct video_data;

class I010Frame : public I010BufferInterface {
public:
	static rtc::scoped_refptr<I010Frame> Create(video_data *src, int width, int height);
	I010Frame(video_data *src, int frameWidth, int frameHeight);
	~I010Frame();

	Type type() const override;

	// PlanarYuv16Buffer overrides
	const uint16_t* DataY() const override;
	const uint16_t* DataU() const override;
	const uint16_t* DataV() const override;

	// Returns the number of steps(in terms of Data*() return type) between
	// successive rows for a given plane.
	int StrideY() const override;
	int StrideU() const override;
	int StrideV() const override;

	// The resolution of the frame in pixels. For formats where some planes are
	// subsampled, this is the highest-resolution plane.
	int width() const override;
	int height() const override;

	// Returns a memory-backed frame buffer in I420 format. If the pixel data is
	// in another format, a conversion will take place. All implementations must
	// provide a fallback to I420 for compatibility with e.g. the internal WebRTC
	// software encoders.
	rtc::scoped_refptr<I420BufferInterface> ToI420() override;

private:
	video_data *frame;
	int frameWidth;
	int frameHeight;
};

