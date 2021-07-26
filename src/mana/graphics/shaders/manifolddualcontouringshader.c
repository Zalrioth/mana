#include "mana/graphics/shaders/manifolddualcontouringshader.h"

int manifold_dual_contouring_shader_init(struct ManifoldDualContouringShader* manifold_dual_countouring_shader, struct GPUAPI* gpu_api) {
  VkDescriptorSetLayoutBinding dcubo_layout_binding = {0};
  dcubo_layout_binding.binding = 0;
  dcubo_layout_binding.descriptorCount = 1;
  dcubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  dcubo_layout_binding.pImmutableSamplers = NULL;
  dcubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding lighting_layout_binding = {0};
  lighting_layout_binding.binding = 1;
  lighting_layout_binding.descriptorCount = 1;
  lighting_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  lighting_layout_binding.pImmutableSamplers = NULL;
  lighting_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[2] = {dcubo_layout_binding, lighting_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 2;
  layout_info.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(gpu_api->vulkan_state->device, &layout_info, NULL, &manifold_dual_countouring_shader->shader.descriptor_set_layout) != VK_SUCCESS)
    return 0;

  int sprite_descriptors = 1;
  VkDescriptorPoolSize pool_sizes[2] = {{0}};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = sprite_descriptors;  // Max number of uniform descriptors
  pool_sizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[1].descriptorCount = sprite_descriptors;  // Max number of image sampler descriptors

  VkDescriptorPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 2;  // Number of things being passed to GPU
  poolInfo.pPoolSizes = pool_sizes;
  poolInfo.maxSets = sprite_descriptors;  // Max number of sets made from this pool

  if (vkCreateDescriptorPool(gpu_api->vulkan_state->device, &poolInfo, NULL, &manifold_dual_countouring_shader->shader.descriptor_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool!\n");
    return 0;
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription binding_description = mesh_manifold_dual_contouring_get_binding_description();
  VkVertexInputAttributeDescription attribute_descriptions[MANIFOLD_DUAL_CONTOURING_VERTEX_ATTRIBUTES];
  memset(attribute_descriptions, 0, sizeof(attribute_descriptions));
  mesh_manifold_dual_contouring_get_attribute_descriptions(attribute_descriptions);

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = MANIFOLD_DUAL_CONTOURING_VERTEX_ATTRIBUTES;
  vertex_input_info.pVertexBindingDescriptions = &binding_description;
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

  // Note: Independent blending is for certain devices only, all attachments must blend the same otherwise
  VkPipelineColorBlendAttachmentState color_blend_attachments[MANIFOLD_DUAL_CONTOURING_COLOR_ATTACHEMENTS];
  memset(color_blend_attachments, 0, sizeof(VkPipelineColorBlendAttachmentState) * MANIFOLD_DUAL_CONTOURING_COLOR_ATTACHEMENTS);
  for (int pipeline_attachment_num = 0; pipeline_attachment_num < MANIFOLD_DUAL_CONTOURING_COLOR_ATTACHEMENTS; pipeline_attachment_num++) {
    color_blend_attachments[pipeline_attachment_num].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachments[pipeline_attachment_num].blendEnable = VK_FALSE;
    color_blend_attachments[pipeline_attachment_num].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
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
  color_blending.attachmentCount = MANIFOLD_DUAL_CONTOURING_COLOR_ATTACHEMENTS;
  color_blending.pAttachments = color_blend_attachments;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  shader_init(&manifold_dual_countouring_shader->shader, gpu_api->vulkan_state, "./assets/shaders/spirv/manifolddualcontouring.vert.spv", "./assets/shaders/spirv/manifolddualcontouring.frag.spv", NULL, vertex_input_info, gpu_api->vulkan_state->gbuffer->render_pass, color_blending, VK_FRONT_FACE_CLOCKWISE, VK_TRUE, gpu_api->vulkan_state->msaa_samples, true, VK_CULL_MODE_BACK_BIT);

  return 1;
}

void manifold_dual_contouring_shader_delete(struct ManifoldDualContouringShader* manifold_dual_countouring_shader, struct GPUAPI* gpu_api) {
  shader_delete(&manifold_dual_countouring_shader->shader, gpu_api->vulkan_state);

  vkDestroyDescriptorPool(gpu_api->vulkan_state->device, manifold_dual_countouring_shader->shader.descriptor_pool, NULL);
}
