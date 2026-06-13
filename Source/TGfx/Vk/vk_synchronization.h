#pragma once
#include <vector>

#include "vk_core.h"
#include "vk_predefinitions.h"

struct semaphore_vk {
 public:
  enum semaphore_status : unsigned char {
    invalid_status = 0,
    // No pending execution, state unknown (rendergraph or you should handle it)
    unused_status = 1,
    used_status   = 2
  };

 private:
  friend struct semaphoresys_vk;
  VkSemaphore SPHandle = VK_NULL_HANDLE;
#ifdef VULKAN_DEBUGGING
  semaphore_idtype_vk ID;
#endif
  semaphore_status current_status;

 public:
  inline semaphore_idtype_vk get_id() {
#ifdef VULKAN_DEBUGGING
    return ID;
#else
    return this;
#endif
  }
  inline VkSemaphore      vksemaphore() { return SPHandle; }
  inline semaphore_status status() { return current_status; };
};
struct semaphoresys_vk {
  inline static void ClearList() {
    printer(result_tgfx_NOTCODED, "SemaphoreSys->ClearList() isn't coded!");
  }
  // Searches for the previously created but later destroyed semaphores, if there isn't
  // Then creates a new VkSemaphore object
  inline static semaphore_vk& Create_Semaphore() {
    for (unsigned int i = 0; i < semaphoresys->Semaphores.size(); i++) {
      if (semaphoresys->Semaphores[i]->current_status == semaphore_vk::unused_status) {
        semaphoresys->Semaphores[i]->current_status = semaphore_vk::used_status;
        return *semaphoresys->Semaphores[i];
      }
    }
    VkSemaphoreCreateInfo Semaphore_ci = {};
    Semaphore_ci.flags                 = 0;
    Semaphore_ci.pNext                 = nullptr;
    Semaphore_ci.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkSemaphore vksemp = VK_NULL_HANDLE;
    if (vkCreateSemaphore(rendergpu->devLogical, &Semaphore_ci, nullptr, &vksemp) !=
        VK_SUCCESS) {
      printer(result_tgfx_FAIL,
              "Window creation has failed while creating semaphores for each swapchain texture!");
      return *semaphoresys->Semaphores[0];
    }
#ifdef VULKAN_DEBUGGING
    if (vksemp == VK_NULL_HANDLE) {
      printer(result_tgfx_FAIL, "vkCreateSemaphore() has failed!");
      return *semaphoresys->Semaphores[0];
    }
#endif

    semaphore_vk* NewSemaphore   = new semaphore_vk;
    NewSemaphore->current_status = semaphore_vk::used_status;
    NewSemaphore->SPHandle       = vksemp;
#ifdef VULKAN_DEBUGGING
    NewSemaphore->ID = semaphoresys->Semaphores.size();
#endif
    semaphoresys->Semaphores.push_back(NewSemaphore);
    return *NewSemaphore;
  }
  // If your semaphore object is signaled then unsignaled, you should destroy it
  // Vulkan object won't be destroyed, it is gonna be marked as "invalid" and then recycled
  // Create_Semaphore() will return the destroyed semaphore (because it is just marked as
  // "invalid").
  inline static void DestroySemaphore(semaphore_idtype_vk& SemaphoreID) {
    semaphore_vk& semaphore  = GetSemaphore_byID(SemaphoreID);
    semaphore.current_status = semaphore_vk::unused_status;
  }
  inline static semaphore_vk& GetSemaphore_byID(semaphore_idtype_vk ID) {
#ifdef VULKAN_DEBUGGING
    for (unsigned int i = 0; i < semaphoresys->Semaphores.size(); i++) {
      if (semaphoresys->Semaphores[i]->ID == ID) {
        return *semaphoresys->Semaphores[i];
      }
    }
    // First object is invalid
    return *semaphoresys->Semaphores[0];
#else
    return *( semaphore_vk* )ID;
#endif
  }
  semaphoresys_vk() {
    semaphore_vk* invalidone   = new semaphore_vk;
    invalidone->current_status = semaphore_vk::invalid_status;
#ifdef VULKAN_DEBUGGING
    invalidone->ID = invalid_semaphoreid;
#endif
    invalidone->SPHandle = VK_NULL_HANDLE;
    Semaphores.push_back(invalidone);
  }
  ~semaphoresys_vk() {}

 private:
  // First object in the list is an invalid semaphore to return in GetSemaphore() functions!
  std::vector<semaphore_vk*> Semaphores;
#ifdef VULKAN_DEBUGGING
  bool               isSorted = true;
  inline static void Sort() {
    printer(result_tgfx_NOTCODED, "semaphoresys->Sort() isn't written!");
  }
#endif
};

struct fencesys_vk {
 public:
  inline fence_vk& CreateFence() {
    for (unsigned int i = 0; i < fencesys->Fences.size(); i++) {
      if (fencesys->Fences[i]->current_status != fence_vk::used) {
        fencesys->Fences[i]->current_status = fence_vk::used;
        return *fencesys->Fences[i];
      }
    }
    VkFenceCreateInfo fence_ci = {};
    fence_ci.flags             = 0;
    fence_ci.pNext             = nullptr;
    fence_ci.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence vk_fence = VK_NULL_HANDLE;
    if (vkCreateFence(rendergpu->devLogical, &fence_ci, nullptr, &vk_fence) != VK_SUCCESS) {
      printer(result_tgfx_FAIL,
              "Window creation has failed while creating semaphores for each swapchain texture!");
      return *fencesys->Fences[0];
    }
#ifdef VULKAN_DEBUGGING
    if (vk_fence == VK_NULL_HANDLE) {
      printer(result_tgfx_FAIL, "vkCreateSemaphore() has failed!");
      return *fencesys->Fences[0];
    }
#endif

    fence_vk* new_fence = new fence_vk();
#ifdef VULKAN_DEBUGGING
    new_fence->ID = fencesys->Fences.size();
#endif
    new_fence->current_status = fence_vk::used;
    new_fence->Fence_o        = vk_fence;
    fencesys->Fences.push_back(new_fence);
    return *new_fence;
  }
  inline void waitfor_fences(const std::vector<fence_idtype_vk>& fences) {
    if (!fences.size()) {
      return;
    }
    std::vector<VkFence> Fence_objects(fences.size());
    for (unsigned int fence_i = 0; fence_i < fences.size(); fence_i++) {
      Fence_objects[fence_i] = getfence_byid(fences[fence_i]).Fence_o;
    }
    if (vkWaitForFences(rendergpu->devLogical, ( uint32_t )Fence_objects.size(),
                        Fence_objects.data(), ( uint32_t )VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
      printer(result_tgfx_FAIL, "vkWaitForFences() has failed!");
      return;
    }
    if (vkResetFences(rendergpu->devLogical, Fence_objects.size(), Fence_objects.data()) !=
        VK_SUCCESS) {
      printer(result_tgfx_FAIL, "VulkanRenderer: Fence reset has failed!");
    }
    for (unsigned int fence_i = 0; fence_i < fences.size(); fence_i++) {
      fence_vk& fence      = getfence_byid(fences[fence_i]);
      fence.current_status = fence_vk::unused;
      for (unsigned int semaphore_i = 0; semaphore_i < fence.wait_semaphores.size();
           semaphore_i++) {
        semaphoresys->DestroySemaphore(fence.wait_semaphores[semaphore_i]);
      }
      fence.wait_semaphores.clear();
    }
  }

 private:
  std::vector<fence_vk*> Fences;
};
