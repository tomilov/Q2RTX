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

#include "path_tracer.h"
#include "utils.glsl"

#define RAY_GEN_DESCRIPTOR_SET_IDX 0
layout(set = RAY_GEN_DESCRIPTOR_SET_IDX, binding = 0)
uniform accelerationStructureNV topLevelAS;

#define GLOBAL_TEXTURES_DESC_SET_IDX 2
#include "global_textures.h"

#define VERTEX_BUFFER_DESC_SET_IDX 3
#define VERTEX_READONLY 1
#include "vertex_buffer.h"

#include "read_visbuf.glsl"
#include "asvgf.glsl"
#include "brdf.glsl"
#include "water.glsl"

#define DESATURATE_ENVIRONMENT_MAP 1

#define RNG_PRIMARY_OFF_X   0
#define RNG_PRIMARY_OFF_Y   1

#define RNG_NEE_LH(bounce)                (2 + 0 + 9 * bounce)
#define RNG_NEE_TRI_X(bounce)             (2 + 1 + 9 * bounce)
#define RNG_NEE_TRI_Y(bounce)             (2 + 2 + 9 * bounce)
#define RNG_NEE_STATIC_DYNAMIC(bounce)    (2 + 3 + 9 * bounce)
#define RNG_BRDF_X(bounce)                (2 + 4 + 9 * bounce)
#define RNG_BRDF_Y(bounce)                (2 + 5 + 9 * bounce)
#define RNG_BRDF_FRESNEL(bounce)          (2 + 6 + 9 * bounce)
#define RNG_SUNLIGHT_X(bounce)			  (2 + 7 + 9 * bounce)
#define RNG_SUNLIGHT_Y(bounce)			  (2 + 8 + 9 * bounce)

#define PRIMARY_RAY_CULL_MASK        (AS_FLAG_EVERYTHING & ~(AS_FLAG_VIEWER_MODELS))
#define REFLECTION_RAY_CULL_MASK     (AS_FLAG_OPAQUE_STATIC | AS_FLAG_OPAQUE_DYNAMIC | AS_FLAG_PARTICLES | AS_FLAG_EXPLOSIONS | AS_FLAG_SKY)
#define BOUNCE_RAY_CULL_MASK         (AS_FLAG_OPAQUE_STATIC | AS_FLAG_OPAQUE_DYNAMIC | AS_FLAG_SKY)
#define SHADOW_RAY_CULL_MASK         (AS_FLAG_OPAQUE_STATIC | AS_FLAG_OPAQUE_DYNAMIC)

/* no BRDF sampling in last bounce */
#define NUM_RNG_PER_FRAME (RNG_NEE_STATIC_DYNAMIC(1) + 1)

#define BOUNCE_SPECULAR 1

#define DYNAMIC_LIGHT_INTENSITY_DIFFUSE 20

#define MAX_OUTPUT_VALUE 1000

#define RT_PAYLOAD_SHADOW  0
#define RT_PAYLOAD_BRDF 1
layout(location = RT_PAYLOAD_SHADOW) rayPayloadNV RayPayloadShadow ray_payload_shadow;
layout(location = RT_PAYLOAD_BRDF) rayPayloadNV RayPayload ray_payload_brdf;

uint rng_seed;

struct Ray {
	vec3 origin, direction;
	float t_min, t_max;
};

vec3
env_map(vec3 direction, bool remove_sun)
{
	direction = (global_ubo.environment_rotation_matrix * vec4(direction, 0)).xyz;

    vec3 envmap = vec3(0);
    if (global_ubo.environment_type == ENVIRONMENT_DYNAMIC)
    {
	    envmap = textureLod(TEX_PHYSICAL_SKY, direction.xzy, 0).rgb;

	    if(remove_sun)
	    {
			// roughly remove the sun from the env map
			envmap = min(envmap, vec3((1 - dot(direction, global_ubo.sun_direction_envmap)) * 200));
		}
	}
    else if (global_ubo.environment_type == ENVIRONMENT_STATIC)
    {
        envmap = textureLod(TEX_ENVMAP, direction.xzy, 0).rgb;
#if DESATURATE_ENVIRONMENT_MAP
        float avg = (envmap.x + envmap.y + envmap.z) / 3.0;
        envmap = mix(envmap, avg.xxx, 0.1) * 0.5;
#endif
    }
	return envmap;
}

