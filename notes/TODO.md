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
- [x] Rework physics
	- [[Physics Rework]]
- [x] application utilities pass
	- [x] gamepad detection
	- [x] change time to accumulation
	- [x] change time to 64bits
- [ ] Sound perf
	- profiler indicates pretty long processing, what is going on here ?
- [ ] Make test environment
	- [ ] environment tileset
	- [ ] tilemap
	- [ ] controllable camera
	- [ ] play/pause/reset

# Potential tech debt

- [x] Decouple sprite mesh renderer from buffers
	- Tilemap renderer seems to be in an appropriate state for this (pipeline | renderer | batch)

- [ ] parallel array list data structure
- [ ] linked buffers accumulator data structure
- [ ] Investigate getting vertex format info directly from the attribute info in the shader
- [ ] multiple assert definition ambiguity
- [x] tilemap concerns are too scrambled together, rendering, physics, recursive structural stuff, entities ?
- [ ] energy conservation on multiple deltas response is currently done by averaging the deltas by correction count, should it be weighted on something ? (solve_collisions)
- [ ] physics properties should probably be differentiated between body properties and surfaces properties
	- could register "materials" in sim step and refer to them in colliders
		- registering in step wont keep it around between frame, do we want it to be supplied in collider or in shape ? feels like it should be in collider
		- currently have 2 32bit numbers as properties, might not be worth the indirection
- [ ] tilemap surface properties