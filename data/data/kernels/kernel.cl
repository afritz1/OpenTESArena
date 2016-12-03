// kernel.cl for OpenTESArena.

// Here is an overview of the ray tracer design. It may change once in a while, but 
// the overall objective should stay the same.
// 1) Primary ray intersections kernel
// 2) Ray tracing kernel (material colors, shadows, reflections)
// 3) (Optional) Post-processing kernel
// 4) Float to int conversion kernel

// OpenCL programming hints:
// - Spread the work out over multiple smaller kernels.
// - Consider using "vload" and "vstore" with vector types (float4).
// - Minimize nesting of if-else statements.
// - Try "fma(...)" and "mad(...)" functions for faster arithmetic.
// - Individual operations could be faster than vector operations.

// Assume OpenCL aligns structs to 8 byte boundaries.

/* ------------------------- */
/* Host settings */
/* -> These settings are added to this file at compile time from the host. */
/* ------------------------- */

#ifndef RENDER_WIDTH
#error "Missing RENDER_WIDTH definition."
#endif

#ifndef RENDER_HEIGHT
#error "Missing RENDER_HEIGHT definition."
#endif

// These world values determine the dimensions of the three reference grids
// (Voxel, Sprite, and Light). A more advanced renderer would use chunks 
// instead, like Minecraft does.
#ifndef WORLD_WIDTH
#error "Missing WORLD_WIDTH definition."
#endif

#ifndef WORLD_HEIGHT
#error "Missing WORLD_HEIGHT definition."
#endif

#ifndef WORLD_DEPTH
#error "Missing WORLD_DEPTH definition."
#endif

/* ------------------------- */
/* Constant settings */
/* ------------------------- */

#define FALSE 0
#define TRUE (!FALSE)

#define EPSILON 1.0e-5f

#define MAX_FOG_DISTANCE 35.0f // This should be dynamic eventually.
#define BETTER_FOG TRUE

#define RENDER_WIDTH_REAL ((float)RENDER_WIDTH)
#define RENDER_HEIGHT_REAL ((float)RENDER_HEIGHT)
#define RENDER_WIDTH_RECIP (1.0f / RENDER_WIDTH_REAL)
#define RENDER_HEIGHT_RECIP (1.0f / RENDER_HEIGHT_REAL)
#define ASPECT_RATIO (RENDER_WIDTH_REAL * RENDER_HEIGHT_RECIP)

#define GLOBAL_UP ((float3)(0.0f, 1.0f, 0.0f))

#define INTERSECTION_T_MAX MAXFLOAT

#define RAY_INITIAL_DEPTH 0

/* ------------------------- */
/* Camera struct */
/* ------------------------- */

// Zoom allows for a variable FOV without recompiling the kernel. 
// Aspect ratio is compile-time constant, though.
// Assume all camera axes are normalized.

typedef struct
{
	float3 eye, forward, right, up;
	float zoom;
} Camera;

/* ------------------------- */
/* Intersection struct */
/* ------------------------- */

// Point is the hit point on the shape. Normal is the normal at the hit point.
// T is for the depth buffer. U and V are texture coordinates.
// RectIndex is the number of rectangles to skip in the rectangles array.

typedef struct
{
	float3 point, normal;
	float t, u, v;
	int rectIndex;
} Intersection;

/* ------------------------- */
/* OwnerReference struct */
/* ------------------------- */

// Points to a list of rectangle indices into the rect buffer that a shadow ray
// should ignore when checking a point light's shadows. 

// The indices all point to the same shape (the "owner sprite"), but there must be 
// a list instead of one sprite ID because the sprite can cover more than one voxel 
// (requiring a copy per voxel with this design, for better cache performance).

// The offset and count are in units of integers.

typedef struct
{
	int offset;
	int count;
} OwnerReference;

/* ------------------------- */
/* Light struct */
/* ------------------------- */

// All lights will use quadratic drop-off (inverse square law).
// -> percent = ((x - D) ^ 2) / (D ^ 2), for some max distance D.
// -> Max reach of the light is determined by intensity (in this design).

// Time complexity of ray tracing is linear in the number of nearby lights, 
// so don't go overboard!

