#pragma once
#ifndef SHADER_H
#define SHADER_H

#include "mana/core/memoryallocator.h"
//
#include <mana/core/gpuapi.h>

#include "mana/core/corecommon.h"
#include "mana/core/fileio.h"
#include "mana/graphics/render/vulkanrenderer.h"

struct VulkanState;

struct Shader {
  struct VkPipelineLayout_T* pipeline_layout;
  struct VkPipeline_T* graphics_pipeline;
  struct VkDescriptorPool_T* descriptor_pool;
  struct VkDescriptorSetLayout_T* descriptor_set_layout;
};

// TODO: Create struct for settings to reduce parameters
// NOTE: Geometry shaders disabled because they suck
int shader_init(struct Shader* shader, struct VulkanState* vulkan_renderer, char* vertex_shader, char* fragment_shader, char* compute_shader, VkPipelineVertexInputStateCreateInfo vertex_input_info, VkRenderPass render_pass, VkPipelineColorBlendStateCreateInfo color_blending, VkFrontFace direction, bool depth_test, VkSampleCountFlagBits num_samples, bool supersampled, VkCullModeFlags cull_mode);
int shader_init_comp(struct Shader* shader, struct VulkanState* vulkan_renderer, char* compute_shader);
void shader_delete(struct Shader* shader, struct VulkanState* vulkan_renderer);
VkShaderModule shader_create_shader_module(struct VulkanState* vulkan_renderer, const char* code, int length);
int sprite_create_sprite_descriptor_pool(struct Shader* shader, struct VulkanState* vulkan_renderer);

#endif  // SHADER_H