// depends on env_map
#include "light_lists.h"

ivec2 get_image_position()
{
	ivec2 pos;

	bool is_even_checkerboard = push_constants.gpu_index == 0 || push_constants.gpu_index < 0 && gl_LaunchIDNV.z == 0;
	if(global_ubo.pt_swap_checkerboard != 0)
		is_even_checkerboard = !is_even_checkerboard;

	if (is_even_checkerboard) {
		pos.x = int(gl_LaunchIDNV.x * 2) + int(gl_LaunchIDNV.y & 1);
	} else {
		pos.x = int(gl_LaunchIDNV.x * 2 + 1) - int(gl_LaunchIDNV.y & 1);
	}

	pos.y = int(gl_LaunchIDNV.y);
	return pos;
}

ivec2 get_image_size()
{
	return ivec2(global_ubo.width, global_ubo.height);
}

bool
found_intersection(RayPayload rp)
{
	return rp.instance_prim != ~0u;
}

bool
is_sky(RayPayload rp)
{
	return (rp.instance_prim & INSTANCE_SKY_FLAG) != 0;
}

bool
is_dynamic_instance(RayPayload pay_load)
{
	return (pay_load.instance_prim & INSTANCE_DYNAMIC_FLAG) > 0;
}

uint
get_primitive(RayPayload pay_load)
{
	return pay_load.instance_prim & PRIM_ID_MASK;
}

Triangle
get_hit_triangle(RayPayload rp)
{
	uint prim = get_primitive(rp);

	return is_dynamic_instance(rp)
		?  get_instanced_triangle(prim)
		:  get_bsp_triangle(prim);
}

vec3
get_hit_barycentric(RayPayload rp)
{
	vec3 bary;
	bary.yz = rp.barycentric;
	bary.x  = 1.0 - bary.y - bary.z;
	return bary;
}

float
get_rng(uint idx)
{
	uvec3 p = uvec3(rng_seed, rng_seed >> 10, rng_seed >> 20);
	p.z = (p.z + idx);
	p &= uvec3(BLUE_NOISE_RES - 1, BLUE_NOISE_RES - 1, NUM_BLUE_NOISE_TEX - 1);

	return min(texelFetch(TEX_BLUE_NOISE, ivec3(p), 0).r, 0.9999999999999);
	//return fract(vec2(get_rng_uint(idx)) / vec2(0xffffffffu));
}

bool
is_water(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_WATER;
}

bool
is_slime(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_SLIME;
}

bool
is_lava(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_LAVA;
}

bool
is_glass(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_GLASS;
}

bool
is_transparent(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_TRANSPARENT;
}

bool
is_chrome(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_CHROME;
}

bool
is_screen(uint material)
{
	return (material & MATERIAL_KIND_MASK) == MATERIAL_KIND_SCREEN;
}

vec3
correct_albedo(vec3 albedo)
{
    return max(vec3(0), pow(albedo, vec3(ALBEDO_TRANSFORM_POWER)) * ALBEDO_TRANSFORM_SCALE + vec3(ALBEDO_TRANSFORM_BIAS));
}

vec3
correct_emissive(uint material_id, vec3 emissive)
{
	return max(vec3(0), emissive.rgb + vec3(EMISSIVE_TRANSFORM_BIAS));
}

void
trace_ray(Ray ray, bool cull_back_faces, int instance_mask)
{
	uint rayFlags = 0;
	if(cull_back_faces)
		rayFlags |=gl_RayFlagsCullBackFacingTrianglesNV;
	
	ray_payload_brdf.transparency = uvec2(0);
    ray_payload_brdf.hit_distance = 0;
    ray_payload_brdf.max_transparent_distance = 0;

	traceNV( topLevelAS, rayFlags, instance_mask,
			SBT_RCHIT_OPAQUE /*sbtRecordOffset*/, 0 /*sbtRecordStride*/, SBT_RMISS_PATH_TRACER /*missIndex*/,
			ray.origin, ray.t_min, ray.direction, ray.t_max, RT_PAYLOAD_BRDF);
}

