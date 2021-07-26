#include "mana/graphics/shaders/atmosphericscattering.h"

int atmospheric_scattering_shader_init(struct AtmosphericScatteringShader* atmospheric_scattering_shader, struct GPUAPI* gpu_api) {
  atmospheric_scattering_shader->sun_angle = degree_to_radian(180.0f);

  double k_solar_irradiance[] =
      {
          1.11776, 1.14259, 1.01249, 1.14716, 1.72765, 1.73054, 1.6887, 1.61253,
          1.91198, 2.03474, 2.02042, 2.02212, 1.93377, 1.95809, 1.91686, 1.8298,
          1.8685, 1.8931, 1.85149, 1.8504, 1.8341, 1.8345, 1.8147, 1.78158, 1.7533,
          1.6965, 1.68194, 1.64654, 1.6048, 1.52143, 1.55622, 1.5113, 1.474, 1.4482,
          1.41018, 1.36775, 1.34188, 1.31429, 1.28303, 1.26758, 1.2367, 1.2082,
          1.18737, 1.14683, 1.12362, 1.1058, 1.07124, 1.04992};

  double k_ozone_cross_section[] =
      {
          1.18e-27, 2.182e-28, 2.818e-28, 6.636e-28, 1.527e-27, 2.763e-27, 5.52e-27,
          8.451e-27, 1.582e-26, 2.316e-26, 3.669e-26, 4.924e-26, 7.752e-26, 9.016e-26,
          1.48e-25, 1.602e-25, 2.139e-25, 2.755e-25, 3.091e-25, 3.5e-25, 4.266e-25,
          4.672e-25, 4.398e-25, 4.701e-25, 5.019e-25, 4.305e-25, 3.74e-25, 3.215e-25,
          2.662e-25, 2.238e-25, 1.852e-25, 1.473e-25, 1.209e-25, 9.423e-26, 7.455e-26,
          6.566e-26, 5.105e-26, 4.15e-26, 4.228e-26, 3.237e-26, 2.451e-26, 2.801e-26,
          2.534e-26, 1.624e-26, 1.465e-26, 2.078e-26, 1.383e-26, 7.105e-27};

  double kDobsonUnit = 2.687e20;
  double kMaxOzoneNumberDensity = 300.0 * kDobsonUnit / 15000.0;
  double kConstantSolarIrradiance = 1.5;
  double kTopRadius = 6420000.0;
  double kRayleigh = 1.24062e-6;
  double kRayleighScaleHeight = 8000.0;
  double kMieScaleHeight = 1200.0;
  double kMieAngstromAlpha = 0.0;
  double kMieAngstromBeta = 5.328e-3;
  double kMieSingleScatteringAlbedo = 0.9;
  double kMiePhaseFunctionG = 0.8;
  double kGroundAlbedo = 0.1;
  bool m_use_half_precision = false;
  double max_sun_zenith_angle = (m_use_half_precision ? 102.0 : 120.0) / 180.0 * UM_PI;

  //////////////////////////////////////////////////////////////////////

  atmospheric_scattering_shader->shader = calloc(1, sizeof(struct Shader));

  VkDescriptorSetLayoutBinding sampler_layout_binding = {0};
  sampler_layout_binding.binding = 0;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = NULL;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 1;
  layout_info.pBindings = &sampler_layout_binding;

  if (vkCreateDescriptorSetLayout(gpu_api->vulkan_state->device, &layout_info, NULL, &atmospheric_scattering_shader->shader->descriptor_set_layout) != VK_SUCCESS)
    return 0;

  VkDescriptorPoolSize pool_size = {0};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = 2;

  VkDescriptorPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  pool_info.maxSets = 2;

  if (vkCreateDescriptorPool(gpu_api->vulkan_state->device, &pool_info, NULL, &atmospheric_scattering_shader->shader->descriptor_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool!\n");
    return 0;
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription binding_description = mesh_sprite_get_binding_description();
  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = 0;  // Note: length of attributeDescriptions
  vertex_input_info.pVertexBindingDescriptions = &binding_description;
  vertex_input_info.pVertexAttributeDescriptions = NULL;

  VkPipelineColorBlendAttachmentState color_blend_attachment = {0};
  color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  color_blend_attachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo color_blending = {0};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = 1;
  color_blending.pAttachments = &color_blend_attachment;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  shader_init(atmospheric_scattering_shader->shader, gpu_api->vulkan_state, "./assets/shaders/spirv/screenspace.vert.spv", "./assets/shaders/spirv/atmosphericscattering.frag.spv", NULL, vertex_input_info, gpu_api->vulkan_state->gbuffer->render_pass, color_blending, VK_FRONT_FACE_CLOCKWISE, VK_FALSE, VK_SAMPLE_COUNT_1_BIT, false, VK_CULL_MODE_BACK_BIT);

  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct AtmosphericScatteringUniformBufferObject), &atmospheric_scattering_shader->uniform_buffer, &atmospheric_scattering_shader->uniform_buffer_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct AtmosphericScatteringUniformBufferObjectSettings), &atmospheric_scattering_shader->uniform_buffer_settings, &atmospheric_scattering_shader->uniform_buffer_settings_memory);
  graphics_utils_setup_descriptor(gpu_api->vulkan_state, atmospheric_scattering_shader->shader->descriptor_set_layout, atmospheric_scattering_shader->shader->descriptor_pool, &atmospheric_scattering_shader->descriptor_set);

  VkWriteDescriptorSet dcs[2] = {0};
  graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &atmospheric_scattering_shader->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct AtmosphericScatteringUniformBufferObject), &atmospheric_scattering_shader->uniform_buffer)});
  graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 1, &atmospheric_scattering_shader->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct AtmosphericScatteringUniformBufferObjectSettings), &atmospheric_scattering_shader->uniform_buffer_settings)});
  //vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 2, dcs, 0, NULL);

  atmospheric_scattering_shader->fullscreen_triangle = calloc(1, sizeof(struct FullscreenTriangle));
  fullscreen_triangle_init(atmospheric_scattering_shader->fullscreen_triangle, gpu_api);

  return 0;
}

