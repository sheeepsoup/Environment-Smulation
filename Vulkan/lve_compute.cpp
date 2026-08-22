#include "lve_compute.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <array>
#define EROSON_EXTENT 1000000//侵蚀n次
namespace lve {

    LveCompute::LveCompute(LveDevice& device, const std::string& computeShaderPath)
        : device(device), computeShaderPath(computeShaderPath) {
    }

    LveCompute::~LveCompute() {
        clean();
    }

    // 读取文件
    std::vector<char> LveCompute::readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("failed to open compute shader file: " + filename);
        }
        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }

    // 创建着色器模块
    VkShaderModule LveCompute::createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device.getDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute shader module!");
        }
        return shaderModule;
    }

    // 描述符集布局（绑定存储缓冲）
    void LveCompute::createDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};

        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        bindings[1].binding = 1;
        bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[1].descriptorCount = 1;
        bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor set layout!");
        }
    }

    // 描述符池
    void LveCompute::createDescriptorPool(uint32_t maxSets) {
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSize.descriptorCount = maxSets * 2;//由于流量也来了所以*2

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = maxSets;

        if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor pool!");
        }
    }

    // 管线布局
    void LveCompute::createPipelineLayout() {
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstantData);

        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &descriptorSetLayout;
        layoutInfo.pushConstantRangeCount = 1; //上传
        layoutInfo.pPushConstantRanges = &pushConstantRange;


        if (vkCreatePipelineLayout(device.getDevice(), &layoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }
    }

    // 创建计算管线
    void LveCompute::createComputePipeline() {
        auto computeCode = readFile(computeShaderPath);
        computeShaderModule = createShaderModule(computeCode);

        VkPipelineShaderStageCreateInfo stageInfo{};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stageInfo.module = computeShaderModule;
        stageInfo.pName = "main";

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.stage = stageInfo;
        pipelineInfo.layout = pipelineLayout;

        if (vkCreateComputePipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }
    }

    // ★ 新增：创建存储缓冲区（对应你问的 computeBuffer）
    void LveCompute::createStorageBuffers(uint32_t count, VkDeviceSize size) {
        bufferSize = size;
        storageBuffers.resize(count);
        storageBuffersMemory.resize(count);
        storageBuffersMapped.resize(count);
        flowBuffers.resize(count);
        flowBuffersMemory.resize(count);
        flowBuffersMapped.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            // 使用你现成的 device.createBuffer
            device.createBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, // ★ 重点是 Storage Buffer
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                storageBuffers[i],
                storageBuffersMemory[i]
            );

            // 映射内存，方便 CPU 随时更新数据
            vkMapMemory(device.getDevice(), storageBuffersMemory[i], 0, size, 0, &storageBuffersMapped[i]);
            //流量
            device.createBuffer(
                size,
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                flowBuffers[i],
                flowBuffersMemory[i]
            );

            vkMapMemory(
                device.getDevice(),
                flowBuffersMemory[i],
                0,
                size,
                0,
                &flowBuffersMapped[i]
            );

            // 原始显存内容不是自动为0
            std::memset(flowBuffersMapped[i], 0, static_cast<size_t>(size));
        }
    }

    // ★ 新增：更新描述符集（把刚才创建的 Buffer 绑定到描述符集）
    void LveCompute::updateDescriptorSets() {
        for (uint32_t i = 0; i < descriptorSets.size(); i++) {
            VkDescriptorBufferInfo bufferInfos[2]{};

            bufferInfos[0].buffer = storageBuffers[i];
            bufferInfos[0].offset = 0;
            bufferInfos[0].range = bufferSize;

            bufferInfos[1].buffer = flowBuffers[i];
            bufferInfos[1].offset = 0;
            bufferInfos[1].range = bufferSize;

            VkWriteDescriptorSet writes[2]{};

            writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[0].dstSet = descriptorSets[i];
            writes[0].dstBinding = 0;
            writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[0].descriptorCount = 1;
            writes[0].pBufferInfo = &bufferInfos[0];

            writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[1].dstSet = descriptorSets[i];
            writes[1].dstBinding = 1;
            writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[1].descriptorCount = 1;
            writes[1].pBufferInfo = &bufferInfos[1];

            vkUpdateDescriptorSets(
                device.getDevice(),
                2,
                writes,
                0,
                nullptr
            );
        }
    }

    // 初始化（整合所有步骤）
    void LveCompute::init(uint32_t maxFramesInFlight, VkDeviceSize bufferSize) {
        createDescriptorSetLayout();
        createDescriptorPool(maxFramesInFlight);
        createPipelineLayout();
        createComputePipeline();

        // 分配描述符集
        descriptorSets.resize(maxFramesInFlight);
        std::vector<VkDescriptorSetLayout> layouts(maxFramesInFlight, descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = maxFramesInFlight;
        allocInfo.pSetLayouts = layouts.data();

        if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate compute descriptor sets!");
        }

        // ★ 创建存储缓冲并绑定到描述符集
        createStorageBuffers(maxFramesInFlight, bufferSize);
        updateDescriptorSets();
    }

    // 更新数据（从 CPU 传到 GPU）
    void LveCompute::updateStorageBuffer(uint32_t frameIndex, void* data, VkDeviceSize size) {
        if (frameIndex >= storageBuffersMapped.size()) return;
        memcpy(storageBuffersMapped[frameIndex], data, size);
    }

    // 记录计算命令
    void LveCompute::recordComputeCommands(VkCommandBuffer cmdBuffer, uint32_t frameIndex, int width) {
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[frameIndex], 0, nullptr);
        const uint32_t totalDroplets = EROSON_EXTENT;
        PushConstantData pushData{};
        pushData.width = width;
        pushData.waterDorpNum = totalDroplets;
        vkCmdPushConstants(cmdBuffer, pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstantData), &pushData);

  
        const uint32_t groupSize = 64;  // 和着色器里的 local_size_x 保持一致
        uint32_t groupCount = (totalDroplets + groupSize - 1) / groupSize; // 结果 = 15625

        vkCmdDispatch(cmdBuffer, groupCount, 1, 1);
    }

    // 清理
    void LveCompute::clean() {
        VkDevice vkDevice = device.getDevice();

        for (size_t i = 0; i < storageBuffers.size(); i++) {
            vkUnmapMemory(vkDevice, storageBuffersMemory[i]); // 解除映射
            vkDestroyBuffer(vkDevice, storageBuffers[i], nullptr);
            vkFreeMemory(vkDevice, storageBuffersMemory[i], nullptr);
        }
        storageBuffers.clear();
        storageBuffersMemory.clear();
        storageBuffersMapped.clear();

        if (computePipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(vkDevice, computePipeline, nullptr);
            computePipeline = VK_NULL_HANDLE;
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(vkDevice, pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(vkDevice, descriptorPool, nullptr);
            descriptorPool = VK_NULL_HANDLE;
        }
        if (descriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(vkDevice, descriptorSetLayout, nullptr);
            descriptorSetLayout = VK_NULL_HANDLE;
        }
        if (computeShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(vkDevice, computeShaderModule, nullptr);
            computeShaderModule = VK_NULL_HANDLE;
        }
        for (size_t i = 0; i < flowBuffers.size(); i++) {
            vkUnmapMemory(vkDevice, flowBuffersMemory[i]);
            vkDestroyBuffer(vkDevice, flowBuffers[i], nullptr);
            vkFreeMemory(vkDevice, flowBuffersMemory[i], nullptr);
        }

        flowBuffers.clear();
        flowBuffersMemory.clear();
        flowBuffersMapped.clear();
    }

    void LveCompute::runErosionSync(LveDevice& device, uint32_t bufferIndex, int mapVertexCount, std::vector<int32_t>& heightData,
        std::vector<uint32_t>& flowData, VkDeviceSize bufferSize) {
        //计算地形
        updateStorageBuffer(bufferIndex, heightData.data(), bufferSize);
        //提交一次计算
        VkCommandBuffer computeCmdBuf;
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = device.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;
        vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &computeCmdBuf);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(computeCmdBuf, &beginInfo);
        recordComputeCommands(computeCmdBuf, bufferIndex, mapVertexCount); // 只跑一次，用第0帧的 descriptor
        vkEndCommandBuffer(computeCmdBuf);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCmdBuf;

        VkFence computeFence;
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &computeFence);

        vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, computeFence);
        vkWaitForFences(device.getDevice(), 1, &computeFence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device.getDevice(), computeFence, nullptr);
        vkFreeCommandBuffers(device.getDevice(), device.getCommandPool(), 1, &computeCmdBuf);
        //计算完毕拷回来
        memcpy(heightData.data(), getMappedData(bufferIndex), bufferSize);
        memcpy(flowData.data(),getFlowMappedData(bufferIndex), sizeof(uint32_t) * flowData.size());
       
    };
} // namespace lve