#include "mana/graphics/dualcontouring/manifold/manifolddualcontouring.h"

//: base(device, resolution, size, true, !FlatShading, 2097152)
void manifold_dual_contouring_init(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api, struct Shader* shader, int resolution, int size) {
  manifold_dual_contouring->shader = shader;

  manifold_dual_contouring->mesh = calloc(1, sizeof(struct Mesh));
  mesh_manifold_dual_contouring_init(manifold_dual_contouring->mesh);

  manifold_dual_contouring->octree_size = size;
  manifold_dual_contouring->resolution = resolution;
}

static inline void manifold_dual_contouring_vulkan_cleanup(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api) {
  vkDestroyBuffer(gpu_api->vulkan_state->device, manifold_dual_contouring->index_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, manifold_dual_contouring->index_buffer_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, manifold_dual_contouring->vertex_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, manifold_dual_contouring->vertex_buffer_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, manifold_dual_contouring->lighting_uniform_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, manifold_dual_contouring->lighting_uniform_buffer_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, manifold_dual_contouring->dc_uniform_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, manifold_dual_contouring->dc_uniform_buffer_memory, NULL);
}

void manifold_dual_contouring_delete(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api) {
  manifold_dual_contouring_vulkan_cleanup(manifold_dual_contouring, gpu_api);

  // Search through octree for unique vertices
  struct Map vertice_map = {0};
  map_init(&vertice_map, sizeof(struct Vertex*));
  manifold_octree_destroy_octree(manifold_dual_contouring->tree, &vertice_map);
  const char* key;
  struct MapIter iter = map_iter();
  while ((key = map_next(&vertice_map, &iter)))
    free(*(char**)map_get(&vertice_map, key));
  map_delete(&vertice_map);

  for (int series_num = 1; series_num < MAX_MANIFOLD_OCTREE_LEVELS; series_num++)
    free(manifold_dual_contouring->node_cache[series_num]);

  mesh_delete(manifold_dual_contouring->mesh);
  free(manifold_dual_contouring->mesh);

  free(manifold_dual_contouring->tree);
  //noise_free(manifold_dual_contouring->noise_set);
}

static inline void manifold_dual_contouring_setup_buffers(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api) {
  graphics_utils_setup_vertex_buffer(gpu_api->vulkan_state, manifold_dual_contouring->mesh->vertices, &manifold_dual_contouring->vertex_buffer, &manifold_dual_contouring->vertex_buffer_memory);
  graphics_utils_setup_index_buffer(gpu_api->vulkan_state, manifold_dual_contouring->mesh->indices, &manifold_dual_contouring->index_buffer, &manifold_dual_contouring->index_buffer_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct ManifoldDualContouringUniformBufferObject), &manifold_dual_contouring->dc_uniform_buffer, &manifold_dual_contouring->dc_uniform_buffer_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct LightingUniformBufferObject), &manifold_dual_contouring->lighting_uniform_buffer, &manifold_dual_contouring->lighting_uniform_buffer_memory);
  graphics_utils_setup_descriptor(gpu_api->vulkan_state, manifold_dual_contouring->shader->descriptor_set_layout, manifold_dual_contouring->shader->descriptor_pool, &manifold_dual_contouring->descriptor_set);

  VkWriteDescriptorSet dcs[2] = {0};
  graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &manifold_dual_contouring->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ManifoldDualContouringUniformBufferObject), &manifold_dual_contouring->dc_uniform_buffer)});
  graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 1, &manifold_dual_contouring->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct LightingUniformBufferObject), &manifold_dual_contouring->lighting_uniform_buffer)});

  vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 2, dcs, 0, NULL);
}

void manifold_dual_contouring_recreate(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api) {
  manifold_dual_contouring_vulkan_cleanup(manifold_dual_contouring, gpu_api);
  manifold_dual_contouring_setup_buffers(manifold_dual_contouring, gpu_api);
}