typedef struct
{
	float3 point, color;
	OwnerReference ownerRef;
	float intensity;
} Light;

/* ------------------------- */
/* LightReference struct */
/* ------------------------- */

// The offset and count are in units of lights.

// It is unlikely that more than five or six lights will touch a voxel at a time.

typedef struct
{
	int offset;
	int count;
} LightReference;

/* ------------------------- */
/* Ray struct */
/* ------------------------- */

typedef struct
{
	float3 point, direction;
	int depth;
} Ray;

/* ------------------------- */
/* SpriteReference struct */
/* ------------------------- */

// No need for a "Sprite" struct; it's just a rectangle.

// The offset and count are in units of rectangles.

// It is unlikely that more than a dozen sprites will occupy a voxel at a time.

typedef struct
{
	int offset;
	int count;
} SpriteReference;

/* ------------------------- */
/* TextureReference struct */
/* ------------------------- */

// No need for a "Texture" struct; it's just float4's.

// The offset is in units of float4's to skip.

// Textures may have several thousands of pixels, including ones from the various 
// layers (normal, specular, ...) eventually.

typedef struct
{
	int offset;
	short width, height;
} TextureReference;

/* ------------------------- */
/* Rectangle struct */
/* ------------------------- */

// It's likely that there will eventually be layers of textures, like normal maps
// and things. However, they could still all use the same ID and just have the
// texture's float4's get packed together. The texture function would jump to get
// to the beginnings of the other pixels.

// p1p2 and p2p3 are not normalized, but normal is.

typedef struct
{
	float3 p1, p2, p3, p1p2, p2p3, normal;
	TextureReference textureRef;
} Rectangle;

/* ------------------------- */
/* VoxelReference struct */
/* ------------------------- */

// No need for a "Voxel" struct; it's just rectangles.

// The offset is in units of rectangles to skip.

// It is unlikely that a voxel will have more than six rectangles.

typedef struct
{
	int offset;
	int count;
} VoxelReference;

/* ------------------------- */
/* Vector functions */
/* ------------------------- */

int float3ToRGB(float3 color)
{
	return (int) 
		(((uchar)(color.x * 255.0f) << 16) |
		 ((uchar)(color.y * 255.0f) << 8) |
		 ((uchar)(color.z * 255.0f)));
}

/* ------------------------- */
/* Texture functions */
/* ------------------------- */

float4 getTextureColor(const Rectangle *rect, float u, float v, 
	const __global float4 *textures)
{
	/* Texture mapping function for rectangles. */	
	const TextureReference *textureRef = &rect->textureRef;
	const int width = textureRef->width;
	const int height = textureRef->height;
	const int offset = textureRef->offset;

	const int x = (int)(u * (float)width);
	const int y = (int)(v * (float)height);

	return textures[offset + x + (y * width)];
}

/* ------------------------- */
/* Camera functions */
/* ------------------------- */

// Generate a direction through the view frustum given x and y screen percents.
float3 cameraImageDirection(const __global Camera *camera, float xx, float yy)
{
	float3 forwardComponent = camera->forward * camera->zoom;
	float3 rightComponent = camera->right * (ASPECT_RATIO * ((2.0f * xx) - 1.0f));
	float3 upComponent = camera->up * ((2.0f * yy) - 1.0f);
	return normalize(forwardComponent + rightComponent - upComponent);
}

/* ------------------------- */
/* Rectangle functions */
/* ------------------------- */

Intersection rectangleHit(const Rectangle *rect, const Ray *ray)
{
	// Ray-rectangle intersection algorithm.
	Intersection intersection;
	intersection.t = INTERSECTION_T_MAX;

	const float normalDirDot = dot(rect->normal, ray->direction);

	if (fabs(normalDirDot) < EPSILON)
	{
		return intersection;
	}

	const float t = -dot(rect->normal, ray->point - rect->p1) / normalDirDot;
	const float3 p = ray->point + (ray->direction * t);

	const float3 diff = p - rect->p1;

	const float p2p3Len = length(rect->p2p3);
	const float p1p2Len = length(rect->p1p2);
	const float u = dot(diff, rect->p2p3) / (p2p3Len * p2p3Len);
	const float v = dot(diff, rect->p1p2) / (p1p2Len * p1p2Len);

	// If the texture coordinates are valid and the distance is positive, 
	// then the intersection is valid.
	if ((u >= 0.0f) && (u <= 1.0f) && (v >= 0.0f) && (v <= 1.0f) && (t > 0.0f))
	{
		// There was a hit. Shape index is set by the calling function.
		intersection.t = t;
		intersection.u = u;
		intersection.v = v;
		intersection.point = p;
		intersection.normal = (normalDirDot < 0.0f) ? rect->normal : -rect->normal;
	}

	return intersection;
}

