#pragma once
#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include <cstorage/cstorage.h>
#include <ubermath/ubermath.h>
#include <vulkan/vulkan.h>

#include "mana/graphics/graphicscommon.h"

struct SamplerSettings {
  uint32_t mip_levels;
  VkFilter filter;
  VkSamplerAddressMode address_mode;
};

static inline void graphics_utils_create_image_view(struct VkDevice_T *device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels, VkImageView *image_view);
static inline int graphics_utils_create_image(struct VkDevice_T *device, struct VkPhysicalDevice_T *physical_device, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory);
static inline int graphics_utils_create_buffer(struct VkDevice_T *device, struct VkPhysicalDevice_T *physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory);
static inline int graphics_utils_transition_image_layout(struct VkDevice_T *device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
static inline uint32_t graphics_utils_find_memory_type(struct VkPhysicalDevice_T *physical_device, uint32_t typeFilter, VkMemoryPropertyFlags properties);
static inline VkCommandBuffer graphics_utils_begin_single_time_commands(struct VkDevice_T *device, struct VkCommandPool_T *command_pool);
static inline void graphics_utils_end_single_time_commands(struct VkDevice_T *device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkCommandBuffer command_buffer);
static inline int graphics_utils_create_sampler(struct VkDevice_T *device, VkSampler *texture_sampler, struct SamplerSettings sampler_settings);
static inline void graphics_utils_copy_buffer_to_image(struct VkDevice_T *device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkBuffer *buffer, VkImage *image, uint32_t width, uint32_t height);
static inline void graphics_utils_generate_mipmaps(struct VkDevice_T *device, VkPhysicalDevice physical_device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkImage image, VkFormat format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels);
static inline VkFormat graphics_utils_find_depth_format(VkPhysicalDevice physical_device);
static inline void graphics_utils_create_color_attachment(VkFormat image_format, struct VkAttachmentDescription *color_attachment);
static inline void graphics_utils_create_depth_attachment(VkPhysicalDevice physical_device, struct VkAttachmentDescription *depth_attachment);
static inline void graphics_utisl_copy_buffer(struct VulkanState *vulkan_state, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);
static inline void graphics_utisl_copy_buffer_offset(struct VulkanState *vulkan_state, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, unsigned int offset);
static inline void graphics_utils_setup_vertex_buffer(struct VulkanState *vulkan_state, struct Vector *vertices, VkBuffer *vertex_buffer, VkDeviceMemory *vertex_buffer_memory);
static inline void graphics_utils_setup_vertex_buffer_pool(struct VulkanState *vulkan_state, struct Vector *vertices, int total_pool_elements, VkBuffer *vertex_buffer, VkDeviceMemory *vertex_buffer_memory);
static inline void graphics_utils_update_vertex_buffer(struct VulkanState *vulkan_state, struct Vector *vertices, VkBuffer *vertex_buffer, VkDeviceMemory *vertex_buffer_memory);
static inline void graphics_utils_setup_index_buffer(struct VulkanState *vulkan_state, struct Vector *indices, VkBuffer *index_buffer, VkDeviceMemory *index_buffer_memory);
static inline void graphics_utils_setup_index_buffer_pool(struct VulkanState *vulkan_state, struct Vector *indices, int total_pool_elements, VkBuffer *index_buffer, VkDeviceMemory *index_buffer_memory);
static inline void graphics_utils_update_index_buffer(struct VulkanState *vulkan_state, struct Vector *indices, VkBuffer *index_buffer, VkDeviceMemory *index_buffer_memory);
static inline void graphics_utils_setup_uniform_buffer(struct VulkanState *vulkan_state, size_t memory_size, VkBuffer *uniform_buffer, VkDeviceMemory *uniform_buffer_memory);
static inline int graphics_utils_setup_descriptor(struct VulkanState *vulkan_state, struct VkDescriptorSetLayout_T *descriptor_set_layout, struct VkDescriptorPool_T *descriptor_pool, VkDescriptorSet *descriptor_set);
static inline VkDescriptorBufferInfo graphics_utils_setup_descriptor_buffer_info(size_t memory_size, VkBuffer *uniform_buffer);
static inline void graphics_utils_setup_descriptor_buffer(struct VulkanState *vulkan_state, VkWriteDescriptorSet *dcs, size_t index, VkDescriptorSet *descriptor_set, VkDescriptorBufferInfo *buffer_info);
static inline VkDescriptorImageInfo graphics_utils_setup_descriptor_image_info(VkImageView *texture_image_view, VkSampler *texture_sampler);
static inline void graphics_utils_setup_descriptor_image(struct VulkanState *vulkan_state, VkWriteDescriptorSet *dcs, size_t index, VkDescriptorSet *descriptor_set, VkDescriptorImageInfo *image_info);

static inline void graphics_utils_create_image_view(struct VkDevice_T *device, VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels, VkImageView *image_view) {
  VkImageViewCreateInfo view_info = {0};
  view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  view_info.image = image;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  view_info.format = format;
  view_info.subresourceRange.aspectMask = aspect_flags;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = mip_levels;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &view_info, NULL, image_view) != VK_SUCCESS)
    fprintf(stderr, "failed to create texture image view!");
}

