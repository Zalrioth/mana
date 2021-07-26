#pragma once
#ifndef GRASS_H
#define GRASS_H

#include "mana/core/memoryallocator.h"
//
#include "mana/graphics/shaders/grassshader.h"

// Each chunk has its own grass buffer and render pass
// LOD based on chunk distance
// Should be fast to copy memory chunks

struct GrassUniformBufferObject {
  alignas(16) mat4 model;
  alignas(16) mat4 view;
  alignas(16) mat4 proj;
};

struct Grass {
  struct GrassShader grass_shader;

  VkBuffer vertex_buffer;
  VkBuffer index_buffer;
  VkBuffer uniform_buffer;
  VkDeviceMemory uniform_buffers_memory;
  VkDescriptorSet descriptor_set;

  struct Vector grass_nodes;

  int compute_once;
  unsigned int index_size;
};

int grass_init(struct Grass* grass, struct GPUAPI* gpu_api);
void grass_delete(struct Grass* grass, struct VulkanState* vulkan_renderer);
void grass_render(struct Grass* grass, struct GPUAPI* gpu_api);
void grass_update_uniforms(struct Grass* grass, struct GPUAPI* gpu_api);

#endif  // GRASS_H
