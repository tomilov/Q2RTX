/*
Copyright (C) 2018 Christoph Schied
Copyright (C) 2019, NVIDIA CORPORATION. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef  _TEXTURES_H_
#define  _TEXTURES_H_

#include "constants.h"

#define IMG_WIDTH  (qvk.extent.width)
#define IMG_HEIGHT (qvk.extent.height)
#define IMG_WIDTH_MGPU (qvk.extent.width / qvk.device_count)

#define IMG_WIDTH_GRAD  ((qvk.extent.width + GRAD_DWN - 1) / GRAD_DWN)
#define IMG_HEIGHT_GRAD ((qvk.extent.height + GRAD_DWN - 1) / GRAD_DWN)
#define IMG_WIDTH_GRAD_MGPU  ((qvk.extent.width + GRAD_DWN - 1) / GRAD_DWN / qvk.device_count)

/* These are images that are to be used as render targets and buffers, but not textures. */
#define LIST_IMAGES \
	IMG_DO(PT_VISBUF,                  0, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_ALBEDO,                  1, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_METALLIC,                2, R8G8_UNORM,          rg8,     IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_MOTION,                  3, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_TRANSPARENT,             4, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_TEX_GRADIENTS,           5, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_HF,        6, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PING_LF_SH,    7, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PONG_LF_SH,    8, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PING_LF_COCG,  9, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PONG_LF_COCG, 10, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_ATROUS_PING_HF,      11, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PONG_HF,      12, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PING_SPEC,    13, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PONG_SPEC,    14, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PING_MOMENTS, 15, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_ATROUS_PONG_MOMENTS, 16, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_COLOR,               17, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TEX_GRADIENTS_FWD,   18, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_POS_WS_FWD,          19, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_VISBUF_FWD,          20, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_LF_PING,        21, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_LF_PONG,        22, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_HF_SPEC_PING,   23, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_HF_SPEC_PONG,   24, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(PT_SHADING_POSITION,       25, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(FLAT_COLOR,                26, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(FLAT_MOTION,               27, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(FLAT_VIEW_DEPTH,           28, R32_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(BLOOM_DOWNSCALE_MIP_1,     29, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH / 2,       IMG_HEIGHT / 2 ) \
	IMG_DO(BLOOM_HBLUR,               30, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH / 4,       IMG_HEIGHT / 4 ) \
	IMG_DO(BLOOM_VBLUR,               31, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH / 4,       IMG_HEIGHT / 4 ) \
	IMG_DO(TAA_OUTPUT,                32, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DIRECTION,         33, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DIRECTION2,        34, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_THROUGHPUT,             35, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(HQ_COLOR_INTERLEAVED,      36, R32G32B32A32_SFLOAT, rgba32f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_LF_SH,            37, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_LF_COCG,          38, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_HF,               39, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_COLOR_SPEC,             40, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL,             41, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_GEO_NORMAL2,            42, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_CLUSTER,                43, R16_UINT,            r16ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \

#define LIST_IMAGES_A_B \
	IMG_DO(PT_VIEW_DEPTH_A,           44, R32_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DEPTH_B,           45, R32_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_A,               46, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_B,               47, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_A,     48, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_B,     49, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_A,   50, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_B,   51, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TAA_A,               52, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TAA_B,               53, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_RNG_SEED_A,          54, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_RNG_SEED_B,          55, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_A,  56, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_B,  57, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_A,58, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_B,59, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_A,     60, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_B,     61, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \

#define LIST_IMAGES_B_A \
	IMG_DO(PT_VIEW_DEPTH_B,           44, R32_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_VIEW_DEPTH_A,           45, R32_SFLOAT,          r32f,    IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_B,               46, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(PT_NORMAL_A,               47, R32_UINT,            r32ui,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_B,     48, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_FILTERED_SPEC_A,     49, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_B,   50, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_MOMENTS_HF_A,   51, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TAA_B,               52, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_TAA_A,               53, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_RNG_SEED_B,          54, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_RNG_SEED_A,          55, R32_UINT,            r32ui,   IMG_WIDTH,           IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_B,  56, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_SH_A,  57, R16G16B16A16_SFLOAT, rgba16f, IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_B,58, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_HIST_COLOR_LF_COCG_A,59, R16G16_SFLOAT,       rg16f,   IMG_WIDTH_MGPU,      IMG_HEIGHT     ) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_B,     60, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \
	IMG_DO(ASVGF_GRAD_SMPL_POS_A,     61, R32_UINT,            r32ui,   IMG_WIDTH_GRAD_MGPU, IMG_HEIGHT_GRAD) \

#define NUM_IMAGES 62 /* this really sucks but I don't know how to fix it
                         counting with enum does not work in GLSL */

// todo: make naming consistent!
#define GLOBAL_TEXTURES_TEX_ARR_BINDING_IDX  0
#define BINDING_OFFSET_IMAGES     (1 + GLOBAL_TEXTURES_TEX_ARR_BINDING_IDX)
#define BINDING_OFFSET_TEXTURES   (BINDING_OFFSET_IMAGES + NUM_IMAGES)
#define BINDING_OFFSET_BLUE_NOISE (BINDING_OFFSET_TEXTURES + NUM_IMAGES)
#define BINDING_OFFSET_ENVMAP     (BINDING_OFFSET_BLUE_NOISE + 1)
#define BINDING_OFFSET_PHYSICAL_SKY (BINDING_OFFSET_ENVMAP + 1)
#define BINDING_OFFSET_PHYSICAL_SKY_IMG (BINDING_OFFSET_PHYSICAL_SKY + 1)
#define BINDING_OFFSET_SKY_TRANSMITTANCE (BINDING_OFFSET_PHYSICAL_SKY_IMG + 1)
#define BINDING_OFFSET_SKY_SCATTERING (BINDING_OFFSET_SKY_TRANSMITTANCE + 1)
#define BINDING_OFFSET_SKY_IRRADIANCE (BINDING_OFFSET_SKY_SCATTERING + 1)
#define BINDING_OFFSET_SKY_CLOUDS (BINDING_OFFSET_SKY_IRRADIANCE + 1)
#define BINDING_OFFSET_TERRAIN_ALBEDO (BINDING_OFFSET_SKY_CLOUDS + 1)
#define BINDING_OFFSET_TERRAIN_NORMALS (BINDING_OFFSET_TERRAIN_ALBEDO + 1)
#define BINDING_OFFSET_TERRAIN_DEPTH (BINDING_OFFSET_TERRAIN_NORMALS + 1)
#define BINDING_OFFSET_TERRAIN_SHADOWMAP (BINDING_OFFSET_TERRAIN_DEPTH + 1)


#ifndef VKPT_SHADER
/***************************************************************************/
/* HOST CODE                                                               */
/***************************************************************************/

#if MAX_RIMAGES != NUM_GLOBAL_TEXTUES
#error need to fix the constant here as well
#endif


enum QVK_IMAGES {
#define IMG_DO(_name, ...) \
	VKPT_IMG_##_name,
	LIST_IMAGES
	LIST_IMAGES_A_B
#undef IMG_DO
	NUM_VKPT_IMAGES
};

typedef char compile_time_check_num_images[(NUM_IMAGES == NUM_VKPT_IMAGES)*2-1];

#elif defined(GLOBAL_TEXTURES_DESC_SET_IDX)
/***************************************************************************/
/* SHADER CODE                                                             */
/***************************************************************************/

/* general texture array for world, etc */
layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = GLOBAL_TEXTURES_TEX_ARR_BINDING_IDX
) uniform sampler2D global_texture_descriptors[];

#define SAMPLER_r16ui   usampler2D
#define SAMPLER_r32ui   usampler2D
#define SAMPLER_rg32ui  usampler2D
#define SAMPLER_r32i    isampler2D
#define SAMPLER_r32f    sampler2D
#define SAMPLER_rg32f   sampler2D
#define SAMPLER_rg16f   sampler2D
#define SAMPLER_rgba32f sampler2D
#define SAMPLER_rgba16f sampler2D
#define SAMPLER_rgba8   sampler2D
#define SAMPLER_r8      sampler2D
#define SAMPLER_rg8     sampler2D

#define IMAGE_r16ui   uimage2D
#define IMAGE_r32ui   uimage2D
#define IMAGE_rg32ui  uimage2D
#define IMAGE_r32i    iimage2D
#define IMAGE_r32f    image2D
#define IMAGE_rg32f   image2D
#define IMAGE_rg16f   image2D
#define IMAGE_rgba32f image2D
#define IMAGE_rgba16f image2D
#define IMAGE_rgba8   image2D
#define IMAGE_r8      image2D
#define IMAGE_rg8     image2D

/* framebuffer images */
#define IMG_DO(_name, _binding, _vkformat, _glslformat, _w, _h) \
	layout(set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_IMAGES + _binding, _glslformat) \
	uniform IMAGE_##_glslformat IMG_##_name;
LIST_IMAGES
LIST_IMAGES_A_B
#undef IMG_DO

/* framebuffer textures */
#define IMG_DO(_name, _binding, _vkformat, _glslformat, _w, _h) \
	layout(set = GLOBAL_TEXTURES_DESC_SET_IDX, binding = BINDING_OFFSET_TEXTURES + _binding) \
	uniform SAMPLER_##_glslformat TEX_##_name;
LIST_IMAGES
LIST_IMAGES_A_B
#undef IMG_DO

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_BLUE_NOISE
) uniform sampler2DArray TEX_BLUE_NOISE;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_ENVMAP
) uniform samplerCube TEX_ENVMAP;

layout(
    set = GLOBAL_TEXTURES_DESC_SET_IDX,
    binding = BINDING_OFFSET_PHYSICAL_SKY
) uniform samplerCube TEX_PHYSICAL_SKY;

layout(
    set = GLOBAL_TEXTURES_DESC_SET_IDX,
    binding = BINDING_OFFSET_PHYSICAL_SKY_IMG,
    rgba16f
) uniform imageCube IMG_PHYSICAL_SKY;

//#precomputed_sky begin

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_SKY_TRANSMITTANCE
) uniform sampler2D TEX_SKY_TRANSMITTANCE;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_SKY_SCATTERING
) uniform sampler3D TEX_SKY_SCATTERING;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_SKY_IRRADIANCE
) uniform sampler2D TEX_SKY_IRRADIANCE;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_SKY_CLOUDS
) uniform sampler3D TEX_SKY_CLOUDS;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_TERRAIN_ALBEDO
) uniform samplerCube IMG_TERRAIN_ALBEDO;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_TERRAIN_NORMALS
) uniform samplerCube IMG_TERRAIN_NORMALS;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_TERRAIN_DEPTH
) uniform samplerCube IMG_TERRAIN_DEPTH;

