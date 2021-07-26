#include "mana/graphics/entities/grass.h"

int grass_init(struct Grass* grass, struct GPUAPI* gpu_api) {
  //graphics_utils_setup_vertex_buffer_pool(gpu_api->vulkan_state, grass->mesh->vertices, MAX_GRASS_NUMS, &grass->vertex_buffer, &grass->vertex_buffer_memory);
  //graphics_utils_setup_index_buffer_pool(gpu_api->vulkan_state, grass->mesh->indices, MAX_GRASS_NUMS, &grass->index_buffer, &grass->index_buffer_memory);
  //graphics_utils_create_buffer(gpu_api->vulkan_state->device, gpu_api->vulkan_state->physical_device, sizeof(struct out_draw_grass_vertices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, grass->vertex_buffer, grass->vertex_buffer_memory);
  //graphics_utils_create_buffer(gpu_api->vulkan_state->device, gpu_api->vulkan_state->physical_device, sizeof(struct out_draw_grass_indices), VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, grass->index_buffer, grass->index_buffer_memory);

  grass_shader_init(&grass->grass_shader, gpu_api);

  grass->compute_once = 0;

  VkBufferCreateInfo buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = sizeof(struct out_draw_grass_vertices);
  buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(gpu_api->vulkan_state->device, &buffer_info, NULL, &grass->vertex_buffer);
  vkBindBufferMemory(gpu_api->vulkan_state->device, grass->vertex_buffer, grass->grass_shader.grass_compute_memory[1], 0);

  VkBufferCreateInfo buffer_info2 = {0};
  buffer_info2.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info2.size = sizeof(struct out_draw_grass_indices);
  buffer_info2.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  buffer_info2.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(gpu_api->vulkan_state->device, &buffer_info2, NULL, &grass->index_buffer);
  vkBindBufferMemory(gpu_api->vulkan_state->device, grass->index_buffer, grass->grass_shader.grass_compute_memory[2], 0);

  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct GrassUniformBufferObject), &grass->uniform_buffer, &grass->uniform_buffers_memory);
  graphics_utils_setup_descriptor(gpu_api->vulkan_state, grass->grass_shader.grass_render_shader.descriptor_set_layout, grass->grass_shader.grass_render_shader.descriptor_pool, &grass->descriptor_set);

  VkWriteDescriptorSet dcs[1] = {0};
  graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &grass->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct GrassUniformBufferObject), &grass->uniform_buffer)});
  vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 1, dcs, 0, NULL);

  vector_init(&grass->grass_nodes, sizeof(vec4));

  for (int loop_num = 0; loop_num < 10; loop_num++) {
    vec4 grass_rand = {randf() * 10, randf() * 10, randf() * 10, 0.0f};
    vector_push_back(&grass->grass_nodes, &grass_rand);
  }

  return 1;
}

static inline void grass_vulkan_cleanup(struct Grass* grass, struct GPUAPI* gpu_api) {
  vkDestroyBuffer(gpu_api->vulkan_state->device, grass->index_buffer, NULL);
  vkDestroyBuffer(gpu_api->vulkan_state->device, grass->vertex_buffer, NULL);
  vkDestroyBuffer(gpu_api->vulkan_state->device, grass->uniform_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, grass->uniform_buffers_memory, NULL);
}

void grass_delete(struct Grass* grass, struct VulkanState* vulkan_state) {
  grass_shader_delete(&grass->grass_shader, vulkan_state);

  vkDestroyBuffer(vulkan_state->device, grass->index_buffer, NULL);
  vkDestroyBuffer(vulkan_state->device, grass->vertex_buffer, NULL);
}