Ray get_shadow_ray(vec3 p1, vec3 p2, float tmin)
{
	vec3 l = p2 - p1;
	float dist = length(l);
	l /= dist;

	Ray ray;
	ray.origin = p1 + l * tmin;
	ray.t_min = 0;
	ray.t_max = dist - tmin - 0.01;
	ray.direction = l;

	return ray;
}

float
trace_shadow_ray(Ray ray, int cull_mask)
{
	const uint rayFlags = gl_RayFlagsOpaqueNV | gl_RayFlagsTerminateOnFirstHitNV | gl_RayFlagsCullBackFacingTrianglesNV;

	ray_payload_shadow.missed = 0;

	traceNV( topLevelAS, rayFlags, cull_mask,
			SBT_RCHIT_EMPTY /*sbtRecordOffset*/, 0 /*sbtRecordStride*/, SBT_RMISS_SHADOW /*missIndex*/,
			ray.origin, ray.t_min, ray.direction, ray.t_max, RT_PAYLOAD_SHADOW);

	return float(ray_payload_shadow.missed);
}

vec3
trace_caustic_ray(Ray ray, int surface_medium)
{
	ray_payload_brdf.hit_distance = -1;

	traceNV(topLevelAS, gl_RayFlagsCullBackFacingTrianglesNV, AS_FLAG_TRANSPARENT,
		SBT_RCHIT_OPAQUE, 0, SBT_RMISS_PATH_TRACER,
			ray.origin, ray.t_min, ray.direction, ray.t_max, RT_PAYLOAD_BRDF);

	float extinction_distance = ray.t_max - ray.t_min;
	vec3 throughput = vec3(1);

	if(found_intersection(ray_payload_brdf))
	{
		Triangle triangle = get_hit_triangle(ray_payload_brdf);
		
		vec3 geo_normal = triangle.normals[0];
		bool is_vertical = abs(geo_normal.z) < 0.1;

		if((is_water(triangle.material_id) || is_slime(triangle.material_id)) && !is_vertical)
		{
			vec3 position = ray.origin + ray.direction * ray_payload_brdf.hit_distance;
			vec3 w = get_water_normal(triangle.material_id, geo_normal, triangle.tangent, position, true);

			float caustic = clamp((1 - pow(clamp(1 - length(w.xz), 0, 1), 2)) * 100, 0, 8);
			caustic = mix(1, caustic, clamp(ray_payload_brdf.hit_distance * 0.02, 0, 1));
			throughput = vec3(caustic);

			if(surface_medium != MEDIUM_NONE)
			{
				extinction_distance = ray_payload_brdf.hit_distance;
			}
			else
			{
				if(is_water(triangle.material_id))
					surface_medium = MEDIUM_WATER;
				else
					surface_medium = MEDIUM_SLIME;

				extinction_distance = max(0, ray.t_max - ray_payload_brdf.hit_distance);
			}
		}
		else if(is_glass(triangle.material_id) || is_water(triangle.material_id) && is_vertical)
		{
			vec3 bary = get_hit_barycentric(ray_payload_brdf);
			vec2 tex_coord = triangle.tex_coords * bary;

			MaterialInfo minfo = get_material_info(triangle.material_id);

	    	vec3 albedo = global_textureLod(minfo.diffuse_texture, tex_coord, 2).rgb;

			if((triangle.material_id & MATERIAL_FLAG_CORRECT_ALBEDO) != 0)
				albedo = correct_albedo(albedo);

			throughput = albedo;
		}
	}

	//return vec3(caustic);
	return extinction(surface_medium, extinction_distance) * throughput;
}

vec3 rgbToNormal(vec3 rgb, out float len)
{
    vec3 n = vec3(rgb.xy * 2 - 1, rgb.z);

    len = length(n);
    return len > 0 ? n / len : vec3(0);
}


float
AdjustRoughnessToksvig(float roughness, float normalMapLen, float mip_level)
{
	float effect = global_ubo.pt_toksvig * clamp(mip_level, 0, 1);
    float shininess = RoughnessToSpecPower(roughness) * effect;
    float ft = normalMapLen / mix(shininess, 1.0f, normalMapLen);
    ft = max(ft, 0.01f);
    return SpecPowerToRoughness(ft * shininess / effect);
}

