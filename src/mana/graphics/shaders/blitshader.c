#include "mana/graphics/shaders/blitshader.h"

int blit_shader_init(struct BlitShader* blit_shader, struct VulkanState* vulkan_renderer, VkRenderPass render_pass, int descriptors) {
  blit_shader->shader = calloc(1, sizeof(struct Shader));

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

  if (vkCreateDescriptorSetLayout(vulkan_renderer->device, &layout_info, NULL, &blit_shader->shader->descriptor_set_layout) != VK_SUCCESS)
    return 0;

  VkDescriptorPoolSize pool_size = {0};
  pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_size.descriptorCount = descriptors;

  VkDescriptorPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 1;
  pool_info.pPoolSizes = &pool_size;
  pool_info.maxSets = descriptors;

  if (vkCreateDescriptorPool(vulkan_renderer->device, &pool_info, NULL, &blit_shader->shader->descriptor_pool) != VK_SUCCESS) {
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

  shader_init(blit_shader->shader, vulkan_renderer, "./assets/shaders/spirv/screenspace.vert.spv", "./assets/shaders/spirv/blit.frag.spv", NULL, vertex_input_info, render_pass, color_blending, VK_FRONT_FACE_CLOCKWISE, VK_FALSE, VK_SAMPLE_COUNT_1_BIT, false, VK_CULL_MODE_BACK_BIT);

  return 1;
}

void blit_shader_delete(struct BlitShader* blit_shader, struct VulkanState* vulkan_renderer) {
  vkDestroyDescriptorPool(vulkan_renderer->device, blit_shader->shader->descriptor_pool, NULL);
  shader_delete(blit_shader->shader, vulkan_renderer);
  free(blit_shader->shader);
}
