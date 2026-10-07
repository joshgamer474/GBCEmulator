#version 460 core
#include <flutter/runtime_effect.glsl>
// Authentic GBC by fishku, Copyright (C) 2024-2025, public domain (CC0).
// Flutter port of authentic_gbc_fast.slang, shared.inc and to_lin_fast.slang:
// https://github.com/libretro/slang-shaders/tree/master/handheld/shaders/authentic_gbc
// Coverage intersection adapted from misc/shaders/coverage/coverage.inc.
// Changes: vertex math moved into fragment stage, input linearization fused,
// Flutter uniforms/coordinates, optional raw blend. Rotation is zero; physical
// display subpixel correction is disabled (upstream default). Notch geometry,
// brightness, box smoothing and square-root output match the fast algorithm.
uniform vec2 uSize;
uniform vec2 uSourceSize;
uniform float uPixelRatio;
uniform float uStrength;
uniform float uBrightness;
uniform float uSmoothing;
uniform sampler2D uFrame;
out vec4 fragColor;

float intersection(vec4 a, vec4 b) {
  vec2 coverage = max(min(a.zw,b.zw)-max(a.xy,b.xy),vec2(0.0));
  return coverage.x*coverage.y;
}
float coverage(vec2 origin, vec4 rect1, vec4 rect2, float halfPixel) {
  vec4 square = vec4(vec2(-halfPixel),vec2(halfPixel));
  return intersection(square,origin.xyxy+rect1)
      + intersection(square,origin.xyxy+rect2);
}
vec3 contribution(vec2 offset, vec2 fraction, vec2 texel, vec2 scale,
    vec2 originOffset, vec4 rect1, vec4 rect2, float halfPixel) {
  vec2 origin = (offset-fraction)*scale+originOffset;
  vec3 weights = vec3(
      coverage(origin-vec2(scale.x/3.0,0.0),rect1,rect2,halfPixel),
      coverage(origin,rect1,rect2,halfPixel),
      coverage(origin+vec2(scale.x/3.0,0.0),rect1,rect2,halfPixel));
  vec2 uv = (clamp(texel+offset,vec2(0.0),uSourceSize-1.0)+0.5)/uSourceSize;
  vec3 sampleColor = texture(uFrame,uv).rgb;
  // Equivalent to the upstream to_lin_fast pass, without an intermediate image.
  return weights*sampleColor*sampleColor;
}
void main() {
  vec2 coord = FlutterFragCoord().xy/uSize*uSourceSize;
  vec2 texel = floor(coord);
  vec2 fraction = fract(coord);
  vec2 direction = step(vec2(0.5),fraction)*2.0-1.0;
  vec2 scale = uSize*uPixelRatio/uSourceSize;
  vec2 aperture = scale*mix(vec2(0.296,0.910),vec2(0.75,0.93),uBrightness);
  vec2 notch = scale*mix(vec2(0.115,0.166),vec2(0.29,0.17),uBrightness);
  vec4 rect1 = vec4(vec2(0.0),aperture-vec2(0.0,notch.y));
  vec4 rect2 = vec4(notch.x,aperture.y-notch.y,aperture);
  vec2 originOffset = (scale-aperture)*0.5;
  float minScale = min(scale.x,scale.y);
  float effectiveBlur = 0.7*uSmoothing*minScale*0.5;
  // max avoids inverted clamp bounds when the viewport is smaller than native.
  float halfPixel = 0.5*clamp(1.0+effectiveBlur,1.0,max(1.0,minScale));
  vec3 result = contribution(vec2(0.0),fraction,texel,scale,originOffset,rect1,rect2,halfPixel)
      + contribution(vec2(direction.x,0.0),fraction,texel,scale,originOffset,rect1,rect2,halfPixel)
      + contribution(vec2(0.0,direction.y),fraction,texel,scale,originOffset,rect1,rect2,halfPixel)
      + contribution(direction,fraction,texel,scale,originOffset,rect1,rect2,halfPixel);
  vec3 lcd = sqrt(max(result/(4.0*halfPixel*halfPixel),vec3(0.0)));
  vec3 raw = texture(uFrame,(clamp(texel,vec2(0.0),uSourceSize-1.0)+0.5)/uSourceSize).rgb;
  fragColor = vec4(mix(raw,lcd,uStrength),1.0);
}