vec3
compute_direct_illumination_static(vec3 position, vec3 normal, vec3 geo_normal, vec3 view_direction, float phong_exp, float phong_weight, int bounce, uint cluster, bool is_gradient, out vec3 pos_on_light, out int light_index)
{
	float pdf;
	vec3 light_color;
	vec3 light_normal;

	sample_light_list(
			cluster,
			position,
			normal,
			geo_normal,
			view_direction,
			phong_exp,
			phong_weight,
			is_gradient,
			pos_on_light,
			light_color,
			light_normal,
			pdf,
			light_index,
			vec3(
				get_rng(RNG_NEE_LH(bounce)),
				get_rng(RNG_NEE_TRI_X(bounce)),
				get_rng(RNG_NEE_TRI_Y(bounce)))
			);

	if(pdf == 0)
		return vec3(0);

	return light_color / pdf;
}

vec3
compute_direct_illumination_dynamic(vec3 position, vec3 normal, vec3 geo_normal, uint bounce, out vec3 pos_on_light)
{
	if(global_ubo.num_sphere_lights == 0)
		return vec3(0);

	float random_light = get_rng(RNG_NEE_LH(bounce)) * global_ubo.num_sphere_lights;
	uint light_idx = min(global_ubo.num_sphere_lights - 1, uint(random_light));

	// Limit the solid angle of sphere lights for indirect lighting 
	// in order to kill some fireflies in locations with many sphere lights.
	// Example: green wall-lamp corridor in the "train" map.
	float max_solid_angle = (bounce == 0) ? 2 * M_PI : 0.02;
	
	vec3 light_color;
	sample_light_list_dynamic(
		light_idx,
		position,
		normal,
		geo_normal,
		max_solid_angle,
		pos_on_light,
		light_color,
		vec2(get_rng(RNG_NEE_TRI_X(bounce)), get_rng(RNG_NEE_TRI_Y(bounce))));

    light_color *= DYNAMIC_LIGHT_INTENSITY_DIFFUSE;

    return light_color * float(global_ubo.num_sphere_lights);
}

