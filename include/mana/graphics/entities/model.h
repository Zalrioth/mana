#pragma once
#ifndef MODEL_H
#define MODEL_H

#include "mana/core/memoryallocator.h"
//
#include <mana/core/gpuapi.h>

#include "mana/core/vulkancore.h"
#include "mana/graphics/graphicscommon.h"
#include "mana/graphics/render/vulkanrenderer.h"
#include "mana/graphics/shaders/shader.h"
#include "mana/graphics/utilities/collada/modelanimation.h"
#include "mana/graphics/utilities/collada/modelanimator.h"
#include "mana/graphics/utilities/collada/modelgeometry.h"
#include "mana/graphics/utilities/collada/modelskeleton.h"
#include "mana/graphics/utilities/collada/modelskinning.h"
#include "mana/graphics/utilities/mesh.h"
#include "mana/graphics/utilities/texture.h"
#include "xmlparser.h"

#define MAX_JOINTS 50

struct GPUAPI;
struct Shader;
struct KeyFrame;
struct KeyFrameData;
struct JointTransform;
struct JointTransformData;

// TODO: alignas note needed?
// Seems like alignment over 16 causes this to get snapped
struct ModelUniformBufferObject {
  alignas(16) mat4 model;
  alignas(16) mat4 view;
  alignas(16) mat4 proj;
  alignas(16) vec3 camera_pos;
};

struct ModelAnimationUniformBufferObject {
  alignas(16) mat4 joint_transforms[MAX_JOINTS];
};

struct ModelJoint {
  int index;
  char* name;
  struct ArrayList* children;
  mat4 animation_transform;
  mat4 local_bind_transform;
  mat4 inverse_bind_transform;
};

static inline void joint_init(struct ModelJoint* joint, int index, char* name, mat4 bind_local_transform) {
  joint->animation_transform = MAT4_IDENTITY;
  joint->inverse_bind_transform = MAT4_IDENTITY;
  joint->children = malloc(sizeof(struct ArrayList));
  array_list_init(joint->children);
  joint->index = index;
  joint->name = strdup(name);
  joint->local_bind_transform = bind_local_transform;
}

static inline void joint_calc_inverse_bind_transform(struct ModelJoint* joint, mat4 parent_bind_transform) {
  mat4 bind_transform = MAT4_ZERO;
  bind_transform = mat4_mul(parent_bind_transform, joint->local_bind_transform);
  joint->inverse_bind_transform = mat4_inverse(bind_transform);
  for (int child_num = 0; child_num < array_list_size(joint->children); child_num++) {
    struct ModelJoint* child_joint = (struct ModelJoint*)array_list_get(joint->children, child_num);
    joint_calc_inverse_bind_transform(child_joint, bind_transform);
  }
}

struct ModelSettings {
  char* path;
  int max_weights;
  struct Shader* shader;
  struct Texture* diffuse_texture;
  struct Texture* normal_texture;
  struct Texture* metallic_texture;
  struct Texture* roughness_texture;
  struct Texture* ao_texture;
};

struct Model {
  struct Shader* shader_handle;
  struct Mesh* model_mesh;
  struct Texture* model_diffuse_texture;
  struct Texture* model_normal_texture;
  struct Texture* model_metallic_texture;
  struct Texture* model_roughness_texture;
  struct Texture* model_ao_texture;
  struct SkeletonData* joints;
  struct ModelJoint* root_joint;
  struct Animator* animator;
  struct Animation* animation;
  bool animated;
  char* path;

  mat4 temp_transform;

  vec3 position;
  quat rotation;
  vec3 scale;

  VkBuffer vertex_buffer;
  VkDeviceMemory vertex_buffer_memory;

  VkBuffer index_buffer;
  VkDeviceMemory index_buffer_memory;

  VkBuffer uniform_buffer;
  VkDeviceMemory uniform_buffers_memory;

  VkBuffer uniform_animation_buffer;
  VkDeviceMemory uniform_animation_buffers_memory;

  VkBuffer lighting_uniform_buffer;
  VkDeviceMemory lighting_uniform_buffers_memory;

  VkDescriptorSet descriptor_set;
};

enum {
  MODEL_SUCCESS = 1
};

int model_init(struct Model* model, struct GPUAPI* gpu_api, struct ModelSettings model_settings);
void model_delete(struct Model* model, struct GPUAPI* gpu_api);
struct ModelJoint* model_create_joints(struct JointData* root_joint_data);
struct KeyFrame* model_create_key_frame(struct KeyFrameData* data);
void model_delete_joints(struct ModelJoint* joint);
void model_delete_joints_data(struct JointData* joint_data);
void model_delete_animation(struct Animation* animation);
struct JointTransform model_create_transform(struct JointTransformData* data);
void model_get_joint_transforms(struct ModelJoint* head_joint, mat4 dest[MAX_JOINTS]);
void model_update_uniforms(struct Model* model, struct GPUAPI* gpu_api, vec3 position, vec3 light_pos);
struct Model* model_get_clone(struct Model* model, struct GPUAPI* gpu_api);
void model_clone_delete(struct Model* model, struct GPUAPI* gpu_api);
void model_render(struct Model* model, struct GPUAPI* gpu_api, float delta_time);
void model_recreate(struct Model* model, struct GPUAPI* gpu_api);

#endif  // MODEL_H