static inline int graphics_utils_create_image(struct VkDevice_T *device, struct VkPhysicalDevice_T *physical_device, uint32_t width, uint32_t height, uint32_t mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *image_memory) {
  VkImageCreateInfo image_info = {0};
  image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = mip_levels;
  image_info.arrayLayers = 1;
  image_info.format = format;
  image_info.tiling = tiling;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.usage = usage;
  image_info.samples = num_samples;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &image_info, NULL, image) != VK_SUCCESS)
    return -1;
  // printf("failed to create image!");

  VkMemoryRequirements mem_mequirements;
  vkGetImageMemoryRequirements(device, *image, &mem_mequirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_mequirements.size;
  alloc_info.memoryTypeIndex = graphics_utils_find_memory_type(physical_device, mem_mequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &alloc_info, NULL, image_memory) !=
      VK_SUCCESS)
    return -1;
  // printf("failed to allocate image memory!");

  vkBindImageMemory(device, *image, *image_memory, 0);

  return 0;
}

static inline int graphics_utils_create_buffer(struct VkDevice_T *device, struct VkPhysicalDevice_T *physical_device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *buffer_memory) {
  VkBufferCreateInfo buffer_info = {0};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.size = size;
  buffer_info.usage = usage;
  buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &buffer_info, NULL, buffer) != VK_SUCCESS) {
    fprintf(stderr, "failed to create buffer!\n");
    return -1;
  }

  VkMemoryRequirements mem_requirements;
  vkGetBufferMemoryRequirements(device, *buffer, &mem_requirements);

  VkMemoryAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_requirements.size;
  alloc_info.memoryTypeIndex = graphics_utils_find_memory_type(physical_device, mem_requirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &alloc_info, NULL, buffer_memory) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate buffer memory!\n");
    return -1;
  }

  vkBindBufferMemory(device, *buffer, *buffer_memory, 0);

  return 0;
}

static inline int graphics_utils_transition_image_layout(struct VkDevice_T *device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels) {
  VkCommandBuffer commandBuffer = graphics_utils_begin_single_time_commands(device, command_pool);

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = old_layout;
  barrier.newLayout = new_layout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = mip_levels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags source_stage = {0};
  VkPipelineStageFlags destination_stage = {0};

  if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    fprintf(stderr, "unsupported layout transition!");
    return -1;
  }

  vkCmdPipelineBarrier(commandBuffer, source_stage, destination_stage, 0, 0, NULL, 0, NULL, 1, &barrier);

  graphics_utils_end_single_time_commands(device, graphics_queue, command_pool, commandBuffer);

  return 0;
}

static inline uint32_t graphics_utils_find_memory_type(struct VkPhysicalDevice_T *physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_properties;
  vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

  for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;

  printf("failed to find suitable memory type!\n");

  return -1;
}

static inline VkCommandBuffer graphics_utils_begin_single_time_commands(struct VkDevice_T *device, struct VkCommandPool_T *command_pool) {
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = command_pool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer command_buffer;
  vkAllocateCommandBuffers(device, &allocInfo, &command_buffer);

  VkCommandBufferBeginInfo begin_info = {0};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(command_buffer, &begin_info);

  return command_buffer;
}

static inline void graphics_utils_end_single_time_commands(struct VkDevice_T *device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkCommandBuffer command_buffer) {
  vkEndCommandBuffer(command_buffer);

  VkSubmitInfo submit_info = {0};
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer;

  vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphics_queue);

  vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}

