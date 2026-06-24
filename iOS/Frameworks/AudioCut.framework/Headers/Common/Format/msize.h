#ifndef MSIZE_H
#define MSIZE_H

#include <cmath>

using namespace std;

namespace MediaLibrary {

class MSize
{
public:
    MSize() = default;
    MSize(const float width, const float height);

    float getWidth() const { return width_; }
    float getHeight() const { return height_; }

    void setWidth(const float width) { width_ = width; }
    void setHeight(const float height) { height_ = height; }

    inline bool operator==(const MSize& size) const;
    inline bool operator!=(const MSize& size) const;

    MSize operator*(const float factor) const { return MSize(width_ * factor, height_ * factor); }
    MSize operator/(const float factor) const { return MSize(width_ / factor, height_ / factor); }

    MSize operator+(const MSize& size) const { return MSize(width_ + size.width_, height_ + size.height_); }
    MSize operator-(const MSize& size) const { return MSize(width_ - size.width_, height_ - size.height_); }

private:
    float width_{0.f};
    float height_{0.f};
};

bool MSize::operator==(const MSize &size) const
{
    return (std::fabs(size.getWidth() - width_) < 0.001f && std::fabs(size.getHeight() - height_) < 0.001f);
}

bool MSize::operator!=(const MSize &size) const
{
    return (std::fabs(size.getWidth() - width_) > 0.001f || std::fabs(size.getHeight() - height_) > 0.001f);
}

void scaleSizeByFit(const MSize& inSize, const MSize& maxSize, MSize* outSize);
void scaleSizeByFill(const MSize& inSize, const MSize& maxSize, MSize* outSize);
void scaleSizeByMin(const MSize& inSize, const MSize& maxSize, MSize* outSize);
void scaleSizeByMin(const MSize& inSize, const int minLength, MSize* outSize);
void scaleSizeByMax(const MSize& inSize, const int maxLength, MSize* outSize);

}

#endif // MSIZE_H