Intersection rectangleHitTextured(const Rectangle *rect, const Ray *ray,
	const __global float4 *textures, float *alpha)
{
	// Ray-rectangle intersection function that ignores a hit if the texture is
	// transparent at that point.
	Intersection intersection = rectangleHit(rect, ray);

	if (intersection.t < INTERSECTION_T_MAX)
	{
		const float u = intersection.u;
		const float v = intersection.v;
		const float4 texelColor = getTextureColor(rect, u, v, textures);
		*alpha = texelColor.w;
	}

	return intersection;
}

/* ------------------------- */
/* 3D grid functions */
/* ------------------------- */

// Returns true if a cell coordinate is contained within the world grid.
bool gridContains(int x, int y, int z)
{
	return (x >= 0) && (y >= 0) && (z >= 0) && 
		(x < WORLD_WIDTH) && (y < WORLD_HEIGHT) && (z < WORLD_DEPTH);
}

// 3D digital differential analysis algorithm.
// - This function takes a ray and walks through a voxel grid. It stops once the max
//   distance has been reached or if an opaque object is hit.
void voxelDDA(const Ray *ray,
	Intersection *intersection,
	const __global VoxelReference *voxelRefs,
	const __global SpriteReference *spriteRefs,
	const __global Rectangle *rects,
	const __global float4 *textures)
{
	// Set up the 3D-DDA cell and step direction variables.
	const float3 position = ray->point;
	const float3 direction = ray->direction;
	const int3 startCell = (int3)(
		(int)floor(position.x),
		(int)floor(position.y),
		(int)floor(position.z));
	const char3 nonNegativeDir = (char3)(
		direction.x >= 0.0f,
		direction.y >= 0.0f,
		direction.z >= 0.0f);
	const int3 step = (int3)(
		nonNegativeDir.x ? 1 : -1,
		nonNegativeDir.y ? 1 : -1,
		nonNegativeDir.z ? 1 : -1);

	// Epsilon needs to be just right for there to be no artifacts. 
	// Hopefully all graphics cards running this have 32-bit floats.
	// Maybe don't use "fast" functions for extra precision?
	const float3 invDirection = (float3)(
		1.0f / ((fabs(direction.x) < EPSILON) ? (EPSILON * (float)step.x) : direction.x),
		1.0f / ((fabs(direction.y) < EPSILON) ? (EPSILON * (float)step.y) : direction.y),
		1.0f / ((fabs(direction.z) < EPSILON) ? (EPSILON * (float)step.z) : direction.z));
	const float3 deltaDist = normalize((float3)(
		(float)step.x * invDirection.x,
		(float)step.y * invDirection.y,
		(float)step.z * invDirection.z));
	
	int3 cell = startCell;
	const float3 sideDist = deltaDist * (float3)(
		nonNegativeDir.x ? ((float)cell.x + 1.0f - position.x) : 
			(position.x - (float)cell.x),
		nonNegativeDir.y ? ((float)cell.y + 1.0f - position.y) : 
			(position.y - (float)cell.y),
		nonNegativeDir.z ? ((float)cell.z + 1.0f - position.z) : 
			(position.z - (float)cell.z));

	// Intersection data for voxels and sprites. They need to be kept separate
	// so the voxel stepping doesn't quit early and inadvertently cause occluded 
	// sprites to show up in front of walls.
	Intersection voxelIntersection, spriteIntersection;
	voxelIntersection.t = INTERSECTION_T_MAX;
	spriteIntersection.t = INTERSECTION_T_MAX;

	// Walk through the voxel grid while the current cell is contained in the world.
	while (gridContains(cell.x, cell.y, cell.z))
	{
		const int gridIndex = cell.x + (cell.y * WORLD_WIDTH) + 
			(cell.z * WORLD_WIDTH * WORLD_HEIGHT);
		
		// Try intersecting each shape pointed to by the voxel reference if no voxel
		// geometry has been hit yet.
		if (voxelIntersection.t == INTERSECTION_T_MAX)
		{
			const VoxelReference voxelRef = voxelRefs[gridIndex];
			for (int i = 0; i < voxelRef.count; ++i)
			{
				const int rectIndex = voxelRef.offset + i;
				const Rectangle rect = rects[rectIndex];
				float alpha = 0.0f;
				Intersection currentTry = rectangleHitTextured(&rect, ray, textures, &alpha);

				// Overwrite the nearest voxel rectangle if the current try is closer 
				// and the intersected texel is not transparent.
				if ((currentTry.t < voxelIntersection.t) && (alpha != 0.0f))
				{
					currentTry.rectIndex = rectIndex;
					voxelIntersection = currentTry;
				}
			}
		}
		
		// Try intersecting each shape pointed to by the sprite reference if no sprite
		// geometry has been hit yet.
		if (spriteIntersection.t == INTERSECTION_T_MAX)
		{
			const SpriteReference spriteRef = spriteRefs[gridIndex];
			for (int i = 0; i < spriteRef.count; ++i)
			{
				const int rectIndex = spriteRef.offset + i;
				const Rectangle rect = rects[rectIndex];
				float alpha = 0.0f;
				Intersection currentTry = rectangleHitTextured(&rect, ray, textures, &alpha);
			
				// Overwrite the nearest sprite rectangle if the current try is closer 
				// and the intersected texel is not transparent.
				if ((currentTry.t < spriteIntersection.t) && (alpha != 0.0f))
				{
					currentTry.rectIndex = rectIndex;
					spriteIntersection = currentTry;
				}
			}
		}

		// Check how far the stepping has gone from the start.
		const float3 diff = (float3)((float)cell.x, (float)cell.y, (float)cell.z) - position;
		const float dist = length(diff);

		// If a voxel shape and sprite shape have been hit, or if the stepping has gone 
		// too far, then stop.
		if (((voxelIntersection.t < INTERSECTION_T_MAX) && 
			(spriteIntersection.t < INTERSECTION_T_MAX)) || (dist > MAX_FOG_DISTANCE))
		{
			break;
		}

		// Decide which voxel to step towards next.
		if ((sideDist.x < sideDist.y) && (sideDist.x < sideDist.z))
		{
			sideDist.x += deltaDist.x;
			cell.x += step.x;
		}
		else if (sideDist.y < sideDist.z)
		{
			sideDist.y += deltaDist.y;
			cell.y += step.y;
		}
		else
		{
			sideDist.z += deltaDist.z;
			cell.z += step.z;
		}
	}

	// Get the closer of the two intersections.
	*intersection = (voxelIntersection.t < spriteIntersection.t) ?
		voxelIntersection : spriteIntersection;
}

