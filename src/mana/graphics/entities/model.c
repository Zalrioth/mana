#include "mana/graphics/entities/model.h"

int model_init(struct Model* model, struct GPUAPI* gpu_api, struct ModelSettings model_settings) {
  struct XmlNode* collada_node = xml_parser_load_xml_file(model_settings.path);
  struct XmlNode* library_controllers_node = xml_node_get_child(collada_node, "library_controllers");  // If texture is null, use custom 8x8 ubo color palette
  model->animated = !(library_controllers_node == NULL || library_controllers_node->child_nodes == NULL || library_controllers_node->child_nodes->num_buckets == 0);

  struct SkinningData* skinning_data = NULL;
  if (model->animated) {
    skinning_data = skin_loader_extract_skin_data(library_controllers_node, model_settings.max_weights);
    model->joints = skeleton_loader_extract_bone_data(xml_node_get_child(collada_node, "library_visual_scenes"), skinning_data->joint_order);
    model->model_mesh = geometry_loader_extract_model_data(xml_node_get_child(collada_node, "library_geometries"), skinning_data->vertices_skin_data, model->animated);

    model->root_joint = model_create_joints(model->joints->head_joint);
    model->animator = malloc(sizeof(struct Animator));
    animator_init(model->animator, model);
    joint_calc_inverse_bind_transform(model->root_joint, MAT4_IDENTITY);

    struct XmlNode* anim_node = xml_node_get_child(collada_node, "library_animations");
    struct XmlNode* joints_node = xml_node_get_child(collada_node, "library_visual_scenes");
    struct AnimationData* animation_data = animation_extract_animation(anim_node, joints_node);

    struct ArrayList* frames = malloc(sizeof(struct ArrayList));
    array_list_init(frames);

    for (int frame_num = 0; frame_num < array_list_size(animation_data->key_frames); frame_num++)
      array_list_add(frames, model_create_key_frame(array_list_get(animation_data->key_frames, frame_num)));

    model->animation = malloc(sizeof(struct Animation));
    animation_init(model->animation, animation_data->length_seconds, frames);

    for (int frame_num = 0; frame_num < array_list_size(animation_data->key_frames); frame_num++) {
      struct KeyFrameData* key_frame_data = (struct KeyFrameData*)array_list_get(animation_data->key_frames, frame_num);
      for (int transform_num = 0; transform_num < array_list_size(key_frame_data->joint_transforms); transform_num++) {
        struct JointTransformData* joint_transform_data = (struct JointTransformData*)array_list_get(key_frame_data->joint_transforms, transform_num);
        free(joint_transform_data->joint_name_id);
        free(joint_transform_data);
      }
      array_list_delete(key_frame_data->joint_transforms);
      free(key_frame_data->joint_transforms);
      free(key_frame_data);
    }
    array_list_delete(animation_data->key_frames);
    free(animation_data->key_frames);
    free(animation_data);

    animator_do_animation(model->animator, model->animation);

    for (int joint_num = 0; joint_num < vector_size(skinning_data->joint_order); joint_num++)
      free(*((char**)vector_get(skinning_data->joint_order, joint_num)));

    vector_delete(skinning_data->joint_order);
    free(skinning_data->joint_order);

    for (int vertice_num = 0; vertice_num < vector_size(skinning_data->vertices_skin_data); vertice_num++) {
      struct VertexSkinData* vertex_skin_data = (struct VertexSkinData*)vector_get(skinning_data->vertices_skin_data, vertice_num);
      vector_delete(vertex_skin_data->joint_ids);
      free(vertex_skin_data->joint_ids);
      vector_delete(vertex_skin_data->weights);
      free(vertex_skin_data->weights);
    }

    vector_delete(skinning_data->vertices_skin_data);
    free(skinning_data->vertices_skin_data);

    free(skinning_data);

  } else
    model->model_mesh = geometry_loader_extract_model_data(xml_node_get_child(collada_node, "library_geometries"), NULL, model->animated);

  model->path = strdup(model_settings.path);
  model->model_diffuse_texture = model_settings.diffuse_texture;
  model->model_normal_texture = model_settings.normal_texture;
  model->model_metallic_texture = model_settings.metallic_texture;
  model->model_roughness_texture = model_settings.roughness_texture;
  model->model_ao_texture = model_settings.ao_texture;
  model->shader_handle = model_settings.shader;

  xml_parser_delete(collada_node);

  return MODEL_SUCCESS;
}

