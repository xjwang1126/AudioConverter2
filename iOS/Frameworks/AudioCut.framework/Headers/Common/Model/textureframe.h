#ifndef TEXTUREFRAME_H
#define TEXTUREFRAME_H

#include "frame.h"
#include "Filter/ImageFilter/matrix.h"

namespace MediaLibrary {

class Frame;

class TextureFrame : public Frame
{
public:
    TextureFrame();
    TextureFrame(const TextureFrame&) = delete;
    virtual ~TextureFrame();

    virtual void setFrame(void* frame) override { frame_ = frame; }
    virtual void setSize(const MSize& size) override { size_ = size; }
    virtual void setTime(const int64_t time, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;
    virtual void setDuration(const int64_t duration, const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) override;

    virtual void* getFrame() const override { return frame_; }
    virtual MSize getSize() const override { return size_; }
    virtual int64_t getTime(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;
    virtual int64_t getDuration(const TimeBase& timeBase = {1, SECOND_TO_MILLISECOND_UNIT}) const override;

    void setPosition(const float* value);
    void setTextureCoordinate(const float* value);
    void setProjectionMatrix(const Matrix& value);
    void setModelViewMatrix(const Matrix& value);

    const float* getPosition() const { return static_cast<const float*>(position); }
    const float* getTextureCoordinate() const { return static_cast<const float*>(textureCoordinate); }
    const Matrix& getProjectionMatrix() const { return projectionMatrix; }
    const Matrix& getModelViewMatrix() const { return modelViewMatrix; }

private:
    virtual Frame* copy() const override;
    virtual Frame* convert(const FrameType& frameType) const override;
    virtual Frame* resize(const MSize& size) const override;

    virtual void copy(Frame* frame) override;

private:
    void* frame_{nullptr};
    MSize size_;
    int64_t time_{-1};
    int64_t duration_{0};

    float position[8];
    float textureCoordinate[8];
    Matrix projectionMatrix;
    Matrix modelViewMatrix;
};

}

#endif // TEXTUREFRAME_H