/* ------------------------- */
/* Shading functions */
/* ------------------------- */

float getFogPercent(float distance, float maxDistance)
{
#if BETTER_FOG
	// Exponential squared fog.
	const float fogDensity = sqrt(-log(EPSILON) / (maxDistance * maxDistance));
	return 1.0f - (1.0f / exp((distance * distance) * (fogDensity * fogDensity)));
#else
	// Linear fog.
	return clamp(distance / maxDistance, 0.0f, 1.0f);
#endif
}

// Primary coloring function for a ray hitting the sky.
float3 getSkyColor(float3 direction, float3 sunDirection, float3 sunColor,
	float3 backgroundColor)
{	
	// Values regarding how the sun is calculated. This could be refined some more.
	const float sunSize = 0.99925f;
	const float sunGlowSize = sunSize - 0.0065f;

	const float raySunDot = dot(direction, sunDirection);

	if (raySunDot < sunGlowSize)
	{
		// The ray is not near the sun.
		return backgroundColor;
	}
	else if (raySunDot < sunSize)
	{
		// The ray is near the sun. Soften the sun glow with a power function.
		const float sunPercent = pow((raySunDot - sunGlowSize) / 
			(sunSize - sunGlowSize), 6.0f);
		return mix(backgroundColor, sunColor, sunPercent);
	}
	else
	{
		// The ray is in the sun.
		return sunColor;
	}
}

