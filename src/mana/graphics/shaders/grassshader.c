#include "mana/graphics/shaders/grassshader.h"

static inline uint32_t findMemoryType(struct GPUAPI* gpu_api, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(gpu_api->vulkan_state->physical_device, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  return 1;
}

int grass_shader_init(struct GrassShader* grass_shader, struct GPUAPI* gpu_api) {
  // Compute shader
  VkDescriptorSetLayoutBinding layout_bindings[3] = {0};
  for (uint32_t i = 0; i < 3; i++) {
    VkDescriptorSetLayoutBinding layout_binding = {0};
    layout_binding.binding = i;
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layout_binding.descriptorCount = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    layout_bindings[i] = layout_binding;
  }

  VkDescriptorSetLayoutCreateInfo set_layout_create_info = {0};
  set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set_layout_create_info.bindingCount = 3;
  set_layout_create_info.pBindings = layout_bindings;

  if (vkCreateDescriptorSetLayout(gpu_api->vulkan_state->device, &set_layout_create_info, NULL, &grass_shader->grass_compute_shader.descriptor_set_layout) != VK_SUCCESS)
    return -1;

  shader_init_comp(&grass_shader->grass_compute_shader, gpu_api->vulkan_state, "./assets/shaders/spirv/grassvert.comp.spv");

  grass_shader->buf_size[0] = sizeof(struct in_grass_vertices);
  grass_shader->buf_size[1] = sizeof(struct out_draw_grass_vertices);
  grass_shader->buf_size[2] = sizeof(struct out_draw_grass_indices);

  // Create buffers
  for (int buff = 0; buff < 3; buff++) {
    VkBufferCreateInfo bufferCreateInfo = {0};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = grass_shader->buf_size[buff];  // TODO: Do correct size
    bufferCreateInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = (&gpu_api->vulkan_state->indices)->graphics_family;

    if (vkCreateBuffer(gpu_api->vulkan_state->device, &bufferCreateInfo, NULL, &grass_shader->grass_compute_buffers[buff]) != VK_SUCCESS)
      return 1;
  }

  // Allocate buffers
  for (int buff = 0; buff < 3; buff++) {
    VkMemoryRequirements bufferMemoryRequirements = {0};
    vkGetBufferMemoryRequirements(gpu_api->vulkan_state->device, grass_shader->grass_compute_buffers[buff], &bufferMemoryRequirements);
    uint32_t typeFilter = bufferMemoryRequirements.memoryTypeBits;

    uint32_t memoryTypeIndex = findMemoryType(gpu_api, typeFilter, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkMemoryAllocateInfo allocateInfo = {0};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = bufferMemoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(gpu_api->vulkan_state->device, &allocateInfo, NULL, &grass_shader->grass_compute_memory[buff]) != VK_SUCCESS)
      return 1;

    if (vkBindBufferMemory(gpu_api->vulkan_state->device, grass_shader->grass_compute_buffers[buff], grass_shader->grass_compute_memory[buff], 0) != VK_SUCCESS)
      return 1;
  }

  // Allocate descriptors
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {0};
  descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.maxSets = 1;

  VkDescriptorPoolSize poolSize = {0};
  poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  poolSize.descriptorCount = 3;

  descriptorPoolCreateInfo.poolSizeCount = 1;
  descriptorPoolCreateInfo.pPoolSizes = &poolSize;
  if (vkCreateDescriptorPool(gpu_api->vulkan_state->device, &descriptorPoolCreateInfo, NULL, &grass_shader->grass_compute_shader.descriptor_pool) != VK_SUCCESS)
    return 1;

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {0};
  descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = grass_shader->grass_compute_shader.descriptor_pool;
  descriptorSetAllocateInfo.descriptorSetCount = 1;
  descriptorSetAllocateInfo.pSetLayouts = &grass_shader->grass_compute_shader.descriptor_set_layout;

  if (vkAllocateDescriptorSets(gpu_api->vulkan_state->device, &descriptorSetAllocateInfo, &grass_shader->descriptorSet) != VK_SUCCESS)
    return 1;

  VkWriteDescriptorSet descriptorSetWrites[3] = {0};
  VkDescriptorBufferInfo bufferInfos[3] = {0};

  uint32_t i = 0;
  for (int buff = 0; buff < 3; buff++) {
    VkWriteDescriptorSet writeDescriptorSet = {0};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = grass_shader->descriptorSet;
    writeDescriptorSet.dstBinding = i;
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;
    writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

    VkDescriptorBufferInfo buffInfo = {0};
    buffInfo.buffer = grass_shader->grass_compute_buffers[buff];
    buffInfo.offset = 0;
    buffInfo.range = VK_WHOLE_SIZE;
    bufferInfos[i] = buffInfo;

    writeDescriptorSet.pBufferInfo = &bufferInfos[i];
    descriptorSetWrites[i] = writeDescriptorSet;
    i++;
  }

  vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 3, descriptorSetWrites, 0, NULL);

  // Create compute command buffer
  VkCommandPoolCreateInfo commandPoolCreateInfo = {0};
  commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  commandPoolCreateInfo.queueFamilyIndex = gpu_api->vulkan_state->indices.graphics_family;

  if (vkCreateCommandPool(gpu_api->vulkan_state->device, &commandPoolCreateInfo, NULL, &grass_shader->commandPool) != VK_SUCCESS)
    return 1;

  VkCommandBufferAllocateInfo commandBufferAllocateInfo = {0};
  commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  commandBufferAllocateInfo.commandPool = grass_shader->commandPool;
  commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  commandBufferAllocateInfo.commandBufferCount = 1;

  if (vkAllocateCommandBuffers(gpu_api->vulkan_state->device, &commandBufferAllocateInfo, &grass_shader->commandBuffer) != VK_SUCCESS)
    return 1;

  /////////////////////////////////////////////////////
  // Render shader

  VkDescriptorSetLayoutBinding ubo_layout_binding = {0};
  ubo_layout_binding.binding = 0;
  ubo_layout_binding.descriptorCount = 1;
  ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  ubo_layout_binding.pImmutableSamplers = NULL;
  ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  //ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutBinding bindings[1] = {ubo_layout_binding};
  VkDescriptorSetLayoutCreateInfo layout_info = {0};
  layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layout_info.bindingCount = 1;
  layout_info.pBindings = bindings;

  if (vkCreateDescriptorSetLayout(gpu_api->vulkan_state->device, &layout_info, NULL, &grass_shader->grass_render_shader.descriptor_set_layout) != VK_SUCCESS)
    return 0;

  int sprite_descriptors = 1;
  VkDescriptorPoolSize pool_sizes[1] = {{0}};
  pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  pool_sizes[0].descriptorCount = sprite_descriptors;  // Max number of uniform descriptors

  VkDescriptorPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.poolSizeCount = 1;  // Number of things being passed to GPU
  pool_info.pPoolSizes = pool_sizes;
  pool_info.maxSets = sprite_descriptors;  // Max number of sets made from this pool

  if (vkCreateDescriptorPool(gpu_api->vulkan_state->device, &pool_info, NULL, &grass_shader->grass_render_shader.descriptor_pool) != VK_SUCCESS) {
    fprintf(stderr, "failed to create descriptor pool!\n");
    return 0;
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_info = {0};
  vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  VkVertexInputBindingDescription binding_description = mesh_grass_get_binding_description();
  VkVertexInputAttributeDescription attribute_descriptions[GRASS_SHADER_VERTEX_ATTRIBUTES];
  memset(attribute_descriptions, 0, sizeof(attribute_descriptions));
  mesh_grass_get_attribute_descriptions(attribute_descriptions);

  vertex_input_info.vertexBindingDescriptionCount = 1;
  vertex_input_info.vertexAttributeDescriptionCount = GRASS_SHADER_VERTEX_ATTRIBUTES;
  vertex_input_info.pVertexBindingDescriptions = &binding_description;
  vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions;

  // Note: Independent blending is for certain devices only, all attachments must blend the same otherwise
  VkPipelineColorBlendAttachmentState color_blend_attachments[GRASS_SHADER_COLOR_ATTACHMENTS];
  memset(color_blend_attachments, 0, sizeof(VkPipelineColorBlendAttachmentState) * GRASS_SHADER_COLOR_ATTACHMENTS);
  for (int pipeline_attachment_num = 0; pipeline_attachment_num < GRASS_SHADER_COLOR_ATTACHMENTS; pipeline_attachment_num++) {
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
  color_blending.attachmentCount = GRASS_SHADER_COLOR_ATTACHMENTS;
  color_blending.pAttachments = color_blend_attachments;
  color_blending.blendConstants[0] = 0.0f;
  color_blending.blendConstants[1] = 0.0f;
  color_blending.blendConstants[2] = 0.0f;
  color_blending.blendConstants[3] = 0.0f;

  shader_init(&grass_shader->grass_render_shader, gpu_api->vulkan_state, "./assets/shaders/spirv/grass.vert.spv", "./assets/shaders/spirv/grass.frag.spv", NULL, vertex_input_info, gpu_api->vulkan_state->gbuffer->render_pass, color_blending, VK_FRONT_FACE_COUNTER_CLOCKWISE, 1, gpu_api->vulkan_state->msaa_samples, true, VK_CULL_MODE_NONE);

  return 1;
}

void grass_shader_delete(struct GrassShader* grass_shader, struct VulkanState* vulkan_renderer) {
  vkDestroyDescriptorPool(vulkan_renderer->device, &grass_shader->grass_compute_shader.descriptor_pool, NULL);
  shader_delete(&grass_shader->grass_compute_shader, vulkan_renderer);
  free(&grass_shader->grass_compute_shader);
}
