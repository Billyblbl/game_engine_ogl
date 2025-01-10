# Focus

- [x] Research vertex attributes vs [[Vertex Pulling]]
- [x] Renderer scheme decoupled from buffers
- [x] Research double buffered batch construction
	- prevents synchronisation issues
	- binding may be a perf issue
		- alternatively using DSA to bind once (or on internal state changes) and never unbind
			- could still allow double buffering by using 1 buffer using halves
- [x] Tilemap renderer -> [[Tilemap rework]]
	- [x] Command construction
	- ~~Reuse modified spritemesh renderer instead ?~~
- [x] Rework ui renderer
	- sheets system
		- sheet decides style & depth
		- push quads per sheet
		- index sheet per gl_DrawID
		- 1 text pushes a sheet & pushes quads for each character
		- ui panels can push white or textured quads
		- "list"-like ui panel layouts may share a sheet for elements
	- tinted textured quads, sample into white texture for flat colors
- [ ] Rework physics
	- needs to be simpler, GJK is nice and all but probably overengineered for this purpose
	- Might not even need rotations, could probably just go with AABBs all the way
		- lets start with AABB, others will use those for culling anyway
- [ ] Trim application complexity
- [ ] Sound perf
	- profiler indicates pretty long processing, what is going on here ?

# Potential tech debt

- [x] Decouple sprite mesh renderer from buffers
	- Tilemap renderer seems to be in an approprotate state for this (pipeline | renderer | batch)
- [ ] Investigate getting vertex format info directly from the attribute info in the shader
