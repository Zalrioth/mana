#include "mana/graphics/shaders/shader.h"

int shader_init(struct Shader* shader, struct VulkanState* vulkan_renderer, char* vertex_shader, char* fragment_shader, char* compute_shader, VkPipelineVertexInputStateCreateInfo vertex_input_info, VkRenderPass render_pass, VkPipelineColorBlendStateCreateInfo color_blending, VkFrontFace direction, bool depth_test, VkSampleCountFlagBits num_samples, bool supersampled, VkCullModeFlags cull_mode) {
  int vertex_length = 0;
  int fragment_length = 0;

  char* vert_shader_code = read_file(vertex_shader, &vertex_length);
  char* frag_shader_code = read_file(fragment_shader, &fragment_length);

  VkShaderModule vert_shader_module = shader_create_shader_module(vulkan_renderer, vert_shader_code, vertex_length);
  VkShaderModule frag_shader_module = shader_create_shader_module(vulkan_renderer, frag_shader_code, fragment_length);

  free(vert_shader_code);
  free(frag_shader_code);

  VkPipelineShaderStageCreateInfo vert_shader_stage_info = {0};
  vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_stage_info.module = vert_shader_module;
  vert_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo frag_shader_stage_info = {0};
  frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_stage_info.module = frag_shader_module;
  frag_shader_stage_info.pName = "main";

  VkPipelineShaderStageCreateInfo shader_stages[] = {vert_shader_stage_info, frag_shader_stage_info};

  VkPipelineInputAssemblyStateCreateInfo input_assembly = {0};
  input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  input_assembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  if (supersampled) {
    viewport.width = (float)vulkan_renderer->swap_chain->swap_chain_extent.width * vulkan_renderer->swap_chain->supersample_scale;
    viewport.height = (float)vulkan_renderer->swap_chain->swap_chain_extent.height * vulkan_renderer->swap_chain->supersample_scale;
  } else {
    viewport.width = (float)vulkan_renderer->swap_chain->swap_chain_extent.width;
    viewport.height = (float)vulkan_renderer->swap_chain->swap_chain_extent.height;
  }

  VkRect2D scissor = {0};
  scissor.offset.x = 0;
  scissor.offset.y = 0;
  if (supersampled) {
    scissor.extent.width = vulkan_renderer->swap_chain->swap_chain_extent.width * vulkan_renderer->swap_chain->supersample_scale;
    scissor.extent.height = vulkan_renderer->swap_chain->swap_chain_extent.height * vulkan_renderer->swap_chain->supersample_scale;
  } else {
    scissor.extent.width = vulkan_renderer->swap_chain->swap_chain_extent.width;
    scissor.extent.height = vulkan_renderer->swap_chain->swap_chain_extent.height;
  }

  VkPipelineViewportStateCreateInfo viewport_state = {0};
  viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewport_state.viewportCount = 1;
  viewport_state.pViewports = &viewport;
  viewport_state.scissorCount = 1;
  viewport_state.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = cull_mode;
  rasterizer.frontFace = direction;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = num_samples;

  VkPipelineDepthStencilStateCreateInfo depth_stencil = {0};
  depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depth_stencil.depthTestEnable = depth_test;
  depth_stencil.depthWriteEnable = depth_test;
  depth_stencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
  depth_stencil.depthBoundsTestEnable = VK_FALSE;
  depth_stencil.stencilTestEnable = VK_FALSE;

  VkPipelineLayoutCreateInfo pipeline_layout_info = {0};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = 1;
  pipeline_layout_info.pSetLayouts = &shader->descriptor_set_layout;

  if (vkCreatePipelineLayout(vulkan_renderer->device, &pipeline_layout_info, NULL, &shader->pipeline_layout) != VK_SUCCESS)
    return 0;

  VkGraphicsPipelineCreateInfo pipeline_info = {0};
  pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipeline_info.stageCount = 2;
  pipeline_info.pStages = shader_stages;
  pipeline_info.pVertexInputState = &vertex_input_info;
  pipeline_info.pInputAssemblyState = &input_assembly;
  pipeline_info.pViewportState = &viewport_state;
  pipeline_info.pRasterizationState = &rasterizer;
  pipeline_info.pMultisampleState = &multisampling;
  pipeline_info.pDepthStencilState = &depth_stencil;
  pipeline_info.pColorBlendState = &color_blending;
  pipeline_info.layout = shader->pipeline_layout;
  pipeline_info.renderPass = render_pass;
  pipeline_info.subpass = 0;
  pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

  if (vkCreateGraphicsPipelines(vulkan_renderer->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &shader->graphics_pipeline) != VK_SUCCESS)
    return VULKAN_RENDERER_CREATE_GRAPHICS_PIPELINE_ERROR;

  vkDestroyShaderModule(vulkan_renderer->device, frag_shader_module, NULL);
  vkDestroyShaderModule(vulkan_renderer->device, vert_shader_module, NULL);

  return VULKAN_RENDERER_SUCCESS;
}

int shader_init_comp(struct Shader* shader, struct VulkanState* vulkan_renderer, char* compute_shader) {
  int compute_length = 0;

  char* compute_shader_code = read_file(compute_shader, &compute_length);

  VkShaderModule comp_shader_module = shader_create_shader_module(vulkan_renderer, compute_shader_code, compute_length);

  free(compute_shader_code);

  VkPipelineLayoutCreateInfo pipeline_layout_create_info = {0};
  pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_create_info.setLayoutCount = 1;
  pipeline_layout_create_info.pSetLayouts = &shader->descriptor_set_layout;

  if (vkCreatePipelineLayout(vulkan_renderer->device, &pipeline_layout_create_info, NULL, &shader->pipeline_layout))
    return -1;

  VkComputePipelineCreateInfo pipeline_info = {0};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipeline_info.stage.module = comp_shader_module;

  pipeline_info.stage.pName = "main";
  pipeline_info.layout = shader->pipeline_layout;

  if (vkCreateComputePipelines(vulkan_renderer->device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &shader->graphics_pipeline) != VK_SUCCESS)
    return VULKAN_RENDERER_CREATE_GRAPHICS_PIPELINE_ERROR;

  vkDestroyShaderModule(vulkan_renderer->device, comp_shader_module, NULL);

  return VULKAN_RENDERER_SUCCESS;
}

void shader_delete(struct Shader* shader, struct VulkanState* vulkan_renderer) {
  vkDestroyPipeline(vulkan_renderer->device, shader->graphics_pipeline, NULL);
  vkDestroyPipelineLayout(vulkan_renderer->device, shader->pipeline_layout, NULL);
  vkDestroyDescriptorSetLayout(vulkan_renderer->device, shader->descriptor_set_layout, NULL);
}

VkShaderModule shader_create_shader_module(struct VulkanState* vulkan_renderer, const char* code, int length) {
  VkShaderModuleCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = length;
  create_info.pCode = (const uint32_t*)(code);

  VkShaderModule shader_module;
  if (vkCreateShaderModule(vulkan_renderer->device, &create_info, NULL, &shader_module) != VK_SUCCESS)
    fprintf(stderr, "Failed to create shader module!\n");

  return shader_module;
}