layout(
	set = GLOBAL_TEXTURES_DESC_SET_IDX,
	binding = BINDING_OFFSET_TERRAIN_SHADOWMAP
) uniform sampler2D TEX_TERRAIN_SHADOWMAP;
//#precomputed_sky end

vec4
global_texture(uint idx, vec2 tex_coord)
{
	if(idx >= NUM_GLOBAL_TEXTUES)
		return vec4(1, 0, 1, 0);
	return texture(global_texture_descriptors[nonuniformEXT(idx)], tex_coord);
}

vec4
global_textureLod(uint idx, vec2 tex_coord, float lod)
{
	if(idx >= NUM_GLOBAL_TEXTUES)
		return vec4(1, 1, 0, 0);
	return textureLod(global_texture_descriptors[nonuniformEXT(idx)], tex_coord, lod);
}

vec4
global_textureGrad(uint idx, vec2 tex_coord, vec2 d_x, vec2 d_y)
{
	if(idx >= NUM_GLOBAL_TEXTUES)
		return vec4(1, 1, 0, 0);
	return textureGrad(global_texture_descriptors[nonuniformEXT(idx)], tex_coord, d_x, d_y);
}

ivec2
global_textureSize(uint idx, int level)
{
	if(idx >= NUM_GLOBAL_TEXTUES)
		return ivec2(0);
	return textureSize(global_texture_descriptors[nonuniformEXT(idx)], level);
}


#endif


#endif /*_TEXTURES_H_*/
// vim: shiftwidth=4 noexpandtab tabstop=4 cindent
