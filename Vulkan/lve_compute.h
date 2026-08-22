#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "lve_device.h"

namespace lve {

    class LveCompute {
    public:
        LveCompute(LveDevice& device, const std::string& computeShaderPath);
        ~LveCompute();

        // 禁止拷贝
        LveCompute(const LveCompute&) = delete;
        LveCompute& operator=(const LveCompute&) = delete;

        // 初始化：传入帧数量和数据大小
        void init(uint32_t maxFramesInFlight, VkDeviceSize bufferSize);

        // 更新数据（CPU -> GPU）
        void updateStorageBuffer(uint32_t frameIndex, void* data, VkDeviceSize size);

        // 记录计算命令
        void recordComputeCommands(VkCommandBuffer cmdBuffer, uint32_t frameIndex, int width);

        // 清理
        void clean();

        // 获取资源
        const std::vector<VkDescriptorSet>& getDescriptorSets() const { return descriptorSets; }
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }
        void* getMappedData(uint32_t frameIndex) const { return storageBuffersMapped[frameIndex]; }
        //上传的内容 上传地图大小
        struct PushConstantData {
            int width;
            int waterDorpNum;//雨滴数量
        };
        void* getFlowMappedData(uint32_t frameIndex) const {
            return flowBuffersMapped[frameIndex];
        }


        void runErosionSync(LveDevice& device, uint32_t bufferIndex, int mapVertexCount, std::vector<int32_t>& heightData,
            std::vector<uint32_t>& flowData, VkDeviceSize bufferSize);
    private:
        LveDevice& device;
        std::string computeShaderPath;
        VkDeviceSize bufferSize = 0;

        // Vulkan 对象
        VkShaderModule computeShaderModule = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkPipeline computePipeline = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> descriptorSets;

        // 存储缓冲（计算数据）
        std::vector<VkBuffer> storageBuffers;
        std::vector<VkDeviceMemory> storageBuffersMemory;
        std::vector<void*> storageBuffersMapped;

        std::vector<VkBuffer> flowBuffers;
        std::vector<VkDeviceMemory> flowBuffersMemory;
        std::vector<void*> flowBuffersMapped;

        // 内部辅助函数
        std::vector<char> readFile(const std::string& filename);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        void createDescriptorSetLayout();
        void createDescriptorPool(uint32_t maxSets);
        void createPipelineLayout();
        void createComputePipeline();
        void createStorageBuffers(uint32_t count, VkDeviceSize size);
        void updateDescriptorSets();  // ★ 这里之前漏了，现在补上
    };

} // namespace lve