void model_delete(struct Model* model, struct GPUAPI* gpu_api) {
  if (model->animated) {
    model_delete_joints_data(model->joints->head_joint);
    free(model->joints);
    model_delete_joints(model->root_joint);
    model_delete_animation(model->animation);
    free(model->animation);
    free(model->animator);
  }

  free(model->path);
  mesh_delete(model->model_mesh);
  free(model->model_mesh);
}

struct ModelJoint* model_create_joints(struct JointData* root_joint_data) {
  struct ModelJoint* joint = malloc(sizeof(struct ModelJoint));
  joint_init(joint, root_joint_data->index, root_joint_data->name_id, root_joint_data->bind_local_transform);

  for (int joint_child_num = 0; joint_child_num < array_list_size(root_joint_data->children); joint_child_num++) {
    struct JointData* child_joint = (struct JointData*)array_list_get(root_joint_data->children, joint_child_num);
    array_list_add(joint->children, model_create_joints(child_joint));
  }
  return joint;
}

void model_delete_joints(struct ModelJoint* joint) {
  if (joint->children != NULL && !array_list_empty(joint->children)) {
    for (int joint_num = 0; joint_num < array_list_size(joint->children); joint_num++)
      model_delete_joints((struct ModelJoint*)array_list_get(joint->children, joint_num));
  }
  free(joint->name);
  array_list_delete(joint->children);
  free(joint->children);
  free(joint);
}

void model_delete_joints_data(struct JointData* joint_data) {
  if (joint_data->children != NULL && !array_list_empty(joint_data->children)) {
    for (int joint_num = 0; joint_num < array_list_size(joint_data->children); joint_num++)
      model_delete_joints_data((struct JointData*)array_list_get(joint_data->children, joint_num));
  }
  free(joint_data->name_id);
  array_list_delete(joint_data->children);
  free(joint_data->children);
  free(joint_data);
}

void model_delete_animation(struct Animation* animation) {
  // Think memory bug is in here
  for (int frame_num = 0; frame_num < array_list_size(animation->key_frames); frame_num++) {
    struct KeyFrame* key_frame = (struct KeyFrame*)array_list_get(animation->key_frames, frame_num);
    map_delete(key_frame->pose);
    free(key_frame->pose);
    free(key_frame);
  }

  array_list_delete(animation->key_frames);
  free(animation->key_frames);
}

struct KeyFrame* model_create_key_frame(struct KeyFrameData* data) {
  struct Map* map = malloc(sizeof(struct Map));
  map_init(map, sizeof(struct JointTransform));
  for (int joint_num = 0; joint_num < array_list_size(data->joint_transforms); joint_num++) {
    struct JointTransformData* joint_data = (struct JointTransformData*)array_list_get(data->joint_transforms, joint_num);
    struct JointTransform joint_transform = model_create_transform(joint_data);
    map_set(map, joint_data->joint_name_id, &joint_transform);
  }
  struct KeyFrame* key_frame = malloc(sizeof(struct KeyFrame));
  key_frame_init(key_frame, data->time, map);
  return key_frame;
}

struct JointTransform model_create_transform(struct JointTransformData* data) {
  mat4 mat = data->joint_local_transform;
  vec3 translation = (vec3){.data[0] = mat.vecs[3].data[0], .data[1] = mat.vecs[3].data[1], .data[2] = mat.vecs[3].data[2]};
  quat rot = mat4_to_quaternion(mat);
  return (struct JointTransform){.position = translation, .rotation = rot};
}

void model_get_joint_transforms(struct ModelJoint* head_joint, mat4 dest[MAX_JOINTS]) {
  dest[head_joint->index] = head_joint->animation_transform;
  for (int child_joint_num = 0; child_joint_num < array_list_size(head_joint->children); child_joint_num++)
    model_get_joint_transforms((struct ModelJoint*)array_list_get(head_joint->children, child_joint_num), dest);
}

