#include "mana/core/vulkancore.h"

static int vulkan_core_create_instance(struct VulkanState* vulkan_state, const char** graphics_lbrary_extensions, uint32_t* graphics_library_extension_count);
static int vulkan_core_setup_debug_messenger(struct VulkanState* vulkan_state);
static int vulkan_core_pick_physical_device(struct VulkanState* vulkan_state, const char** graphics_lbrary_extensions);
static bool vulkan_core_device_can_render(struct VulkanState* vulkan_state, VkPhysicalDevice device);
static int vulkan_core_create_logical_device(struct VulkanState* vulkan_state);
static int vulkan_core_create_command_pool(struct VulkanState* vulkan_state);
static bool vulkan_core_check_validation_layer_support(struct VulkanState* vulkan_state);
static void vulkan_command_pool_cleanup(struct VulkanState* vulkan_state);
static void vulkan_device_cleanup(struct VulkanState* vulkan_state);
static void vulkan_debug_cleanup(struct VulkanState* vulkan_state);
static void vulkan_core_destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator);

int vulkan_core_init(struct VulkanState* vulkan_state, const char** graphics_lbrary_extensions, uint32_t* graphics_library_extension_count) {
  vulkan_state->framebuffer_resized = false;
  vulkan_state->physical_device = VK_NULL_HANDLE;

  int vulkan_error_code;
  if ((vulkan_error_code = vulkan_core_create_instance(vulkan_state, graphics_lbrary_extensions, graphics_library_extension_count)) != VULKAN_CORE_SUCCESS)
    goto vulkan_core_create_instance_cleanup;
  if ((vulkan_error_code = vulkan_core_setup_debug_messenger(vulkan_state)) != VULKAN_CORE_SUCCESS)
    goto vulkan_debug_error;
  if ((vulkan_error_code = vulkan_core_pick_physical_device(vulkan_state, graphics_lbrary_extensions)) != VULKAN_CORE_SUCCESS)
    goto vulkan_pick_device_error;
  if ((vulkan_error_code = vulkan_core_create_logical_device(vulkan_state)) != VULKAN_CORE_SUCCESS)
    goto vulkan_device_error;
  if ((vulkan_error_code = vulkan_core_create_command_pool(vulkan_state)) != VULKAN_CORE_SUCCESS)
    goto vulkan_command_pool_error;

  return vulkan_error_code;

vulkan_command_pool_error:
  vulkan_command_pool_cleanup(vulkan_state);
vulkan_device_error:
  vulkan_device_cleanup(vulkan_state);
vulkan_pick_device_error:
vulkan_debug_error:
  vulkan_debug_cleanup(vulkan_state);
vulkan_core_create_instance_cleanup:

  return vulkan_error_code;
}

void vulkan_core_delete(struct VulkanState* vulkan_state) {
  vulkan_command_pool_cleanup(vulkan_state);
  vulkan_device_cleanup(vulkan_state);
  vulkan_debug_cleanup(vulkan_state);
}

static void vulkan_device_cleanup(struct VulkanState* vulkan_state) {
  vkDestroyDevice(vulkan_state->device, NULL);
}

static void vulkan_command_pool_cleanup(struct VulkanState* vulkan_state) {
  vkDestroyCommandPool(vulkan_state->device, vulkan_state->command_pool, NULL);
}

static void vulkan_debug_cleanup(struct VulkanState* vulkan_state) {
  if (enable_validation_layers)
    vulkan_core_destroy_debug_utils_messenger_ext(vulkan_state->instance, vulkan_state->debug_messenger, NULL);
}

static void vulkan_core_destroy_debug_utils_messenger_ext(VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger, const VkAllocationCallbacks* p_allocator) {
  PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != NULL)
    func(instance, debug_messenger, p_allocator);
}

