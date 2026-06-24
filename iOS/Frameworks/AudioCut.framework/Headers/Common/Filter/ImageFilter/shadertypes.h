#ifndef SHADERTYPES_H
#define SHADERTYPES_H

#include <simd/simd.h>

typedef enum VertexInputIndex {
    VertexIndexVertex = 0,
    VertexIndexUniform = 1,
} VertexInputIndex;

typedef enum TextureIndex {
    TextureIndexBaseColor = 0,
    TextureIndexBlendColor = 1,
    TextureIndexFilterColor = 2,
    TextureIndexTextureColor = 3,
    TextureIndexCoverColor = 4,
    TextureIndexUniform  = 5,
    TextureIndexLumaColor = 6,
    TextureIndexChromaColor = 7,
    TextureIndexMattingColor = 8,
    TextureIndexIn1Color = 9,
    TextureIndexIn2Color = 10,
    TextureIndexFrameColor = 11,
} TextureIndex;

typedef struct {
    vector_float2 position;

    vector_float4 color;
} Vertex;

typedef struct {
    vector_float2 position;

    vector_float2 textureCoordinate;
} TextureVertex;

typedef struct {
    vector_float2 position;

    vector_float2 textureCoordinate;
    vector_float2 coverTextureCoordinate;
    vector_float2 mattingTextureCoordinate;
} FileVertex;

typedef struct {
    vector_float2 position;

    vector_float2 textTextureCoordinate;
    vector_float2 textureTextureCoordinate;
    vector_float2 coverTextureCoordinate;
    float textEnable;
    float strokeEnable;
    float dropShadowEnable;
    float animationOpacity;
} TextVertex;

typedef struct {
    vector_float2 position;

    vector_float2 textureCoordinate;
    vector_float2 frameTextureCoordinate;
} EffectVertex;

typedef struct {
    matrix_float4x4 mvp;
} Uniform;

typedef struct {
    int maskEnable;
    int maskType;
    int maskPositive;
    float maskCenterX;
    float maskCenterY;
    float maskIntensity;
    float maskRadius;
    float maskAngle;
    float maskHorizontalLength;
    float maskVerticalLength;
    vector_float2 resolution;
} MaskUniform;

typedef struct {
    int wipeEnable;
    int wipeType;
    float wipeCenterX;
    float wipeCenterY;
    float wipeAngle;
    float wipeHorizontalLength;
    float wipeVerticalLength;
    vector_float2 resolution;
} WipeUniform;

typedef struct {
    matrix_float4x4 mvp;

    int type;
    int alphaValid;
    matrix_float3x3 matrix;

    vector_float2 resolution;

    int filterEnable;
    float filterOpacity;

    int adjustEnable;
    float adjustBrightness;
    float adjustContrast;
    float adjustSaturation;
    float adjustExposure;
    float adjustHighlight;
    float adjustShadow;
    float adjustWhiteBalance;
    float adjustTint;
    float adjustHue;
    float adjustRedChannel;
    float adjustGreenChannel;
    float adjustBlueChannel;
    float adjustVignette;
    float adjustSharpen;
    float adjustImageWidthFactor;
    float adjustImageHeightFactor;

    int coverEnable;
    int coverInverted;

    int mattingEnable;

    MaskUniform mask;

    WipeUniform wipeIn;
    WipeUniform wipeOut;
    WipeUniform wipeCycle;

    WipeUniform compositionWipeIn;
    WipeUniform compositionWipeOut;
    WipeUniform compositionWipeCycle;

    float opacity;

    int backgroundColorEnable;
} FileUniform;

typedef struct {
    matrix_float4x4 mvp;

    float textSmoothing;

    vector_float4 backgroundColor;

    vector_float4 fontColor;

    int textureEnable;
    float textureIntensity;

    int strokeEnable;
    vector_float4 strokeColor;
    float strokeOpacity;
    float strokeWidth;

    int dropShadowEnable;
    vector_float4 dropShadowColor;
    float dropShadowOpacity;
    float dropShadowAngle;
    float dropShadowDistance;
    float dropShadowBlur;

    int coverEnable;
    int coverInverted;

    MaskUniform mask;

    WipeUniform wipeIn;
    WipeUniform wipeOut;
    WipeUniform wipeCycle;

    WipeUniform compositionWipeIn;
    WipeUniform compositionWipeOut;
    WipeUniform compositionWipeCycle;
} TextUniform;

typedef struct {
    matrix_float4x4 mvp;

    float aspectRatio;
    float ratio;
    float curveRatio;
} TransitionUniform;

typedef struct {
    matrix_float4x4 mvp;

    vector_float2 resolution;
    float time;

    int shape;
    int positive;
    float opacity;
    float centerX;
    float centerY;
    float radius;
    float intensity;
    float angle;
    float horizontalLength;
    float verticalLength;

    float left;
    float right;
    float top;
    float bottom;

    float sensitivity;

    float scale;
    float speed;
    float adjust;

    float vibration;

    float barrel;
    float brightness;

    float saturation;

    float density;
} FXUniform;

typedef struct {
    matrix_float4x4 mvp;

    vector_float2 resolution;
    float time;

    int horizontal;
    int vertical;

    int shape;
    int positive;
    float opacity;
    float centerX;
    float centerY;
    float radius;
    float intensity;
    float angle;
    float horizontalLength;
    float verticalLength;

    float left;
    float right;
    float top;
    float bottom;

    float sensitivity;

    float scale;
    float speed;
    float adjust;

    float vibration;

    float barrel;
    float brightness;

    float saturation;

    float density;
} EffectUniform;

typedef struct {
    matrix_float4x4 mvp;

    int blendType;
    float intensity;
    float alpha;
} BlendUniform;

typedef struct {
    matrix_float4x4 mvp;

    float alpha;
} RenderUniform;

typedef struct {
    int vertical;
    float blur;
} GaussianblurUniform;

#endif // SHADERTYPES_H