void model_update_uniforms(struct Model* model, struct GPUAPI* gpu_api, vec3 position, vec3 light_pos) {
  struct LightingUniformBufferObject light_ubo = {{0}};
  light_ubo.direction = light_pos;
  vec3 light_ambient = (vec3){.data[0] = 1.0f, .data[1] = 1.0f, .data[2] = 1.0f};
  light_ubo.ambient_color = light_ambient;
  vec3 light_diffuse = (vec3){.data[0] = 1.0f, .data[1] = 1.0f, .data[2] = 1.0f};
  light_ubo.diffuse_colour = light_diffuse;
  vec3 light_specular = (vec3){.data[0] = 1.0f, .data[1] = 1.0f, .data[2] = 1.0f};
  light_ubo.specular_colour = light_specular;

  void* data;

  struct ModelUniformBufferObject ubom = {{{0}}};

  ubom.proj = gpu_api->vulkan_state->gbuffer->projection_matrix;
  ubom.proj.vecs[1].data[1] *= -1;

  ubom.view = gpu_api->vulkan_state->gbuffer->view_matrix;

  ubom.model = mat4_translate(MAT4_IDENTITY, model->position);
  ubom.model = mat4_mul(ubom.model, quaternion_to_mat4(quaternion_normalise(model->rotation)));
  ubom.model = mat4_scale(ubom.model, model->scale);

  ubom.camera_pos = position;

  vkMapMemory(gpu_api->vulkan_state->device, model->uniform_buffers_memory, 0, sizeof(struct ModelUniformBufferObject), 0, &data);
  memcpy(data, &ubom, sizeof(struct ModelUniformBufferObject));
  vkUnmapMemory(gpu_api->vulkan_state->device, model->uniform_buffers_memory);

  if (model->animated) {
    struct ModelAnimationUniformBufferObject uboa = {{{0}}};
    model_get_joint_transforms(model->root_joint, uboa.joint_transforms);

    void* animation_data;
    vkMapMemory(gpu_api->vulkan_state->device, model->uniform_animation_buffers_memory, 0, sizeof(struct ModelAnimationUniformBufferObject), 0, &animation_data);
    memcpy(animation_data, &uboa, sizeof(struct ModelAnimationUniformBufferObject));
    vkUnmapMemory(gpu_api->vulkan_state->device, model->uniform_animation_buffers_memory);
  }

  void* lighting_data;
  vkMapMemory(gpu_api->vulkan_state->device, model->lighting_uniform_buffers_memory, 0, sizeof(struct LightingUniformBufferObject), 0, &lighting_data);
  memcpy(lighting_data, &light_ubo, sizeof(struct LightingUniformBufferObject));
  vkUnmapMemory(gpu_api->vulkan_state->device, model->lighting_uniform_buffers_memory);
}

struct ModelJoint* model_create_joints_clone(struct ModelJoint* root_joint) {
  struct ModelJoint* joint = malloc(sizeof(struct ModelJoint));
  *joint = *root_joint;
  joint->name = strdup(root_joint->name);
  joint->children = malloc(sizeof(struct ArrayList));
  array_list_init(joint->children);

  for (int joint_child_num = 0; joint_child_num < array_list_size(root_joint->children); joint_child_num++)
    array_list_add(joint->children, model_create_joints_clone((struct ModelJoint*)array_list_get(root_joint->children, joint_child_num)));

  return joint;
}

void model_joints_clone_delete(struct ModelJoint* root_joint) {
  if (root_joint->children != NULL && !array_list_empty(root_joint->children)) {
    for (int joint_num = 0; joint_num < array_list_size(root_joint->children); joint_num++)
      model_delete_joints((struct ModelJoint*)array_list_get(root_joint->children, joint_num));
  }
  free(root_joint->name);
  array_list_delete(root_joint->children);
  free(root_joint->children);
  free(root_joint);
}

struct Model* model_get_clone(struct Model* model, struct GPUAPI* gpu_api) {
  struct Model* new_model = malloc(sizeof(struct Model));
  *new_model = *model;

  new_model->position = VEC3_ZERO;
  new_model->rotation = QUAT_DEFAULT;
  new_model->scale = VEC3_ONE;

  new_model->model_mesh = malloc(sizeof(struct Mesh));
  new_model->model_mesh->indices = malloc(sizeof(struct Vector));
  new_model->model_mesh->vertices = malloc(sizeof(struct Vector));