static int vulkan_core_create_instance(struct VulkanState* vulkan_state, const char** graphics_lbrary_extensions, uint32_t* graphics_library_extension_count) {
  if (enable_validation_layers && !vulkan_core_check_validation_layer_support(vulkan_state))
    return VULKAN_CORE_CREATE_INSTANCE_ERROR;

  VkApplicationInfo app_info = {0};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  // TODO: Pull name and version from engine
  app_info.pApplicationName = "TODO";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "Mana";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo create_info = {0};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;

  //auto extensions = getRequiredExtensions();

  if (enable_validation_layers) {
    graphics_lbrary_extensions[*graphics_library_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;  //"VK_EXT_DEBUG_UTILS_EXTENSION_NAME\0";
    (*graphics_library_extension_count)++;
  }

  //

  create_info.enabledExtensionCount = *graphics_library_extension_count;
  create_info.ppEnabledExtensionNames = graphics_lbrary_extensions;

  if (enable_validation_layers) {
    create_info.enabledLayerCount = (uint32_t)1;
    create_info.ppEnabledLayerNames = validation_layers;
  } else
    create_info.enabledLayerCount = 0;

  int vulkan_instance_status = vkCreateInstance(&create_info, NULL, &vulkan_state->instance);

  if (vulkan_instance_status == VK_ERROR_INCOMPATIBLE_DRIVER)
    fprintf(stderr, "GPU driver is incompatible with vulkan!\n");

  if (vulkan_instance_status != VK_SUCCESS)
    return VULKAN_CORE_CREATE_INSTANCE_ERROR;

  return VULKAN_CORE_SUCCESS;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity, VkDebugUtilsMessageTypeFlagsEXT message_type, const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data, void* p_uiser_data) {
  fprintf(stderr, "validation layer: %s\n", p_callback_data->pMessage);
  return VK_FALSE;
}

static VkResult create_debug_utils_messenger_ext(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* p_create_info, const VkAllocationCallbacks* p_allocator, VkDebugUtilsMessengerEXT* p_debug_messenger) {
  PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != NULL)
    return func(instance, p_create_info, p_allocator, p_debug_messenger);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

static int vulkan_core_setup_debug_messenger(struct VulkanState* vulkan_state) {
  if (enable_validation_layers && !vulkan_core_check_validation_layer_support(vulkan_state))
    return VULKAN_CORE_SETUP_DEBUG_MESSENGER_ERROR;

  if (enable_validation_layers) {
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {0};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = debug_callback;

    if (create_debug_utils_messenger_ext(vulkan_state->instance, &debugInfo, NULL, &vulkan_state->debug_messenger) != VK_SUCCESS)
      return VULKAN_CORE_SETUP_DEBUG_MESSENGER_ERROR;
  }

  return VULKAN_CORE_SUCCESS;
}

static int vulkan_core_pick_physical_device(struct VulkanState* vulkan_state, const char** graphics_lbrary_extensions) {
  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(vulkan_state->instance, &device_count, NULL);

  if (device_count == 0)
    return VULKAN_CORE_PICK_PHYSICAL_DEVICE_ERROR;

  VkPhysicalDevice* devices = malloc(sizeof(VkPhysicalDevice) * device_count);
  vkEnumeratePhysicalDevices(vulkan_state->instance, &device_count, devices);

  VkPhysicalDeviceProperties current_device_properties = {0};
  uint32_t current_largest_heap = 0;
  bool discrete_selected = false;

  for (int device_num = 0; device_num < device_count; device_num++) {
    VkPhysicalDevice device = devices[device_num];
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device, &device_properties);

    if (vulkan_core_device_can_render(vulkan_state, device)) {
      //if (graphics_lbrary_extensions != NULL && !vulkan_core_device_can_present(vulkan_state, device))
      //  continue;

      // Check extension support function call

      if (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        VkPhysicalDeviceMemoryProperties device_memory_properties = {0};
        vkGetPhysicalDeviceMemoryProperties(device, &device_memory_properties);

        for (int heap_num = 0; heap_num < device_memory_properties.memoryHeapCount; heap_num++) {
          VkMemoryHeap device_heap = device_memory_properties.memoryHeaps[heap_num];

          if (device_heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT && device_heap.size > current_largest_heap) {
            discrete_selected = true;
            current_largest_heap = device_heap.size;
            current_device_properties = device_properties;
            vulkan_state->physical_device = device;
          }
        }
      } else if (current_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
        if (discrete_selected == false) {
          current_device_properties = device_properties;
          vulkan_state->physical_device = device;
        }
      } else
        printf("Unknown device: %s\n", device_properties.deviceName);
    }
  }

  printf("Selected device: %s\n", current_device_properties.deviceName);

  free(devices);

  if (vulkan_state->physical_device == VK_NULL_HANDLE)
    return VULKAN_CORE_PICK_PHYSICAL_DEVICE_ERROR;

  return VULKAN_CORE_SUCCESS;
}