void
get_direct_illumination(
	vec3 position, 
	vec3 normal, 
	vec3 geo_normal, 
	uint cluster_idx, 
	uint material_id,
	int shadow_cull_mask, 
	vec3 view_direction, 
	float roughness, 
	int surface_medium, 
	bool enable_caustics, 
	float surface_specular, 
	float direct_specular_weight, 
	bool enable_static,
	bool enable_dynamic,
	bool is_gradient, 
	int bounce,
	out vec3 diffuse,
	out vec3 specular)
{
	diffuse = vec3(0);
	specular = vec3(0);

	vec3 pos_on_light_static;
	vec3 pos_on_light_dynamic;

	vec3 contrib_static = vec3(0);
	vec3 contrib_dynamic = vec3(0);

	float phong_exp = RoughnessToSpecPower(roughness);
	float phong_weight = min(0.9, surface_specular * direct_specular_weight);

	int static_light_index = -1;

	/* static illumination */
	if(enable_static) {
		contrib_static = compute_direct_illumination_static(position, normal, geo_normal, view_direction, phong_exp, phong_weight, bounce, cluster_idx, is_gradient, pos_on_light_static, static_light_index);
	}

	bool is_static = true;
	float vis = 1;

	/* dynamic illumination */
	if(enable_dynamic) {
		contrib_dynamic = compute_direct_illumination_dynamic(position, normal, geo_normal, bounce, pos_on_light_dynamic);
	}

	float l_static  = luminance(abs(contrib_static));
	float l_dynamic = luminance(abs(contrib_dynamic));
	float l_sum = l_static + l_dynamic;

	bool null_light = (l_sum == 0);

	float w = null_light ? 0.5 : l_static / (l_static + l_dynamic);

	float rng = get_rng(RNG_NEE_STATIC_DYNAMIC(0));
	is_static = (rng < w);
	vis = is_static ? (1 / w) : (1 / (1 - w));
	vec3 pos_on_light = null_light ? position : (is_static ? pos_on_light_static : pos_on_light_dynamic);
	vec3 contrib = is_static ? contrib_static : contrib_dynamic;

	// Surfaces marked with this flag are double-sided, so use a positive ray offset
	float min_t = (material_id & (MATERIAL_FLAG_WARP | MATERIAL_FLAG_DOUBLE_SIDED)) != 0 ? 0.01 : -0.01;
	Ray shadow_ray = get_shadow_ray(position, pos_on_light, min_t);

	vis *= trace_shadow_ray(shadow_ray, null_light ? 0 : shadow_cull_mask);
#ifdef ENABLE_SHADOW_CAUSTICS
	if(enable_caustics)
	{
		contrib *= trace_caustic_ray(shadow_ray, surface_medium);
	}
#endif

	/* 
		Accumulate light shadowing statistics to guide importance sampling on the next frame.
		Inspired by paper called "Adaptive Shadow Testing for Ray Tracing" by G. Ward, EUROGRAPHICS 1994.

		The algorithm counts the shadowed and unshadowed rays towards each light, per cluster,
		per surface orientation in each cluster. Orientation helps improve accuracy in cases 
		when a single cluster has different parts which have the same light mostly shadowed and 
		mostly unshadowed.

		On the next frame, the light CDF is built using the counts from this frame, or the frame
		before that in case of gradient rays. See light_lists.h for more info.

		Only applies to static polygon lights (i.e. no model or beam lights) because the dynamic
		polygon lights do not have static indices, and it would be difficult to map them 
		between frames.
	*/
	if(global_ubo.pt_light_stats != 0 
		&& is_static 
		&& !null_light
		&& static_light_index >= 0 
		&& static_light_index < global_ubo.num_static_lights)
	{
		uint addr = get_light_stats_addr(cluster_idx, static_light_index, get_primary_direction(normal));

		// Offset 0 is unshadowed rays,
		// Offset 1 is shadowed rays
		if(vis == 0) addr += 1;

		// Increment the ray counter
		atomicAdd(light_stats_bufers[global_ubo.current_frame_idx % NUM_LIGHT_STATS_BUFFERS].stats[addr], 1);
	}

	if(null_light)
		return;

	diffuse = vis * contrib;

	if(vis > 0 && direct_specular_weight > 0)
	{
		specular = diffuse * (GGX(view_direction, normalize(pos_on_light - position), normal, roughness, 0.0) * direct_specular_weight);
	}

	vec3 L = pos_on_light - position;
	L = normalize(L);

	float NdotL = max(0, dot(normal, L));

	diffuse *= NdotL / M_PI;
}

void
get_sunlight(
	uint cluster_idx, 
	uint material_id,
	vec3 position, 
	vec3 normal, 
	vec3 geo_normal, 
	vec3 view_direction, 
	float roughness, 
	int surface_medium, 
	bool enable_caustics, 
	out vec3 diffuse, 
	out vec3 specular, 
	int shadow_cull_mask)
{
	diffuse = vec3(0);
	specular = vec3(0);

	if(global_ubo.sun_visible == 0)
		return;

	bool visible = (cluster_idx == ~0u) || (get_sky_visibility(cluster_idx >> 5) & (1 << (cluster_idx & 31))) != 0;

	if(!visible)
		return;

	vec2 rng3 = vec2(get_rng(RNG_SUNLIGHT_X(0)), get_rng(RNG_SUNLIGHT_Y(0)));
	vec2 disk = sample_disk(rng3);
	disk.xy *= global_ubo.sun_tan_half_angle;

	vec3 direction = normalize(global_ubo.sun_direction + global_ubo.sun_tangent * disk.x + global_ubo.sun_bitangent * disk.y);

	float NdotL = dot(direction, normal);
	float GNdotL = dot(direction, geo_normal);

	if(NdotL <= 0 || GNdotL <= 0)
		return;

	float min_t = (material_id & MATERIAL_FLAG_DOUBLE_SIDED) != 0 ? 0.01 : -0.01;
	Ray shadow_ray = get_shadow_ray(position, position + direction * 10000, min_t);
 
	float vis = trace_shadow_ray(shadow_ray, shadow_cull_mask);

	if(vis == 0)
		return;

#ifdef ENABLE_SUN_SHAPE
	// Fetch the sun color from the environment map. 
	// This allows us to get properly shaped shadows from the sun that is partially occluded
	// by clouds or landscape.

	vec3 envmap_direction = (global_ubo.environment_rotation_matrix * vec4(direction, 0)).xyz;
	
    vec3 envmap = textureLod(TEX_PHYSICAL_SKY, envmap_direction.xzy, 0).rgb;

    diffuse = (global_ubo.sun_solid_angle * global_ubo.pt_env_scale) * envmap;
#else
    // Fetch the average sun color from the resolved UBO - it's faster.

    diffuse = sun_color_ubo.sun_color;
#endif

#ifdef ENABLE_SHADOW_CAUSTICS
	if(enable_caustics)
	{
    	diffuse *= trace_caustic_ray(shadow_ray, surface_medium);
	}
#endif

    if(global_ubo.pt_sun_specular > 0)
    {
		float NoH_offset = 0.5 * square(global_ubo.sun_tan_half_angle);
    	specular = diffuse * GGX(view_direction, global_ubo.sun_direction, normal, roughness, NoH_offset);
	}

	diffuse *= NdotL / M_PI;
}

