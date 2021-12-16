#ifndef ENTITY_RENDER_DEFINITION_H
#define ENTITY_RENDER_DEFINITION_H

// Common entity render data usable by all renderers. Can be pointed to by multiple entity
// render instances.

// Most effective with static, single-frame entities like trees. Dynamic entities like citizens will benefit
// less from this, although two citizens on the same animation frame with identical palettes should still
// be able to share this in some cases.

class EntityRenderDefinition
{
private:
	// @todo: shared entity render data a renderer would care about
	// - texture ID (only ONE frame of animation, no "list of IDs for an animation" in here. Forces all entities' anims
	//   to be evaluated relative to the camera before getting inserted into the scene graph.
	// - entity shading type enum: default (NPCs, trees), light (always max light level), ghost, etc.

	// @todo: std::optional citizen palette ObjectTextureID from renderer (this is here and not in the render inst in case
	// two or more citizens look identical on the same frame, unlikely as it may be in practice)

	// @todo: light properties? std::optional<double> lightRadius?
public:

};

#endif
