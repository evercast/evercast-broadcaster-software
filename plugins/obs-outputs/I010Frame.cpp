#include "I010Frame.h"
#include "rtc_base/ref_counted_object.h"
#include "DualPurposeI420Buffer.h"
#include "media-io/video-io.h"
#include <api\video\i420_buffer.h>
#include <libyuv\convert.h>

rtc::scoped_refptr<I010Frame> I010Frame::Create(video_data* src, int width, int height)
{
	struct video_data* frame = (struct video_data*)bzalloc(sizeof(struct video_data));
	frame->timestamp = src->timestamp;
	frame->info = src->info;
	memcpy(frame->linesize, src->linesize, sizeof(uint32_t) * MAX_AV_PLANES);

	int size = width * height * sizeof(uint16_t) * 1.5;
	frame->data[0] = (uint8_t*)bmalloc(size);
	memcpy(frame->data[0], src->data[0], size);
	frame->data[1] = frame->data[0] + (src->data[1] - src->data[0]);
	frame->data[2] = frame->data[0] + (src->data[2] - src->data[0]);

	/*
	for (int i = 0; i < size / 3; i++) {
		frame->data[1][i * 2] = 255;
		frame->data[1][i * 2 + 1] = 0;
	}
	*/

	// NOTE: This is the wrong place to do this; it is compensating for a particular big-endian file.
	uint16_t* data = (uint16_t*)frame->data[0];
	for (int i = 0; i < size / sizeof(uint16_t); i++) {
		data[i] = (data[i] >> 8) | (data[i] << 8);
	}

	/*
	int end = height / 4;
	for (long i = 0; i < end; i++) {
		memcpy((frame->data[1] + i * frame->linesize[1]), (frame->data[1] + (i * 2 + 1) * frame->linesize[1]), frame->linesize[1]);
		memcpy((frame->data[2] + i * frame->linesize[2]), (frame->data[2] + (i * 2 + 1) * frame->linesize[2]), frame->linesize[2]);
	}
	*/

	return new rtc::RefCountedObject<I010Frame>(frame, width, height);
}

I010Frame::I010Frame(video_data* src, int srcWidth, int srcHeight) :
	frame(src),
	frameWidth(srcWidth),
	frameHeight(srcHeight)
{}

I010Frame::~I010Frame()
{
	bfree(frame->data[0]);
	bfree(frame);
}

webrtc::VideoFrameBuffer::Type I010Frame::type() const {
	return webrtc::VideoFrameBuffer::Type::kI010;
};

const uint16_t* I010Frame::DataY() const {
	return (uint16_t*)frame->data[0];
}

const uint16_t* I010Frame::DataU() const {
	return (uint16_t*)frame->data[1];
}

const uint16_t* I010Frame::DataV() const {
	return (uint16_t*)frame->data[2];
}

int I010Frame::StrideY() const {
	return frame->linesize[0] / sizeof(uint16_t);
}

int I010Frame::StrideU() const {
	return frame->linesize[1] / sizeof(uint16_t);
}

int I010Frame::StrideV() const {
	return frame->linesize[2] / sizeof(uint16_t);
}

int I010Frame::width() const {
	return frameWidth;
}

int I010Frame::height() const {
	return frameHeight;
}

rtc::scoped_refptr<webrtc::I420BufferInterface> I010Frame::ToI420() {
	rtc::scoped_refptr<DualPurposeI420Buffer> i420_buffer =
		DualPurposeI420Buffer::Create(width(), height());
	i420_buffer->setRealFrame(this);

	/*
	libyuv::I010ToI420(DataY(), StrideY(), DataU(), StrideU(), DataV(), StrideV(),
		i420_buffer->MutableDataY(), i420_buffer->StrideY() / 2,
		i420_buffer->MutableDataU(), StrideU() / 2,
		i420_buffer->MutableDataV(), StrideV() / 2,
		width(), height());*/
	return i420_buffer;
}