vec3 clamp_output(vec3 c)
{
	if(any(isnan(c)) || any(isinf(c)))
		return vec3(0);
	else 
		return clamp(c, vec3(0), vec3(MAX_OUTPUT_VALUE));
}

vec3
sample_emissive_texture(uint material_id, MaterialInfo minfo, vec2 tex_coord, vec2 tex_coord_x, vec2 tex_coord_y, float mip_level)
{
	if (minfo.emissive_texture != 0)
    {
        vec4 image3;
	    if (mip_level >= 0)
	        image3 = global_textureLod(minfo.emissive_texture, tex_coord, mip_level);
	    else
	        image3 = global_textureGrad(minfo.emissive_texture, tex_coord, tex_coord_x, tex_coord_y);

    	vec3 corrected = correct_emissive(material_id, image3.rgb);

	    return corrected * minfo.emissive_scale;
	}

	return vec3(0);
}

vec2
lava_uv_warp(vec2 uv)
{
	// Lava UV warp that (hopefully) matches the warp in the original Quake 2.
	// Relevant bits of the original rasterizer:

	// #define AMP     8*0x10000
	// #define SPEED   20
	// #define CYCLE   128
	// sintable[i] = AMP + sin(i * M_PI * 2 / CYCLE) * AMP; 
	// #define TURB_SIZE               64  // base turbulent texture size
	// #define TURB_MASK               (TURB_SIZE - 1)
	// turb_s = ((s + turb[(t >> 16) & (CYCLE - 1)]) >> 16) & TURB_MASK;
    // turb_t = ((t + turb[(s >> 16) & (CYCLE - 1)]) >> 16) & TURB_MASK;
    
    return uv.xy + sin(fract(uv.yx * 0.5 + global_ubo.time * 20 / 128) * 2 * M_PI) * 0.125;
}

vec3 get_emissive_shell(uint material_id)
{
	vec3 c = vec3(0);

	if((material_id & (MATERIAL_FLAG_SHELL_RED | MATERIAL_FLAG_SHELL_GREEN | MATERIAL_FLAG_SHELL_BLUE)) != 0)
	{ 
	    if((material_id & MATERIAL_FLAG_SHELL_RED) != 0) c.r += 1;
	    if((material_id & MATERIAL_FLAG_SHELL_GREEN) != 0) c.g += 1;
	    if((material_id & MATERIAL_FLAG_SHELL_BLUE) != 0) c.b += 1;

	    if((material_id & MATERIAL_FLAG_WEAPON) != 0) c *= 0.2;
	}

    return c;
}

bool get_is_gradient(ivec2 ipos)
{
	if(global_ubo.flt_enable != 0)
	{
		uint u = texelFetch(TEX_ASVGF_GRAD_SMPL_POS_A, ipos / GRAD_DWN, 0).r;

		ivec2 grad_strata_pos = ivec2(
				u >> (STRATUM_OFFSET_SHIFT * 0),
				u >> (STRATUM_OFFSET_SHIFT * 1)) & STRATUM_OFFSET_MASK;

		return (u > 0 && all(equal(grad_strata_pos, ipos % GRAD_DWN)));
	}
	
	return false;
}