void manifold_dual_contouring_contour(struct ManifoldDualContouring* manifold_dual_contouring, struct GPUAPI* gpu_api, struct Vector* noises, float threshold) {
  mesh_clear(manifold_dual_contouring->mesh);
  manifold_dual_contouring->tree = calloc(1, sizeof(struct ManifoldOctreeNode));
  manifold_dual_contouring->tree->index = 0;
  manifold_dual_contouring->tree->position = IVEC3_ZERO;
  manifold_dual_contouring->tree->type = MANIFOLD_NODE_INTERNAL;
  manifold_dual_contouring->tree->child_index = 0;

#if MANIFOLD_BENCHMARK
  double start_time, end_time;
  start_time = engine_get_time();
  manifold_octree_construct_base(manifold_dual_contouring->tree, manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, noises);
  end_time = engine_get_time();
  printf("Construct base time taken: %lf\n", end_time - start_time);
  // ~1.0 start
  // 0.119
  // 0.117
  // 0.075
  // Above is for 32 ^ 3 new standard is 64 ^ 3
  // ~8.0 start untested by based off above start
  // 0.5
  // 0.425
  // 0.34
  // 0.15

  start_time = engine_get_time();
  manifold_octree_cluster_cell_base(manifold_dual_contouring->tree, 0, noises);
  end_time = engine_get_time();
  printf("Cluster cell base time taken: %lf\n", end_time - start_time);
  // 0.11 start
  // 0.028

  start_time = engine_get_time();
  manifold_octree_generate_vertex_buffer(manifold_dual_contouring->tree, manifold_dual_contouring->mesh->vertices);
  end_time = engine_get_time();
  printf("Generate vertex buffer time taken: %lf\n", end_time - start_time);
  // 0.063 start
  // 0.02 Note: Ranges from 0.035-0.015
  // 0.014

  start_time = engine_get_time();
  manifold_octree_process_cell(manifold_dual_contouring->tree, manifold_dual_contouring->mesh->indices, threshold);
  end_time = engine_get_time();
  printf("Process cell time taken: %lf\n", end_time - start_time);
  // 0.027 start
  // 0.009

  //start_time = engine_get_time();
  ////manifold_octree_process_cell(manifold_dual_contouring->tree, manifold_dual_contouring->mesh->indices, threshold);
  //float STEP = 1.0 / 64.0f;
  //struct RidgedFractalNoise noise = {0};
  //ridged_fractal_noise_init(&noise);
  //noise.octave_count = 4;
  //noise.frequency = 1.0;
  //noise.lacunarity = 2.2324f;
  //noise.step = STEP;
  ////noise.parallel = true;
  //float* nopise = ridged_fractal_noise_eval_3d_avx2(&noise, 64, 64, 64);
  //end_time = engine_get_time();
  //printf("Gpu test: %lf\n", end_time - start_time);
#else
  for (int gen_octree = 0; gen_octree < 8; gen_octree++) {
    manifold_dual_contouring->tree->children[gen_octree] = calloc(1, sizeof(struct ManifoldOctreeNode));
    if (gen_octree == 0) {
      for (int gen_octree2 = 0; gen_octree2 < 8; gen_octree2++)
        manifold_dual_contouring->tree->children[gen_octree]->children[gen_octree2] = calloc(1, sizeof(struct ManifoldOctreeNode));
    }
  }

  // What I'm looking for TCornerDeltas
  int scale = 1;
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[0], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 0, .y = 0, .z = 0}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[1], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 0, .y = 0, .z = 64}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[2], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 0, .y = 64, .z = 0}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[3], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 0, .y = 64, .z = 64}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[4], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 64, .y = 0, .z = 0}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[5], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 64, .y = 0, .z = 64}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[6], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 64, .y = 64, .z = 0}, noises);
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[0]->children[7], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 64, .y = 64, .z = 64}, noises);

  scale = 8;
  manifold_octree_construct_base(manifold_dual_contouring->tree->children[1], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution, scale, (ivec3){.x = 0, .y = 0, .z = 128}, noises);
  //manifold_octree_construct_base(manifold_dual_contouring->tree->children[2], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution / 2, (ivec3){.x = 32, .y = -32, .z = -32}, noises);
  //manifold_octree_construct_base(manifold_dual_contouring->tree->children[3], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution / 2, (ivec3){.x = 32, .y = -32, .z = -32}, noises);
  //manifold_octree_construct_base(manifold_dual_contouring->tree->children[4], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution / 2, (ivec3){.x = 96, .y = -32, .z = -32}, noises);
  //manifold_octree_construct_base(manifold_dual_contouring->tree->children[5], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution / 2, (ivec3){.x = 96, .y = -32, .z = -32}, noises);
  //manifold_octree_construct_base(manifold_dual_contouring->tree->children[6], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution / 2, (ivec3){.x = 96, .y = -32, .z = -32}, noises);
  //manifold_octree_construct_base(manifold_dual_contouring->tree->children[7], manifold_dual_contouring->node_cache, manifold_dual_contouring->resolution / 2, (ivec3){.x = 96, .y = -32, .z = -32}, noises);

  manifold_dual_contouring->tree->children[0]->type = MANIFOLD_NODE_INTERNAL;

  manifold_dual_contouring->tree->children[1]->type = MANIFOLD_NODE_INTERNAL;
  manifold_dual_contouring->tree->children[2]->type = MANIFOLD_NODE_NONE;
  manifold_dual_contouring->tree->children[3]->type = MANIFOLD_NODE_NONE;
  manifold_dual_contouring->tree->children[4]->type = MANIFOLD_NODE_NONE;
  manifold_dual_contouring->tree->children[5]->type = MANIFOLD_NODE_NONE;
  manifold_dual_contouring->tree->children[6]->type = MANIFOLD_NODE_NONE;
  manifold_dual_contouring->tree->children[7]->type = MANIFOLD_NODE_NONE;

  manifold_octree_cluster_cell_base(manifold_dual_contouring->tree, 0, noises);                                      // Finds vertices
  manifold_octree_generate_vertex_buffer(manifold_dual_contouring->tree, manifold_dual_contouring->mesh->vertices);  // Finds normals and puts vertices in buffer
  manifold_octree_process_cell(manifold_dual_contouring->tree, manifold_dual_contouring->mesh->indices, threshold);  // Finds indices and puts indices in buffer
#endif

  if (vector_size(manifold_dual_contouring->mesh->vertices) > 0)
    manifold_dual_contouring_setup_buffers(manifold_dual_contouring, gpu_api);
}

//void manifold_dual_contouring_construct_tree_grid(struct ManifoldOctreeNode* node) {
//  if (node == NULL)
//    return;
//
//  int x = (int)node->position.x;
//  int y = (int)node->position.y;
//  int z = (int)node->position.z;
//  vec3 c = {.r = 0.0f, .g = 0.2f, .b = 0.8f};
//  vec3 v = {.r = 1.0f, .g = 0.0f, .b = 0.0f};
//
//  float size = node->size;
//
//  if (node->type == MANIFOLD_NODE_INTERNAL && array_list_size(node->vertices) == 0) {
//    for (int i = 0; i < 8; i++) {
//      manifold_dual_contouring_construct_tree_grid(node->children[i]);
//    }
//  }
//}