// Gets the color of a material at an intersection.
float3 getShadedColor(const Rectangle *rect, float depth, float3 normal, float3 view,
	float3 point, float2 uv, const float3 sunDirection, const float3 sunColor, 
	const float3 backgroundColor, const float ambient,
	const __global VoxelReference *voxelRefs,
	const __global SpriteReference *spriteRefs,
	const __global LightReference *lightRefs,
	const __global Rectangle *rects,
	const __global Light *lights,
	const __global float4 *textures)
{
	// Get the color of the texel that was hit.
	const float3 textureColor = getTextureColor(rect, uv.x, uv.y, textures).xyz;

	// Reduce the texture color by the ambient percent.
	float3 color = textureColor * ambient;

	// See if the intersection point is lit by the sun.
	Ray sunRay;
	sunRay.point = point + (normal * EPSILON);
	sunRay.direction = sunDirection;
	sunRay.depth = RAY_INITIAL_DEPTH;

	Intersection sunTry;
	sunTry.t = INTERSECTION_T_MAX;
	voxelDDA(&sunRay, &sunTry, voxelRefs, spriteRefs, rects, textures);

	if (sunTry.t == INTERSECTION_T_MAX)
	{
		// The point is lit. Add some sunlight depending on the angle.
		const float lnDot = dot(sunDirection, normal);
		color += (textureColor * sunColor) * lnDot;
	}

	// Interpolate with fog based on depth.
	const float fogPercent = getFogPercent(depth, MAX_FOG_DISTANCE);
	return mix(color, backgroundColor, fogPercent);
}

// Primary function for managing a ray tracing loop. The parameters include the initial
// intersection data to start from, and they may be overwritten as needed.
// - Currently just a ray caster. A recursive (iterative) version is on the to-do list.
//   The user will eventually be able to select maximum recursion depth (0-3 ish).
float3 getRayTracedColor(float depth, float3 normal, float3 view, 
	float3 point, float2 uv, int rectIndex,
	const __global VoxelReference *voxelRefs,
	const __global SpriteReference *spriteRefs,
	const __global LightReference *lightRefs,
	const __global Rectangle *rects,
	const __global Light *lights,
	const __global float4 *textures,
	const __global float *gameTime)
{
	// The world time is based on game time. This value could certainly use some 
	// refinement so the sun position actually matches the game time. It would
	// need to know how many seconds a game day is (30 minutes?).
	const float worldTime = (*gameTime) * 0.020f;

	// Arbitrary sun color.
	const float3 sunColor = (float3)(1.0f, 0.925f, 0.80f);

	// Direction towards the sun.
	const float3 sunDirection = normalize((float3)(
		sin(worldTime) * 0.20f, sin(worldTime), cos(worldTime)));
	
	// Ambient light value based on the sun's height. It is clamped above a minimum
	// so night isn't pitch black (dungeons might be, though. Eventually).
	const float ambient = clamp(sunDirection.y * 1.20f, 0.20f, 1.0f);
	
	// Sky colors to interpolate between.
	const float3 horizonColor = (float3)(0.90f, 0.90f, 0.95f);
	const float3 zenithColor = horizonColor * 0.75f;

	// Direction of the ray at the intersection, opposite the view vector.
	const float3 direction = -view;
	
	// Elevation of the ray relative to the horizontal plane, clamped non-negative.
	const float elevationPercent = clamp(direction.y, 0.0f, 1.0f);

	// Background color if the ray were to hit the sky. Also acts as fog.
	const float3 backgroundColor = mix(horizonColor, zenithColor, 
		elevationPercent) * ambient;

	// Resulting color of the current ray.
	float3 color = (float3)0.0f;

	// See if a shape was hit.
	if (depth <= MAX_FOG_DISTANCE)
	{
		// A shape was hit. Calculate the shaded color.
		const Rectangle rect = rects[rectIndex];
		color = getShadedColor(&rect, depth, normal, view, point, uv, sunDirection, 
			sunColor, backgroundColor, ambient, voxelRefs, spriteRefs, lightRefs, 
			rects, lights, textures);

		// Reflections go here eventually...
	}
	else
	{
		// No shape was hit. Calculate the color of the sky for this ray.
		color = getSkyColor(direction, sunDirection, sunColor, backgroundColor);
	}

	return color;
}