static bool vulkan_core_device_can_render(struct VulkanState* vulkan_state, VkPhysicalDevice device) {
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, NULL);

  VkQueueFamilyProperties* queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families);

  bool graphics_family_found = false;
  for (int queue_family_num = 0; queue_family_num < queue_family_count; queue_family_num++) {
    if (queue_families[queue_family_num].queueCount > 0 && queue_families[queue_family_num].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      (&vulkan_state->indices)->graphics_family = queue_family_num;
      graphics_family_found = true;
      break;
    }
  }

  free(queue_families);

  if (!graphics_family_found)
    return false;

  return true;
}

static int vulkan_core_create_logical_device(struct VulkanState* vulkan_state) {
  const uint32_t unique_queue_families[2] = {vulkan_state->indices.graphics_family, vulkan_state->indices.present_family};
  const int unique_queue_family_count = (unique_queue_families[0] == unique_queue_families[1]) ? 1 : 2;

  VkDeviceQueueCreateInfo queue_create_infos[2] = {0};
  float queue_priority = 1.0f;
  for (int queue_num = 0; queue_num < unique_queue_family_count; queue_num++) {
    VkDeviceQueueCreateInfo queue_create_info = {0};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = queue_num;
    queue_create_info.queueCount = 1;
    queue_create_info.pQueuePriorities = &queue_priority;
    queue_create_infos[queue_num] = queue_create_info;
  }

  struct VkPhysicalDeviceFeatures device_features = {0};
  device_features.samplerAnisotropy = VK_TRUE;

  struct VkDeviceCreateInfo device_info = {0};
  device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  device_info.queueCreateInfoCount = (uint32_t)unique_queue_family_count;
  device_info.pQueueCreateInfos = queue_create_infos;

  device_info.pEnabledFeatures = &device_features;

  device_info.enabledExtensionCount = (uint32_t)VULKAN_DEVICE_EXTENSION_COUNT;
  device_info.ppEnabledExtensionNames = device_extensions;

  if (enable_validation_layers) {
    device_info.enabledLayerCount = (uint32_t)VULKAN_VALIDATION_LAYER_COUNT;
    device_info.ppEnabledLayerNames = validation_layers;
  } else
    device_info.enabledLayerCount = 0;

  if (vkCreateDevice(vulkan_state->physical_device, &device_info, NULL, &vulkan_state->device) != VK_SUCCESS)
    return VULKAN_CORE_CREATE_LOGICAL_DEVICE_ERROR;

  vkGetDeviceQueue(vulkan_state->device, vulkan_state->indices.graphics_family, 0, &vulkan_state->graphics_queue);
  vkGetDeviceQueue(vulkan_state->device, vulkan_state->indices.present_family, 0, &vulkan_state->present_queue);

  return VULKAN_CORE_SUCCESS;
}

static int vulkan_core_create_command_pool(struct VulkanState* vulkan_state) {
  VkCommandPoolCreateFlags command_pool_flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

  VkCommandPoolCreateInfo pool_info = {0};
  pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  pool_info.queueFamilyIndex = vulkan_state->indices.graphics_family;
  pool_info.flags = command_pool_flags;

  if (vkCreateCommandPool(vulkan_state->device, &pool_info, NULL, &vulkan_state->command_pool) != VK_SUCCESS)
    return VULKAN_CORE_CREATE_COMMAND_POOL_ERROR;

  return VULKAN_CORE_SUCCESS;
}

static bool vulkan_core_check_validation_layer_support(struct VulkanState* vulkan_state) {
  uint32_t layer_count;
  vkEnumerateInstanceLayerProperties(&layer_count, NULL);

  VkLayerProperties* available_layers = malloc(sizeof(VkLayerProperties) * layer_count);
  vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

  for (int layer_name_num = 0; layer_name_num < VULKAN_VALIDATION_LAYER_COUNT; layer_name_num++) {
    bool layer_found = false;

    for (int layer_property_num = 0; layer_property_num < layer_count; layer_property_num++) {
      if (strcmp(validation_layers[layer_name_num], available_layers[layer_property_num].layerName) == 0) {
        layer_found = true;
        break;
      }
    }

    if (layer_found == false)
      return false;
  }

  free(available_layers);

  return true;
}
