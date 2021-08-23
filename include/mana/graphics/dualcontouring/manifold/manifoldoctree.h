#pragma once
#ifndef MANIFOLD_OCTREE_H
#define MANIFOLD_OCTREE_H

#include "mana/core/memoryallocator.h"
//
#include <cnoise/cnoise.h>
#include <cstorage/cstorage.h>
#include <ubermath/ubermath.h>

#include "mana/graphics/dualcontouring/manifold/manifolddualcontouring.h"
#include "mana/graphics/dualcontouring/manifold/manifoldtables.h"
#include "mana/graphics/dualcontouring/qef.h"
#include "mana/graphics/utilities/mesh.h"

#define MANIFOLD_RESOLUTION 32
#define MANIFOLD_MAX_OCTREE_LEVELS 6  // 1, 2, 4, 8, 16, 32

enum NodeType {
  MANIFOLD_NODE_NONE,
  MANIFOLD_NODE_INTERNAL,
  MANIFOLD_NODE_LEAF,
  MANIFOLD_NODE_COLLAPSED
};

struct Vertex {
  struct Vertex* parent;
  int index;
  bool collapsible;
  struct QefSolver qef;
  vec3 normal;
  int surface_index;
  float error;
  int euler;
  int eis[12];
  int in_cell;
  bool face_prop2;
};

static inline void vertex_init(struct Vertex* vertex) {
  vertex->parent = NULL;
  vertex->index = -1;
  vertex->collapsible = true;
  qef_solver_init(&vertex->qef);
  vertex->normal = VEC3_ZERO;
  vertex->surface_index = -1;
  vertex->error = 0.0f;
  vertex->euler = 0;
  memset(vertex->eis, 0, sizeof(int) * 12);
  vertex->face_prop2 = false;
}

struct ManifoldOctreeNode {
  int index;
  ivec3 position;
  int size;
  int scale;
  struct ManifoldOctreeNode* children[8];
  struct ManifoldOctreeNode* node_cache[MANIFOLD_MAX_OCTREE_LEVELS];
  enum NodeType type;
  struct ArrayList* vertices;
  unsigned char corners;
  int child_index;
};

static inline void octree_node_init(struct ManifoldOctreeNode* octree_node, ivec3 position, int size, int scale, enum NodeType type) {
  octree_node->index = 0;
  octree_node->position = position;
  octree_node->size = size;
  octree_node->scale = scale;
  octree_node->type = type;
}

/*static inline float Sphere(vec3 pos) {
  float STEP = 1.0 / 64.0f;
  struct RidgedFractalNoise noise = {0};
  ridged_fractal_noise_init(&noise);
  noise.octave_count = 4;
  noise.frequency = 1.0;
  noise.lacunarity = 2.2324f;
  noise.step = STEP;

  return ridged_fractal_noise_eval_3d_single(&noise, pos.x, pos.y, pos.z);
}*/

// Note: The following aren't that slow something else is taking up cpu cycles
// TODO: Build this into noise library
//static inline void planet_sample(float poss[8][3], float dest[8], struct Vector* noises, int scale) {
//  for (int noise_num = 0; noise_num < vector_size(noises); noise_num++) {
//    struct Noise* noise = vector_get(noises, noise_num);
//    switch (noise->noise_type) {
//      case (RIDGED_FRACTAL_NOISE):
//        noise->ridged_fractal_noise.step = 1.0f / scale;
//        ridged_fractal_noise_eval_custom(&noise->ridged_fractal_noise, poss, dest);
//        break;
//    }
//  }
//}

//static inline void planet_normal_avx2(vec3 v, float dest[8], struct Vector* noises, int scale) {
//  float h = 0.001f;
//  float poss[8][3] = {{v.x + h, v.y, v.z}, {v.x - h, v.y, v.z}, {v.x, v.y + h, v.z}, {v.x, v.y - h, v.z}, {v.x, v.y, v.z + h}, {v.x, v.y, v.z - h}, {0}, {0}};
//
//  for (int noise_num = 0; noise_num < vector_size(noises); noise_num++) {
//    struct Noise* noise = vector_get(noises, noise_num);
//    switch (noise->noise_type) {
//      case (RIDGED_FRACTAL_NOISE):
//        noise->ridged_fractal_noise.step = 1.0f / scale;
//        ridged_fractal_noise_eval_custom(&noise->ridged_fractal_noise, poss, dest);
//        break;
//    }
//  }
//}

// TODO: Calculating normals can be much faster in a special noise function either by a single 8x1 array for SIMD with each position unique in 3D space
// OR calculate bulk in chunky 8*8 kernel sorta thing
#define FAST_NORMALS_AVX2 false
static inline vec3 planet_normal(vec3 v, struct NoiseModule* planet_shape, int scale) {
  //#if FAST_NORMALS_AVX2
  //  float dest[8] = {0};
  //  planet_normal_avx2(v, dest, noises, scale);
  //  vec3 gradient = (vec3){.x = dest[0] - dest[1], .y = dest[2] - dest[3], .z = dest[4] - dest[5]};
  //  gradient = vec3_old_skool_normalise(gradient);
  //  return gradient;
  //#else
  float h = 0.001f * scale;
  float dxp = noise_module_eval(planet_shape, v.x + h, v.y, v.z);
  float dxm = noise_module_eval(planet_shape, v.x - h, v.y, v.z);
  float dyp = noise_module_eval(planet_shape, v.x, v.y + h, v.z);
  float dym = noise_module_eval(planet_shape, v.x, v.y - h, v.z);
  float dzp = noise_module_eval(planet_shape, v.x, v.y, v.z + h);
  float dzm = noise_module_eval(planet_shape, v.x, v.y, v.z - h);
  vec3 gradient = (vec3){.x = dxp - dxm, .y = dyp - dym, .z = dzp - dzm};
  gradient = vec3_old_skool_normalise(gradient);
  return gradient;
  //#endif
}

void manifold_octree_construct_base(struct ManifoldOctreeNode* octree_node, int size, int scale, ivec3 position, struct NoiseModule* planet_shape);
void manifold_octree_destroy_octree(struct ManifoldOctreeNode* octree_node, struct Map* vertice_map);
void manifold_octree_generate_vertex_buffer(struct ManifoldOctreeNode* octree_node, struct Vector* vertices);
void manifold_octree_process_cell(struct ManifoldOctreeNode* octree_node, struct Vector* indexes, float threshold);
void manifold_octree_cluster_cell_base(struct ManifoldOctreeNode* octree_node, float error, struct NoiseModule* planet_shape);

#endif  // MANIFOLD_OCTREE_H
