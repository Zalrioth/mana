#include "mana/graphics/shaders/spriteshader.h"

int sprite_shader_init(struct SpriteShader* sprite_shader, struct GPUAPI* gpu_api, int depth_test) {
  VkDescriptorSetLayoutBinding ubo_layout_binding = {0};
  ubo_layout_binding.binding = 0;
  ubo_layout_binding.descriptorCount = 1;
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.pImmutableSamplers = NULL;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  //ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding sampler_layout_binding = {0};
  sampler_layout_binding.binding = 1;
  sampler_layout_binding.descriptorCount = 1;
  sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  sampler_layout_binding.pImmutableSamplers = NULL;
  sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[2] = {ubo_layout_binding, sampler_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 2;
  layout_info.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(gpu_api->vulkan_state->device, &layout_info, NULL, &sprite_shader->shader.descriptor_set_layout) != VK_SUCCESS)
    return 0;

  int sprite_descriptors = 2048;
  VkDescriptorPoolSize pool_sizes[2] = {{0}};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = sprite_descriptors;  // Max number of uniform descriptors
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  pool_sizes[1].descriptorCount = sprite_descriptors;  // Max number of image sampler descriptors

  VkDescriptorPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 2;  // Number of things being passed to GPU
  pool_info.pPoolSizes = pool_sizes;
  pool_info.maxSets = sprite_descriptors;  // Max number of sets made from this pool

  if (vkCreateDescriptorPool(gpu_api->vulkan_state->device, &pool_info, NULL, &sprite_shader->shader.descriptor_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool!\n");
    return 0;
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription binding_description = mesh_sprite_get_binding_description();
  VkVertexInputAttributeDescription attribute_descriptions[SPRITE_SHADER_VERTEX_ATTRIBUTES];
  memset(attribute_descriptions, 0, sizeof(attribute_descriptions));
  mesh_sprite_get_attribute_descriptions(attribute_descriptions);

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = SPRITE_SHADER_VERTEX_ATTRIBUTES;
  vertex_input_info.pVertexBindingDescriptions = &binding_description;
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

  // Note: Independent blending is for certain devices only, all attachments must blend the same otherwise
  VkPipelineColorBlendAttachmentState color_blend_attachments[SPRITE_SHADER_COLOR_ATTACHEMENTS];
  memset(color_blend_attachments, 0, sizeof(VkPipelineColorBlendAttachmentState) * SPRITE_SHADER_COLOR_ATTACHEMENTS);
  for (int pipeline_attachment_num = 0; pipeline_attachment_num < SPRITE_SHADER_COLOR_ATTACHEMENTS; pipeline_attachment_num++) {
    color_blend_attachments[pipeline_attachment_num].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachments[pipeline_attachment_num].blendEnable = VK_TRUE;
    color_blend_attachments[pipeline_attachment_num].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachments[pipeline_attachment_num].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    color_blend_attachments[pipeline_attachment_num].colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachments[pipeline_attachment_num].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachments[pipeline_attachment_num].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachments[pipeline_attachment_num].alphaBlendOp = VK_BLEND_OP_ADD;
  }

  VkPipelineColorBlendStateCreateInfo color_blending = {0};
  color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  color_blending.logicOpEnable = VK_FALSE;
  color_blending.logicOp = VK_LOGIC_OP_COPY;
  color_blending.attachmentCount = SPRITE_SHADER_COLOR_ATTACHEMENTS;
  color_blending.pAttachments = color_blend_attachments;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  shader_init(&sprite_shader->shader, gpu_api->vulkan_state, "./assets/shaders/spirv/sprite.vert.spv", "./assets/shaders/spirv/sprite.frag.spv", NULL, vertex_input_info, gpu_api->vulkan_state->gbuffer->render_pass, color_blending, VK_FRONT_FACE_COUNTER_CLOCKWISE, depth_test, gpu_api->vulkan_state->msaa_samples, true, VK_CULL_MODE_BACK_BIT);

  return 1;
}

void sprite_shader_delete(struct SpriteShader* sprite_shader, struct GPUAPI* gpu_api) {
  shader_delete(&sprite_shader->shader, gpu_api->vulkan_state);

  vkDestroyDescriptorPool(gpu_api->vulkan_state->device, sprite_shader->shader.descriptor_pool, NULL);
}