  *new_model->model_mesh->indices = *model->model_mesh->indices;
  *new_model->model_mesh->vertices = *model->model_mesh->vertices;
  //(new_model->animated) ? mesh_model_init(new_model->model_mesh) : mesh_model_static_init(new_model->model_mesh);
  new_model->model_mesh->indices->items = malloc(model->model_mesh->indices->capacity * model->model_mesh->indices->memory_size);
  memcpy(new_model->model_mesh->indices->items, model->model_mesh->indices->items, model->model_mesh->indices->capacity * model->model_mesh->indices->memory_size);

  new_model->model_mesh->vertices->items = malloc(model->model_mesh->vertices->capacity * model->model_mesh->vertices->memory_size);
  memcpy(new_model->model_mesh->vertices->items, model->model_mesh->vertices->items, model->model_mesh->vertices->capacity * model->model_mesh->vertices->memory_size);

  if (new_model->animated) {
    new_model->root_joint = model_create_joints_clone(model->root_joint);
    new_model->animator = malloc(sizeof(struct Animator));
    animator_init(new_model->animator, new_model);
    animator_do_animation(new_model->animator, new_model->animation);
  }

  graphics_utils_setup_vertex_buffer(gpu_api->vulkan_state, new_model->model_mesh->vertices, &new_model->vertex_buffer, &new_model->vertex_buffer_memory);
  graphics_utils_setup_index_buffer(gpu_api->vulkan_state, new_model->model_mesh->indices, &new_model->index_buffer, &new_model->index_buffer_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct ModelUniformBufferObject), &new_model->uniform_buffer, &new_model->uniform_buffers_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct LightingUniformBufferObject), &new_model->lighting_uniform_buffer, &new_model->lighting_uniform_buffers_memory);
  if (model->animated)
    graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct ModelAnimationUniformBufferObject), &new_model->uniform_animation_buffer, &new_model->uniform_animation_buffers_memory);
  graphics_utils_setup_descriptor(gpu_api->vulkan_state, new_model->shader_handle->descriptor_set_layout, new_model->shader_handle->descriptor_pool, &new_model->descriptor_set);

  if (model->animated) {
    VkWriteDescriptorSet dcs[8] = {0};
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &new_model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ModelUniformBufferObject), &new_model->uniform_buffer)});
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 1, &new_model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct LightingUniformBufferObject), &new_model->lighting_uniform_buffer)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 2, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_diffuse_texture->texture_image_view, &new_model->model_diffuse_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 3, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_normal_texture->texture_image_view, &new_model->model_normal_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 4, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_metallic_texture->texture_image_view, &new_model->model_metallic_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 5, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_roughness_texture->texture_image_view, &new_model->model_roughness_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 6, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_ao_texture->texture_image_view, &new_model->model_ao_texture->texture_sampler)});
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 7, &new_model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ModelAnimationUniformBufferObject), &new_model->uniform_animation_buffer)});
    vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 8, dcs, 0, NULL);
  } else {
    VkWriteDescriptorSet dcs[7] = {0};
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &new_model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ModelUniformBufferObject), &new_model->uniform_buffer)});
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 1, &new_model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct LightingUniformBufferObject), &new_model->lighting_uniform_buffer)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 2, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_diffuse_texture->texture_image_view, &new_model->model_diffuse_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 3, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_normal_texture->texture_image_view, &new_model->model_normal_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 4, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_metallic_texture->texture_image_view, &new_model->model_metallic_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 5, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_roughness_texture->texture_image_view, &new_model->model_roughness_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 6, &new_model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&new_model->model_ao_texture->texture_image_view, &new_model->model_ao_texture->texture_sampler)});
    vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 7, dcs, 0, NULL);
  }

  return new_model;
}

static inline void model_vulkan_cleanup(struct Model* model, struct GPUAPI* gpu_api) {
  vkDestroyBuffer(gpu_api->vulkan_state->device, model->index_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, model->index_buffer_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, model->vertex_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, model->vertex_buffer_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, model->uniform_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, model->uniform_buffers_memory, NULL);

  vkDestroyBuffer(gpu_api->vulkan_state->device, model->lighting_uniform_buffer, NULL);
  vkFreeMemory(gpu_api->vulkan_state->device, model->lighting_uniform_buffers_memory, NULL);

  if (model->animated) {
    vkDestroyBuffer(gpu_api->vulkan_state->device, model->uniform_animation_buffer, NULL);
    vkFreeMemory(gpu_api->vulkan_state->device, model->uniform_animation_buffers_memory, NULL);
  }
}