static inline int graphics_utils_create_sampler(struct VkDevice_T *device, VkSampler *texture_sampler, struct SamplerSettings sampler_settings) {
  VkSamplerCreateInfo sampler_info = {0};
  sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_info.magFilter = sampler_settings.filter;
  sampler_info.minFilter = sampler_settings.filter;
  // TODO: Let this be changeable
  sampler_info.addressModeU = sampler_settings.address_mode;
  sampler_info.addressModeV = sampler_settings.address_mode;
  sampler_info.addressModeW = sampler_settings.address_mode;
  sampler_info.anisotropyEnable = VK_TRUE;
  sampler_info.maxAnisotropy = 16;
  sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  sampler_info.unnormalizedCoordinates = VK_FALSE;
  sampler_info.compareEnable = VK_FALSE;
  sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
  sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler_info.minLod = 0;
  sampler_info.maxLod = (float)(sampler_settings.mip_levels);
  sampler_info.mipLodBias = 0;

  if (vkCreateSampler(device, &sampler_info, NULL, texture_sampler) != VK_SUCCESS) {
    printf("failed to create texture sampler!\n");
    return -1;
  }

  return 0;
}

static inline void graphics_utils_copy_buffer_to_image(struct VkDevice_T *device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkBuffer *buffer, VkImage *image, uint32_t width, uint32_t height) {
  VkCommandBuffer command_buffer = graphics_utils_begin_single_time_commands(device, command_pool);

  VkBufferImageCopy region = {0};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = (VkOffset3D){0, 0, 0};
  region.imageExtent = (VkExtent3D){width, height, 1};

  vkCmdCopyBufferToImage(command_buffer, *buffer, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  graphics_utils_end_single_time_commands(device, graphics_queue, command_pool, command_buffer);
}

static inline void graphics_utils_generate_mipmaps(struct VkDevice_T *device, VkPhysicalDevice physical_device, struct VkQueue_T *graphics_queue, struct VkCommandPool_T *command_pool, VkImage image, VkFormat format, int32_t tex_width, int32_t tex_height, uint32_t mip_levels) {
  VkFormatProperties format_properties;
  vkGetPhysicalDeviceFormatProperties(physical_device, format, &format_properties);

  if (!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
    return;
  //throw std::runtime_error("texture image format does not support linear blitting!");

  VkCommandBuffer command_buffer = graphics_utils_begin_single_time_commands(device, command_pool);

  VkImageMemoryBarrier barrier = {0};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  int32_t mip_width = tex_width;
  int32_t mip_height = tex_height;

  for (uint32_t i = 1; i < mip_levels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    VkImageBlit blit = {0};
    blit.srcOffsets[0] = (VkOffset3D){0, 0, 0};
    blit.srcOffsets[1] = (VkOffset3D){mip_width, mip_height, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = (VkOffset3D){0, 0, 0};
    blit.dstOffsets[1] = (VkOffset3D){mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(command_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

    if (mip_width > 1) mip_width /= 2;
    if (mip_height > 1) mip_height /= 2;
  }

  barrier.subresourceRange.baseMipLevel = mip_levels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
  graphics_utils_end_single_time_commands(device, graphics_queue, command_pool, command_buffer);
}

#define TOTAL_CANDIDIATES 3
static inline VkFormat graphics_utils_find_depth_format(VkPhysicalDevice physical_device) {
  VkFormat candidate[TOTAL_CANDIDIATES] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
  VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

  for (int loop_num = 0; loop_num < TOTAL_CANDIDIATES; loop_num++) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physical_device, candidate[loop_num], &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features) {
      return candidate[loop_num];
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return candidate[loop_num];
    }
  }

  return -1;
}

static inline void graphics_utils_create_color_attachment(VkFormat image_format, struct VkAttachmentDescription *color_attachment) {
  color_attachment->format = image_format;
  color_attachment->samples = VK_SAMPLE_COUNT_1_BIT;
  color_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color_attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color_attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  color_attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  color_attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  color_attachment->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

static inline void graphics_utils_create_depth_attachment(VkPhysicalDevice physical_device, struct VkAttachmentDescription *depth_attachment) {
  depth_attachment->format = graphics_utils_find_depth_format(physical_device);
  depth_attachment->samples = VK_SAMPLE_COUNT_1_BIT;
  depth_attachment->loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth_attachment->storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depth_attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth_attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depth_attachment->finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
}

static inline void graphics_utisl_copy_buffer(struct VulkanState *vulkan_state, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size) {
  VkCommandBuffer command_buffer = graphics_utils_begin_single_time_commands(vulkan_state->device, vulkan_state->command_pool);

  VkBufferCopy copy_region = {0};
  copy_region.size = size;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  graphics_utils_end_single_time_commands(vulkan_state->device, vulkan_state->graphics_queue, vulkan_state->command_pool, command_buffer);
}

static inline void graphics_utisl_copy_buffer_offset(struct VulkanState *vulkan_state, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size, unsigned int offset) {
  VkCommandBuffer command_buffer = graphics_utils_begin_single_time_commands(vulkan_state->device, vulkan_state->command_pool);

  VkBufferCopy copy_region = {0};
  copy_region.size = size;
  copy_region.srcOffset = offset;
  vkCmdCopyBuffer(command_buffer, src_buffer, dst_buffer, 1, &copy_region);

  graphics_utils_end_single_time_commands(vulkan_state->device, vulkan_state->graphics_queue, vulkan_state->command_pool, command_buffer);
}

static inline void graphics_utils_setup_vertex_buffer(struct VulkanState *vulkan_state, struct Vector *vertices, VkBuffer *vertex_buffer, VkDeviceMemory *vertex_buffer_memory) {
  VkDeviceSize vertex_buffer_size = vertices->memory_size * vertices->size;
  VkBuffer vertex_staging_buffer = {0};
  VkDeviceMemory vertex_staging_buffer_memory = {0};
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffer, &vertex_staging_buffer_memory);
  void *vertex_data;
  vkMapMemory(vulkan_state->device, vertex_staging_buffer_memory, 0, vertex_buffer_size, 0, &vertex_data);
  memcpy(vertex_data, vertices->items, vertex_buffer_size);
  vkUnmapMemory(vulkan_state->device, vertex_staging_buffer_memory);
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);
  graphics_utisl_copy_buffer(vulkan_state, vertex_staging_buffer, *vertex_buffer, vertex_buffer_size);
  vkDestroyBuffer(vulkan_state->device, vertex_staging_buffer, NULL);
  vkFreeMemory(vulkan_state->device, vertex_staging_buffer_memory, NULL);
}

static inline void graphics_utils_setup_vertex_buffer_pool(struct VulkanState *vulkan_state, struct Vector *vertices, int total_pool_elements, VkBuffer *vertex_buffer, VkDeviceMemory *vertex_buffer_memory) {
  VkDeviceSize vertex_buffer_size = vertices->memory_size * total_pool_elements;
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_buffer_memory);
}

static inline void graphics_utils_update_vertex_buffer(struct VulkanState *vulkan_state, struct Vector *vertices, VkBuffer *vertex_buffer, VkDeviceMemory *vertex_buffer_memory) {
  VkDeviceSize vertex_buffer_size = vertices->memory_size * vertices->size;
  VkBuffer vertex_staging_buffer = {0};
  VkDeviceMemory vertex_staging_buffer_memory = {0};
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, vertex_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertex_staging_buffer, &vertex_staging_buffer_memory);
  void *vertex_data;
  vkMapMemory(vulkan_state->device, vertex_staging_buffer_memory, 0, vertex_buffer_size, 0, &vertex_data);
  memcpy(vertex_data, vertices->items, vertex_buffer_size);
  vkUnmapMemory(vulkan_state->device, vertex_staging_buffer_memory);
  graphics_utisl_copy_buffer(vulkan_state, vertex_staging_buffer, *vertex_buffer, vertex_buffer_size);
  vkDestroyBuffer(vulkan_state->device, vertex_staging_buffer, NULL);
  vkFreeMemory(vulkan_state->device, vertex_staging_buffer_memory, NULL);
}

