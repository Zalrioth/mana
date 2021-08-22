#include "mana/graphics/entities/manifoldplanet.h"

void manifold_planet_init(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api, size_t octree_size, struct Shader* shader, struct Vector* noises, vec3 position) {
  planet->planet_type = MANIFOLD_ROUND_PLANET;
  planet->terrain_shader = shader;
  planet->position = position;
  planet->noises = noises;
  // Think the 14 here for "size" is meant to represent matrix scaling but hasn't been added yet
  manifold_dual_contouring_init(&planet->manifold_dual_contouring, gpu_api, shader, octree_size, 14);
  manifold_dual_contouring_contour(&planet->manifold_dual_contouring, gpu_api, noises, 0.0f);
}

void manifold_planet_delete(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api) {
  manifold_dual_contouring_delete(&planet->manifold_dual_contouring, gpu_api);
}

void manifold_planet_render(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api) {
  if (vector_size(planet->manifold_dual_contouring.mesh->vertices) == 0)
    return;

  vkCmdBindPipeline(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, planet->terrain_shader->graphics_pipeline);
  VkBuffer vertex_buffers[] = {planet->manifold_dual_contouring.vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, planet->manifold_dual_contouring.index_buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, planet->terrain_shader->pipeline_layout, 0, 1, &planet->manifold_dual_contouring.descriptor_set, 0, NULL);
  vkCmdDrawIndexed(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, planet->manifold_dual_contouring.mesh->indices->size, 1, 0, 0, 0);
}

// TODO: Pass lights and sun position?
void manifold_planet_update_uniforms(struct ManifoldPlanet* planet, struct GPUAPI* gpu_api, struct Camera* camera, vec3 light_pos) {
  if (vector_size(planet->manifold_dual_contouring.mesh->vertices) == 0)
    return;

  struct ManifoldDualContouringUniformBufferObject dcubo = {{{0}}};
  dcubo.proj = gpu_api->vulkan_state->gbuffer->projection_matrix;
  dcubo.proj.m11 *= -1;

  dcubo.view = gpu_api->vulkan_state->gbuffer->view_matrix;

  dcubo.model = mat4_translate(MAT4_IDENTITY, planet->position);

  dcubo.camera_pos = camera->position;

  void* terrain_data;
  vkMapMemory(gpu_api->vulkan_state->device, planet->manifold_dual_contouring.dc_uniform_buffer_memory, 0, sizeof(struct ManifoldDualContouringUniformBufferObject), 0, &terrain_data);
  memcpy(terrain_data, &dcubo, sizeof(struct ManifoldDualContouringUniformBufferObject));
  vkUnmapMemory(gpu_api->vulkan_state->device, planet->manifold_dual_contouring.dc_uniform_buffer_memory);

  struct LightingUniformBufferObject light_ubo = {{0}};
  light_ubo.direction = light_pos;
  light_ubo.ambient_color = (vec3){.data[0] = 1.0f, .data[1] = 1.0f, .data[2] = 1.0f};
  light_ubo.diffuse_colour = (vec3){.data[0] = 1.0f, .data[1] = 1.0f, .data[2] = 1.0f};
  light_ubo.specular_colour = (vec3){.data[0] = 1.0f, .data[1] = 1.0f, .data[2] = 1.0f};

  void* lighting_data;
  vkMapMemory(gpu_api->vulkan_state->device, planet->manifold_dual_contouring.lighting_uniform_buffer_memory, 0, sizeof(struct LightingUniformBufferObject), 0, &lighting_data);
  memcpy(lighting_data, &light_ubo, sizeof(struct LightingUniformBufferObject));
  vkUnmapMemory(gpu_api->vulkan_state->device, planet->manifold_dual_contouring.lighting_uniform_buffer_memory);
}