void atmospheric_scattering_shader_delete(struct AtmosphericScatteringShader* atmospheric_scattering_shader, struct GPUAPI* gpu_api) {
  vkDestroyBuffer(gpu_api->vulkan_state->device, atmospheric_scattering_shader->uniform_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, atmospheric_scattering_shader->uniform_buffer_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, atmospheric_scattering_shader->uniform_buffer_settings, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, atmospheric_scattering_shader->uniform_buffer_settings_memory, NULL);

  fullscreen_triangle_delete(atmospheric_scattering_shader->fullscreen_triangle, gpu_api);
  free(atmospheric_scattering_shader->fullscreen_triangle);

  vkDestroyDescriptorPool(gpu_api->vulkan_state->device, atmospheric_scattering_shader->shader->descriptor_pool, NULL);

  shader_delete(atmospheric_scattering_shader->shader, gpu_api->vulkan_state);
  free(atmospheric_scattering_shader->shader);
}

void atmospheric_scattering_shader_render(struct AtmosphericScatteringShader* atmospheric_scattering_shader, struct GPUAPI* gpu_api) {
  //struct AtmosphericScatteringUniformBufferObject ubo = {{{0}}};
  //
  //glm_mat4_copy(gpu_api->vulkan_state->gbuffer->projection_matrix, ubo.proj);
  //glm_mat4_copy(gpu_api->vulkan_state->gbuffer->view_matrix, ubo.view);
  //glm_mat4_identity(ubo.model);
  //
  //glm_rotate(ubo.model, time / 4, (vec3){0.0f, 0.0f + entity_num / 3.14159265358979, 1.0f});
  //glm_translate(ubo.model, (vec3){0.0f + entity_num / 3.14159265358979, 0.0f + entity_num / 3.14159265358979, 0.0f + entity_num / 3.14159265358979});
  //ubo.proj[1][1] *= -1;
  //
  //void* data;
  //vkMapMemory(gpu_api->vulkan_state->device, ((struct Sprite*)array_list_get(&game->sprites, entity_num))->uniform_buffers_memory, 0, sizeof(struct AtmosphericScatteringUniformBufferObject), 0, &data);
  //memcpy(data, &ubo, sizeof(struct UniformBufferObject));
  //vkUnmapMemory(gpu_api->vulkan_state->device, ((struct Sprite*)array_list_get(&game->sprites, entity_num))->uniform_buffers_memory);
  //
  ////////////////////////////////////////////////////////////////////////////////////
  //
  //post_process_start(gpu_api->vulkan_state->post_process, gpu_api->vulkan_state);
  //
  //vkCmdBindPipeline(gpu_api->vulkan_state->post_process->post_process_command_buffers[gpu_api->vulkan_state->post_process->ping_pong], VK_PIPELINE_BIND_POINT_GRAPHICS, atmospheric_scattering_shader->shader->graphics_pipeline);
  //VkBuffer vertex_buffers[] = {atmospheric_scattering_shader->fullscreen_quad->vertex_buffer};
  //VkDeviceSize offsets[] = {0};
  //vkCmdBindVertexBuffers(gpu_api->vulkan_state->post_process->post_process_command_buffers[gpu_api->vulkan_state->post_process->ping_pong], 0, 1, vertex_buffers, offsets);
  //vkCmdBindIndexBuffer(gpu_api->vulkan_state->post_process->post_process_command_buffers[gpu_api->vulkan_state->post_process->ping_pong], atmospheric_scattering_shader->fullscreen_quad->index_buffer, 0, VK_INDEX_TYPE_UINT32);
  //vkCmdBindDescriptorSets(gpu_api->vulkan_state->post_process->post_process_command_buffers[gpu_api->vulkan_state->post_process->ping_pong], VK_PIPELINE_BIND_POINT_GRAPHICS, atmospheric_scattering_shader->shader->pipeline_layout, 0, 1, &atmospheric_scattering_shader->descriptor_sets[gpu_api->vulkan_state->post_process->ping_pong ^ true], 0, NULL);
  //vkCmdDrawIndexed(gpu_api->vulkan_state->post_process->post_process_command_buffers[gpu_api->vulkan_state->post_process->ping_pong], atmospheric_scattering_shader->fullscreen_quad->mesh->indices->size, 1, 0, 0, 0);
  //
  //post_process_stop(gpu_api->vulkan_state->post_process, gpu_api->vulkan_state);
}
