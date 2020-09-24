#include "DualPurposeI420Buffer.h"

void DualPurposeI420Buffer::setRealFrame(VideoFrameBuffer* frame)
{
	this->realFrame = frame;
}

VideoFrameBuffer* DualPurposeI420Buffer::getRealFrame()
{
	return this->realFrame;
}