void model_clone_delete(struct Model* model, struct GPUAPI* gpu_api) {
  model_vulkan_cleanup(model, gpu_api);

  // TODO: Delete uniform colors if needed

  if (model->animated) {
    free(model->animator);
    model_joints_clone_delete(model->root_joint);
  }

  mesh_delete(model->model_mesh);
  free(model->model_mesh);
}

void model_render(struct Model* model, struct GPUAPI* gpu_api, float delta_time) {
  // TODO: Should animation updating be seperated from rendering?
  if (model->animated)
    animator_update(model->animator, delta_time);

  vkCmdBindPipeline(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, model->shader_handle->graphics_pipeline);
  VkBuffer vertex_buffers[] = {model->vertex_buffer};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, 0, 1, vertex_buffers, offsets);
  vkCmdBindIndexBuffer(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, model->index_buffer, 0, VK_INDEX_TYPE_UINT32);
  vkCmdBindDescriptorSets(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, model->shader_handle->pipeline_layout, 0, 1, &model->descriptor_set, 0, NULL);
  vkCmdDrawIndexed(gpu_api->vulkan_state->gbuffer->gbuffer_command_buffer, model->model_mesh->indices->size, 1, 0, 0, 0);
}

void model_recreate(struct Model* model, struct GPUAPI* gpu_api) {
  model_vulkan_cleanup(model, gpu_api);

  graphics_utils_setup_vertex_buffer(gpu_api->vulkan_state, model->model_mesh->vertices, &model->vertex_buffer, &model->vertex_buffer_memory);
  graphics_utils_setup_index_buffer(gpu_api->vulkan_state, model->model_mesh->indices, &model->index_buffer, &model->index_buffer_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct ModelUniformBufferObject), &model->uniform_buffer, &model->uniform_buffers_memory);
  graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct LightingUniformBufferObject), &model->lighting_uniform_buffer, &model->lighting_uniform_buffers_memory);
  if (model->animated)
    graphics_utils_setup_uniform_buffer(gpu_api->vulkan_state, sizeof(struct ModelAnimationUniformBufferObject), &model->uniform_buffer, &model->uniform_buffers_memory);
  graphics_utils_setup_descriptor(gpu_api->vulkan_state, model->shader_handle->descriptor_set_layout, model->shader_handle->descriptor_pool, &model->descriptor_set);

  if (model->animated) {
    VkWriteDescriptorSet dcs[8] = {0};
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ModelUniformBufferObject), &model->uniform_buffer)});
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 1, &model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct LightingUniformBufferObject), &model->lighting_uniform_buffer)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 2, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_diffuse_texture->texture_image_view, &model->model_diffuse_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 3, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_normal_texture->texture_image_view, &model->model_normal_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 4, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_metallic_texture->texture_image_view, &model->model_metallic_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 5, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_roughness_texture->texture_image_view, &model->model_roughness_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 6, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_ao_texture->texture_image_view, &model->model_ao_texture->texture_sampler)});
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 7, &model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ModelAnimationUniformBufferObject), &model->uniform_buffer)});
    vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 8, dcs, 0, NULL);
  } else {
    VkWriteDescriptorSet dcs[7] = {0};
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 0, &model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct ModelUniformBufferObject), &model->uniform_buffer)});
    graphics_utils_setup_descriptor_buffer(gpu_api->vulkan_state, dcs, 1, &model->descriptor_set, (VkDescriptorBufferInfo[]){graphics_utils_setup_descriptor_buffer_info(sizeof(struct LightingUniformBufferObject), &model->lighting_uniform_buffer)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 2, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_diffuse_texture->texture_image_view, &model->model_diffuse_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 3, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_normal_texture->texture_image_view, &model->model_normal_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 4, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_metallic_texture->texture_image_view, &model->model_metallic_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 5, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_roughness_texture->texture_image_view, &model->model_roughness_texture->texture_sampler)});
    graphics_utils_setup_descriptor_image(gpu_api->vulkan_state, dcs, 6, &model->descriptor_set, (VkDescriptorImageInfo[]){graphics_utils_setup_descriptor_image_info(&model->model_ao_texture->texture_image_view, &model->model_ao_texture->texture_sampler)});
    vkUpdateDescriptorSets(gpu_api->vulkan_state->device, 7, dcs, 0, NULL);
  }
}

// TODO: Maybe get instanced/batched where specific vertex data is not needed