void grass_render(struct Grass* grass, struct GPUAPI* gpu_api) {
  if (grass->compute_once == 0) {
    grass->compute_once = 1;
    const uint32_t elements = grass->grass_nodes.size;

    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(grass->grass_shader.commandBuffer, &beginInfo);

    vkCmdBindPipeline(grass->grass_shader.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, grass->grass_shader.grass_compute_shader.graphics_pipeline);
    vkCmdBindDescriptorSets(grass->grass_shader.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, grass->grass_shader.grass_compute_shader.pipeline_layout, 0, 1, &grass->grass_shader.descriptorSet, 0, NULL);
    vkCmdDispatch(grass->grass_shader.commandBuffer, elements, 1, 1);
    vkEndCommandBuffer(grass->grass_shader.commandBuffer);

    void* data1 = NULL;
    vkMapMemory(gpu_api->vulkan_state->device, grass->grass_shader.grass_compute_memory[0], 0, (sizeof(unsigned int) * 4) + (sizeof(vec4) * elements), 0, &data1);

    struct in_grass_vertices* d_a = data1;

    d_a->total_grass_vertices = elements;
    memcpy(d_a->grass_vertices, grass->grass_nodes.items, grass->grass_nodes.memory_size * elements);
    d_a->total_draw_grass_vertices = 0;
    d_a->total_draw_grass_indices = 0;

    vkUnmapMemory(gpu_api->vulkan_state->device, grass->grass_shader.grass_compute_memory[0]);

    // Return stuff
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &grass->grass_shader.commandBuffer;
    vkQueueSubmit(gpu_api->vulkan_state->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(gpu_api->vulkan_state->graphics_queue);

    data1 = NULL;
    vkMapMemory(gpu_api->vulkan_state->device, grass->grass_shader.grass_compute_memory[0], 0, sizeof(ivec3), 0, &data1);
    d_a = data1;
    grass->index_size = d_a->total_draw_grass_indices;
    vkUnmapMemory(gpu_api->vulkan_state->device, grass->grass_shader.grass_compute_memory[0]);
  }

  vkCmdBindPipeline(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grass->grass_shader.grass_render_shader.graphics_pipeline);

  VkBuffer vertex_buffers[] = {grass->vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, grass->index_buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, grass->grass_shader.grass_render_shader.pipeline_layout, 0, 1, &grass->descriptor_set, 0, NULL);
  vkCmdDrawIndexed(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, grass->index_size, 1, 0, 0, 0);
}

void grass_update_uniforms(struct Grass* grass, struct GPUAPI* gpu_api) {
  struct GrassUniformBufferObject ubos = {{{0}}};
  ubos.proj = gpu_api->vulkan_state->gbuffer->projection_matrix;
  ubos.proj.vecs[1].data[1] *= -1;

  ubos.view = gpu_api->vulkan_state->gbuffer->view_matrix;

  ubos.model = MAT4_IDENTITY;

  void* data;
  vkMapMemory(gpu_api->vulkan_state->device, grass->uniform_buffers_memory, 0, sizeof(struct GrassUniformBufferObject), 0, &data);
  memcpy(data, &ubos, sizeof(struct GrassUniformBufferObject));
  vkUnmapMemory(gpu_api->vulkan_state->device, grass->uniform_buffers_memory);
}

//void grass_recreate(struct Grass* grass, struct GPUAPI* gpu_api) {
//  grass_shader_delete(&grass->grass_shader, gpu_api);
//  grass_shader_init(&grass->grass_shader, gpu_api);
//
//  grass_vulkan_cleanup(grass, gpu_api);
//
//  graphics_utils_setup_vertex_buffer(gpu_api->vulkan_state, grass->mesh->vertices, &grass->vertex_buffer, &grass->vertex_buffer_memory);
//  graphics_utils_setup_index_buffer(gpu_api->vulkan_state, grass->mesh->indices, &grass->index_buffer, &grass->index_buffer_memory);
//  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct GrassUniformBufferObject), &grass->uniform_buffer, &grass->uniform_buffers_memory);
//  graphics_utils_setup_descriptor(gpu_api->vulkan_state, grass->grass_shader.grass_render_shader.descriptor_set_layout, grass->grass_shader.grass_render_shader.descriptor_pool, &grass->descriptor_set);
//
//  VkWriteDescriptorSet dcs[1] = {0};
//  graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &grass->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct GrassUniformBufferObject), &grass->uniform_buffer)});
//  vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 1, dcs, 0, NULL);
//}
