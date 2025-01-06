# Focus

- [x] Research vertex attributes vs [[Vertex Pulling]]
- [x] Renderer scheme decoupled from buffers
- [ ] Research double buffered batch construction
	- prevents synchronisation issues
	- binding may be a perf issue
		- alternatively using DSA to bind once (or on internal state changes) and never unbind
			- could still allow double buffering by using 1 buffer using halves
- [ ] Tilemap renderer -> [[Focus]]
	- [ ] Command construction
	- [ ] Reuse modified spritemesh renderer instead ?
- [ ] Rework physics
	- needs to be simpler, GJK is nice and all but overengineered for this purpose
	- Might not even need rotations, could probably just go with AABBs all the way
		- lets start with AABB, others will use those for culling anyway
- [ ] Trim application complexity
- [ ] Sound perf
	- profiler indicates pretty long processing, what is going on here ?

# Potential tech debt

- [ ] Decouple sprite mesh renderer from buffers
	- Tilemap renderer seems to be in an approprotate state for this (pipeline | renderer | batch)