static inline void graphics_utils_setup_index_buffer(struct VulkanState *vulkan_state, struct Vector *indices, VkBuffer *index_buffer, VkDeviceMemory *index_buffer_memory) {
  VkDeviceSize index_buffer_size = indices->memory_size * indices->size;
  VkBuffer index_staging_buffer = {0};
  VkDeviceMemory index_staging_buffer_memory = {0};
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_staging_buffer, &index_staging_buffer_memory);
  void *index_data;
  vkMapMemory(vulkan_state->device, index_staging_buffer_memory, 0, index_buffer_size, 0, &index_data);
  memcpy(index_data, indices->items, index_buffer_size);
  vkUnmapMemory(vulkan_state->device, index_staging_buffer_memory);
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);
  graphics_utisl_copy_buffer(vulkan_state, index_staging_buffer, *index_buffer, index_buffer_size);
  vkDestroyBuffer(vulkan_state->device, index_staging_buffer, NULL);
  vkFreeMemory(vulkan_state->device, index_staging_buffer_memory, NULL);
}

static inline void graphics_utils_setup_index_buffer_pool(struct VulkanState *vulkan_state, struct Vector *indices, int total_pool_elements, VkBuffer *index_buffer, VkDeviceMemory *index_buffer_memory) {
  VkDeviceSize index_buffer_size = indices->memory_size * total_pool_elements;
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_buffer_memory);
}

