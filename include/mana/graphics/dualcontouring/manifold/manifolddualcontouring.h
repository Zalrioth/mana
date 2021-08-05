#pragma once
#ifndef MANIFOLD_DUAL_CONTOURING_H
#define MANIFOLD_DUAL_CONTOURING_H

#include "mana/core/memoryallocator.h"
//
#include <cstorage/cstorage.h>
#include <math.h>
#include <ubermath/ubermath.h>

#include "mana/core/engine.h"
#include "mana/graphics/dualcontouring/manifold/manifoldoctree.h"
#include "mana/graphics/dualcontouring/manifold/manifoldtables.h"
#include "mana/graphics/dualcontouring/qef.h"
#include "mana/graphics/utilities/mesh.h"

#define MANIFOLD_BENCHMARK true

struct ManifoldDualContouringUniformBufferObject {
  alignas(32) mat4 model;
  alignas(32) mat4 view;
  alignas(32) mat4 proj;
  alignas(32) vec3 camera_pos;
};

struct ManifoldDualContouring {
  int resolution;
  int octree_size;
  struct ManifoldOctreeNode* tree;
  struct ManifoldOctreeNode* node_cache[MAX_MANIFOLD_OCTREE_LEVELS];
  struct ArrayList* vertice_list;

  struct Shader* shader;
  struct Mesh* mesh;
  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;
  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;
  VkBuffer dc_uniform_buffer;
  VkDeviceMemory dc_uniform_buffer_memory;
  VkBuffer lighting_uniform_buffer;
  VkDeviceMemory lighting_uniform_buffer_memory;
  VkDescriptorSet descriptor_set;
};

void manifold_dual_contouring_init(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api, struct Shader* shader, int resolution, int size);
void manifold_dual_contouring_delete(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api);
void manifold_dual_contouring_recreate(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api);
void manifold_dual_contouring_contour(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api, struct Vector* noises, float threshold);
void manifold_dual_contouring_construct_tree_grid(struct ManifoldOctreeNode* node);
vec3 manifold_dual_contouring_get_normal_q(struct Vector* verts, int indexes[6], int index_length);

#endif  // MANIFOLD_DUAL_CONTOURING_H
