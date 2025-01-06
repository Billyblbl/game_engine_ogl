![](https://www.youtube.com/watch?v=kVqFMKF1YcQ)
https://www.yosoygames.com.ar/wp/2018/03/vertex-formats-part-2-fetch-vs-pull/
https://github.com/gpuweb/gpuweb/discussions/4967
https://github.com/nlguillemot/ProgrammablePulling

- Slight situational overhead compared to vertex buffers
- can allow for more complex access patterns
- Easy to put in place but not necessary to make the standard way
- To apply on a case by case basis per renderer
- ~~pushing can only share data between vertices by divisor~~ this is actually false, divisor is for instance id not vertex id, need pulling or duplication to share data between vertices
- ~~can use pushing to provide indices for pulling, useful if the shader is does not know the divisor in advance (effectively uses a vbo as an ibo in a sense)~~