static inline void graphics_utils_update_index_buffer(struct VulkanState *vulkan_state, struct Vector *indices, VkBuffer *index_buffer, VkDeviceMemory *index_buffer_memory) {
  VkDeviceSize index_buffer_size = indices->memory_size * indices->size;
  VkBuffer index_staging_buffer = {0};
  VkDeviceMemory index_staging_buffer_memory = {0};
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, index_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &index_staging_buffer, &index_staging_buffer_memory);
  void *index_data;
  vkMapMemory(vulkan_state->device, index_staging_buffer_memory, 0, index_buffer_size, 0, &index_data);
  memcpy(index_data, indices->items, index_buffer_size);
  vkUnmapMemory(vulkan_state->device, index_staging_buffer_memory);
  graphics_utisl_copy_buffer(vulkan_state, index_staging_buffer, *index_buffer, index_buffer_size);
  vkDestroyBuffer(vulkan_state->device, index_staging_buffer, NULL);
  vkFreeMemory(vulkan_state->device, index_staging_buffer_memory, NULL);
}

static inline void graphics_utils_setup_uniform_buffer(struct VulkanState *vulkan_state, size_t memory_size, VkBuffer *uniform_buffer, VkDeviceMemory *uniform_buffer_memory) {
  VkDeviceSize uniform_buffer_size = memory_size;
  graphics_utils_create_buffer(vulkan_state->device, vulkan_state->physical_device, uniform_buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniform_buffer, uniform_buffer_memory);
}

static inline int graphics_utils_setup_descriptor(struct VulkanState *vulkan_state, struct VkDescriptorSetLayout_T *descriptor_set_layout, struct VkDescriptorPool_T *descriptor_pool, VkDescriptorSet *descriptor_set) {
  VkDescriptorSetLayout layout = {0};
  layout = descriptor_set_layout;

  VkDescriptorSetAllocateInfo alloc_info = {0};
  alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  alloc_info.descriptorPool = descriptor_pool;
  alloc_info.descriptorSetCount = 1;
  alloc_info.pSetLayouts = &layout;

  if (vkAllocateDescriptorSets(vulkan_state->device, &alloc_info, descriptor_set) != VK_SUCCESS) {
    fprintf(stderr, "failed to allocate descriptor sets!\n");
    return 1;
  }

  return 0;
}

static inline VkDescriptorBufferInfo graphics_utils_setup_descriptor_buffer_info(size_t memory_size, VkBuffer *uniform_buffer) {
  VkDescriptorBufferInfo buffer_info = {0};
  buffer_info.buffer = *uniform_buffer;
  buffer_info.offset = 0;
  buffer_info.range = memory_size;

  return buffer_info;
}

static inline void graphics_utils_setup_descriptor_buffer(struct VulkanState *vulkan_state, VkWriteDescriptorSet *dcs, size_t index, VkDescriptorSet *descriptor_set, VkDescriptorBufferInfo *buffer_info) {
  dcs[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  dcs[index].dstSet = *descriptor_set;
  dcs[index].dstBinding = index;
  dcs[index].dstArrayElement = 0;
  dcs[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  dcs[index].descriptorCount = 1;
  dcs[index].pBufferInfo = buffer_info;
}

static inline VkDescriptorImageInfo graphics_utils_setup_descriptor_image_info(VkImageView *texture_image_view, VkSampler *texture_sampler) {
  VkDescriptorImageInfo image_info = {0};
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  image_info.imageView = *texture_image_view;
  image_info.sampler = *texture_sampler;

  return image_info;
}

static inline void graphics_utils_setup_descriptor_image(struct VulkanState *vulkan_state, VkWriteDescriptorSet *dcs, size_t index, VkDescriptorSet *descriptor_set, VkDescriptorImageInfo *image_info) {
  dcs[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  dcs[index].dstSet = *descriptor_set;
  dcs[index].dstBinding = index;
  dcs[index].dstArrayElement = 0;
  dcs[index].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  dcs[index].descriptorCount = 1;
  dcs[index].pImageInfo = image_info;
}

#endif  // GRAPHICS_UTILS_H