/* ------------------------- */
/* Kernel functions */
/* ------------------------- */

// The intersect kernel populates the primary rays' depth, normal, view, 
// hit point, UV, and rectangle index buffers.
__kernel void intersect(
	const __global Camera *camera,
	const __global VoxelReference *voxelRefs,
	const __global SpriteReference *spriteRefs,
	const __global Rectangle *rects,
	const __global float4 *textures,
	__global float *depthBuffer, 
	__global float3 *normalBuffer,
	__global float3 *viewBuffer, 
	__global float3 *pointBuffer, 
	__global float2 *uvBuffer,
	__global int *rectangleIndexBuffer)
{
	// Coordinates of "this" pixel relative to the top left.
	const int x = get_global_id(0);
	const int y = get_global_id(1);

	// Get 0->1 percentages across the screen.
	const float xx = (float)x * RENDER_WIDTH_RECIP;
	const float yy = (float)y * RENDER_HEIGHT_RECIP;

	Ray ray;
	ray.point = camera->eye;
	ray.direction = cameraImageDirection(camera, xx, yy);
	ray.depth = RAY_INITIAL_DEPTH;

	Intersection intersection;
	intersection.t = INTERSECTION_T_MAX;

	// Get the nearest geometry hit by the ray.
	voxelDDA(&ray, &intersection, voxelRefs, spriteRefs, rects, textures);
	
	// Put the intersection data in the global buffers.
	const int index = x + (y * RENDER_WIDTH);
	depthBuffer[index] = intersection.t;
	normalBuffer[index] = intersection.normal;
	viewBuffer[index] = -ray.direction;
	pointBuffer[index] = intersection.point;
	uvBuffer[index] = (float2)(intersection.u, intersection.v);
	rectangleIndexBuffer[index] = intersection.rectIndex;
}

// Fill the color buffer with the results of rays traced in the world.
__kernel void rayTrace(
	const __global VoxelReference *voxelRefs,
	const __global SpriteReference *spriteRefs,
	const __global LightReference *lightRefs,
	const __global Rectangle *rects,
	const __global Light *lights,
	const __global int *owners,
	const __global float4 *textures,
	const __global float *gameTime,
	const __global float *depthBuffer, 
	const __global float3 *normalBuffer, 
	const __global float3 *viewBuffer,
	const __global float3 *pointBuffer, 
	const __global float2 *uvBuffer,
	const __global int *rectangleIndexBuffer,
	__global float3 *colorBuffer)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	const int index = x + (y * RENDER_WIDTH);

	// Begin the ray tracing function with some initial data.
	const float depth = depthBuffer[index];
	const float3 normal = normalBuffer[index];
	const float3 view = viewBuffer[index];
	const float3 point = pointBuffer[index];
	const float2 uv = uvBuffer[index];
	const int rectIndex = rectangleIndexBuffer[index];

	const float3 color = getRayTracedColor(depth, normal, view, point, uv, rectIndex,
		voxelRefs, spriteRefs, lightRefs, rects, lights, textures, gameTime);

	colorBuffer[index] = color;
}

// Optional kernel. Does it also need a temp float3 buffer?
__kernel void postProcess(
	const __global float3 *input, 
	__global float3 *output)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	
	const int index = x + (y * RENDER_WIDTH);
	float3 color = input[index];

	// --- Write to color here ---

	// Brightness... gamma correction...

	output[index] = color;
}

// Prepare float3 colors for display.
__kernel void convertToRGB(
	const __global float3 *input, 
	__global int *output)
{
	const int x = get_global_id(0);
	const int y = get_global_id(1);
	
	const int index = x + (y * RENDER_WIDTH);

	// The color doesn't *have* to be clamped here. It just has to be within 
	// the range of [0, 255]. Clamping is just a simple solution for now.
	float3 color = clamp(input[index], 0.0f, 1.0f);

	output[index] = float3ToRGB(color);
}