void
get_material(Triangle triangle, vec2 tex_coord, vec2 tex_coord_x, vec2 tex_coord_y, float mip_level, vec3 geo_normal,
    out vec3 albedo, out vec3 normal, out float metallic, out float specular, out float roughness, out vec3 emissive)
{
	if((triangle.material_id & MATERIAL_FLAG_FLOWING) != 0)
	{
		tex_coord.x -= global_ubo.time * 0.5;
	}

	if((triangle.material_id & MATERIAL_FLAG_WARP) != 0)
	{
		tex_coord = lava_uv_warp(tex_coord);
	}


	MaterialInfo minfo = get_material_info(triangle.material_id);
	

    vec4 image1;
	if (mip_level >= 0)
	    image1 = global_textureLod(minfo.diffuse_texture, tex_coord, mip_level);
	else
	    image1 = global_textureGrad(minfo.diffuse_texture, tex_coord, tex_coord_x, tex_coord_y);

	if((triangle.material_id & MATERIAL_FLAG_CORRECT_ALBEDO) != 0)
		albedo = correct_albedo(image1.rgb);
	else
		albedo = image1.rgb;

	normal = geo_normal;
	metallic = 0;
    specular = 0;
    roughness = 1;

    if (minfo.normals_texture != 0)// && dot(triangle.tangent, triangle.tangent) > 0)
    {
        vec4 image2;
	    if (mip_level >= 0)
	        image2 = global_textureLod(minfo.normals_texture, tex_coord, mip_level);
	    else
	        image2 = global_textureGrad(minfo.normals_texture, tex_coord, tex_coord_x, tex_coord_y);

		float normalMapLen;
		vec3 local_normal = rgbToNormal(image2.rgb, normalMapLen);

		if(dot(triangle.tangent, triangle.tangent) > 0)
		{
			vec3 tangent = triangle.tangent,
				 bitangent = cross(geo_normal, tangent);

			if((triangle.material_id & MATERIAL_FLAG_HANDEDNESS) != 0)
        		bitangent = -bitangent;
			
			normal = tangent * local_normal.x + bitangent * local_normal.y + geo_normal * local_normal.z;
        
			float bump_scale = global_ubo.pt_bump_scale * minfo.bump_scale;
			if(is_glass(triangle.material_id))
        		bump_scale *= 0.2;

			normal = normalize(mix(geo_normal, normal, bump_scale));
		}

        metallic = clamp(image2.a * minfo.specular_scale, 0, 1);
        
        if(minfo.roughness_override >= 0)
        	roughness = max(image1.a, minfo.roughness_override);
        else
        	roughness = image1.a;

        roughness = clamp(roughness, 0, 1);

        float effective_mip = mip_level;

    	if (effective_mip < 0)
    	{
        	ivec2 texSize = global_textureSize(minfo.normals_texture, 0);
        	vec2 tx = tex_coord_x * texSize;
        	vec2 ty = tex_coord_y * texSize;
        	float d = max(dot(tx, tx), dot(ty, ty));
        	effective_mip = 0.5 * log2(d);
        }

        bool is_mirror = (roughness < MAX_MIRROR_ROUGHNESS) && (is_chrome(triangle.material_id) || is_screen(triangle.material_id));

        if (normalMapLen > 0 && global_ubo.pt_toksvig > 0 && effective_mip > 0 && !is_mirror)
        {
            roughness = AdjustRoughnessToksvig(roughness, normalMapLen, effective_mip);
        }
    } 

    if(global_ubo.pt_roughness_override >= 0) roughness = global_ubo.pt_roughness_override;
    if(global_ubo.pt_metallic_override >= 0) metallic = global_ubo.pt_metallic_override;

	specular = mix(0.05, 1.0, metallic);

    if(roughness == 1)
    {
    	specular = 0;
    }

    emissive = sample_emissive_texture(triangle.material_id, minfo, tex_coord, tex_coord_x, tex_coord_y, mip_level);

    emissive += get_emissive_shell(triangle.material_id) * albedo * (1 - metallic * 0.9);
}