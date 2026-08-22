//后面着色器换成slang



#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>
#include<vulkan/vulkan.h>
#include<cstdlib>
#include<iostream>
#include<stdexcept>
#include"lve_windows.h"
#include"lve_pipeline.h"
#include"lve_device.h"
#include"lve_swapChain.h"
#include"lve_renderPass.h"
#include"lve_renderer.h"
#include"lve_model.h"
#include"lve_uniform.h"
#include"lve_camera.h"
#include"lve_compute.h"
#include"lve_terrain.h"
#include <set>
#include<vector>
//----------------------------------------------------------------------------------------
// 参数控制
const int seed = 114514;//地图种子
const float HEIGHT_FIXED_SCALE = 10000.0f;//这个用于int->float还原,不用改
//----------------------------------------------------------------------------------------
//本地无限地形生成开关
bool unlimitedArea = false;
//----------------------------------------------------------------------------------------
lve::LveWindows win(1366,768,"从零开始的vulkan生活");//窗口
lve::Lvepipeline pipeLine("shader/simple_shader.vert.spv", "shader/simple_shader.frag.spv");
lve::LveDevice device;
lve::LveSwapChain swapChain;
lve::LveRenderPass renderPass;
lve::LveRenderer renderer;
lve::LveModel model;
lve::LveUniform uniform;
lve::LveCamera camera;
lve::LveCompute compute(device, "shader/compute.comp.spv");
lve::LveTerrain terrain;

uint32_t currentFrame = 0;//当前帧




#pragma region 消息回调
VkDebugUtilsMessengerEXT callback;
//回调函数
static VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "error: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

#pragma endregion

void clean() {
	vkDeviceWaitIdle(device.getDevice());
	pipeLine.clean(device.getDevice());
	swapChain.cleanupSwapChain(device.getDevice());
	renderPass.clean(device.getDevice());
	renderer.clean(device.getDevice());
	win.cleanSurface(device.getInstance());
	model.clean(device.getDevice());
	compute.clean();
	uniform.clean(device.getDevice(),renderer.getMaxFramesInFlight());
	auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(device.getInstance(), "vkDestroyDebugUtilsMessengerEXT");
	if (vkDestroyDebugUtilsMessengerEXT) {
		vkDestroyDebugUtilsMessengerEXT(device.getInstance(), callback, nullptr);
	}
	device.clean(device.getDevice(), device.getInstance());
}




int main() {
	//初始化

	
	//3.创建实例


	device.createInstance();
	//创建回调
	// 创建回调
	if (enableValidationLayers) {
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity =
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugUtilsCallback;  // 你的回调函数
		debugCreateInfo.pUserData = nullptr;  // 可传递自定义数据

		// 加载创建函数
		auto vkCreateDebugUtilsMessengerEXT =
			(PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(device.getInstance(), "vkCreateDebugUtilsMessengerEXT");

		if (vkCreateDebugUtilsMessengerEXT) {
			VkResult debugResult = vkCreateDebugUtilsMessengerEXT(device.getInstance(), &debugCreateInfo, nullptr, &callback);
			if (debugResult != VK_SUCCESS) {
				std::cout << "create callback error!! error code: " << debugResult << std::endl;
			}
		}
		else {
			std::cout << "unable load vkCreateDebugUtilsMessengerEXT" << std::endl;
		}
	}

	//创建表面
	win.createWindowSurface(device.getInstance(), win.win);
	device.pickPhysicalDevice();//选择物理设备
	//创建逻辑设备
	//创建队列
	device.createQueueFamiliesIndices(win);
	//创建逻辑设备
	device.createLogicalDevice();
	//检索列队句柄
	device.createArrHandle();
	
	//创建交换链
	swapChain.createSwapChain( device,win);
	
	

	//前提都设置好了---------------------------------------------------------------------------开始整画面

	pipeLine.initPipeline();//初始化管线,设置视口等信息
	device.createCommandPool(swapChain.getQueueFamilyIndices_what(0));//创建命令池
	//深度缓冲
	swapChain.createDepthResources(device);
	
	//渲染过程----------------------------------
	pipeLine.createShader(device.getDevice());//创建着色器模块
	

	using Clock = std::chrono::steady_clock;

	auto start = Clock::now();
	//初始化地形
	terrain.processArea(seed);


	auto afterTerrain = Clock::now();
	std::cout << "terrain generation: "<< std::chrono::duration<float>(afterTerrain - start).count() << " seconds\n";

	//创建计算着色器
	VkDeviceSize computeBufferSize = sizeof(int32_t) * terrain.getHeightData().size();
	compute.init(renderer.getMaxFramesInFlight(), computeBufferSize);

	
	//侵蚀模拟计算
	std::vector<int32_t> heightUint(terrain.getHeightData().size());//高度数据
	std::vector<uint32_t> flowUint(	terrain.getHeightData().size(), 0);//流量数据
	auto erosionStart = Clock::now();
	for (size_t i = 0; i < 	terrain.getHeightData().size(); i++) {
		heightUint[i] = static_cast<int32_t>(	terrain.getHeightData()[i] * HEIGHT_FIXED_SCALE + 0.5f);
	}
	compute.runErosionSync(device, 0, terrain.getMapVertexNum(), heightUint, flowUint, computeBufferSize);
	terrain.updateHeightFlow(heightUint, flowUint, HEIGHT_FIXED_SCALE);
	

	
	auto erosionEnd = Clock::now();

	std::cout << "GPU erosion: "
		<< std::chrono::duration<float>(
			erosionEnd - erosionStart).count()
		<< " seconds\n";
	// 重新计算法线
	auto normalStart = Clock::now();
	terrain.calculateNormal(); //
	auto normalEnd = Clock::now();
	std::cout << "normal calculation: "
		<< std::chrono::duration<float>(
			normalEnd - normalStart).count()
		<< " seconds\n";

	//放大地形
	terrain.SetModelSize(2);

	//[2选1]具体区别看model.h
	//model.createVertexBuffer(device);//创建顶点缓冲区
	model.createVertexBufferWithStaging(device, terrain.getVertices());//创建顶点缓冲区,使用staging buffer
	model.createIndexBufferWithStaging(device, terrain.getIndices());//创建索引缓冲区


	pipeLine.distritbutePipeline();//分配管线

	//创建布局
	uniform.createDescriptorSetLayout(device.getDevice());
	uint32_t MAX_FLIT_FAMES = renderer.getMaxFramesInFlight();
	//创建UBO
	uniform.createUniformBuffer(MAX_FLIT_FAMES, device);
	//创建描述符池
	uniform.createDescriptorPool(MAX_FLIT_FAMES, device.getDevice());
	//分配并更新描述符集合
	uniform.createDescriptorSets(MAX_FLIT_FAMES, device.getDevice());

	//管线布局
	pipeLine.createPipelineLayout(device.getDevice(),uniform.getDescriptorSetLayout());//创建管线布局



	renderPass.createRenderPass(swapChain.getSwapChainSurfaceFormat(), device.findDepthFormat(),device.getDevice());//创建渲染通道



	//创建图像管线--------------------------------------------------


	pipeLine.createOthers();//创建其他管线相关信息,如视口,裁切矩形等

	pipeLine.createpipeline(renderPass.getRenderPass(), device.getDevice(),renderer.getGraphicsPipeline());//创建图像管线

	//绘制部分+--------------------------------------------------------------------
	//创建帧缓冲对象

	swapChain.createFrameBuffer(device, renderPass.getRenderPass());



	renderer.createCommandBuffers(device.getDevice(), device.getCommandPool(), swapChain.getSwapChainImageCount());//创建命令缓冲区)


	renderer.createSignalSemaphore(device.getDevice(), swapChain.getSwapChainImageCount());//创建信号量

	//初始化摄像机
	camera.setViewDirection(
		glm::vec3{ 2.0f, 2.0f, 80.0f },   // 摄像机位置
		glm::vec3{ -1.0f, -1.0f, -1.0f }, // 摄像机方向
		glm::vec3{ 0.0f, 0.0f, 1.0f });   // Z 轴向上

	camera.setPerspectiveProjection(
		glm::radians(45.0f),
		swapChain.getSwapChainExtent().width / static_cast<float>(swapChain.getSwapChainExtent().height),
		0.1f,//近裁截面
		300.0f);//远裁截面


	SDL_Event event;
	SDL_SetWindowRelativeMouseMode(win.win, true);//启用相对鼠标模式
	Uint64 lastTime = SDL_GetTicks();
	glm::mat4 modelMatrix{ 1.0f };

	modelMatrix = glm::translate(modelMatrix,glm::vec3{ 0.0f, 0.0f, 0.0f });

	modelMatrix = glm::rotate(modelMatrix,glm::radians(45.0f),glm::vec3{ 0.0f, 0.0f, 1.0f });

	modelMatrix = glm::scale(modelMatrix,glm::vec3{ 1.0f, 1.0f, 1.0f });




	while (1) {
		const Uint64 currentTime = SDL_GetTicks();

		const float dealtTime =
			static_cast<float>(currentTime - lastTime) / 1000.0f;
		lastTime = SDL_GetTicks();
		while (SDL_PollEvent(&event)) {//处理事件
			if (event.type == SDL_EVENT_QUIT) {
				clean();
				return EXIT_SUCCESS;
			}
			if (event.type == SDL_EVENT_MOUSE_MOTION) {
				camera.rotate(
					event.motion.xrel,
					event.motion.yrel);
			}
		}
		const bool* keyboardState = SDL_GetKeyboardState(nullptr);

		if (keyboardState[SDL_SCANCODE_W]) {
			camera.forward_and_behind(true, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_S]) {
			camera.forward_and_behind(false, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_A]) {
			camera.right_and_left(false, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_D]) {
			camera.right_and_left(true, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_SPACE]) {
			camera.up_and_down(true, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_LSHIFT]) {
			camera.up_and_down(false, dealtTime);
		}
		if (keyboardState[SDL_SCANCODE_ESCAPE]) {
			clean();
			return EXIT_SUCCESS;
		}
		
		renderer.run(device.getDevice(), swapChain, device.getGraphicsQueue(), device.getPresentQueue(),
			currentFrame, renderPass.getRenderPass(),model,uniform.getDescriptorSets(),pipeLine.getPipelineLayout(),
			uniform, modelMatrix,camera.getView(),camera.getProjection(),compute,camera.getPos(),terrain.getIndices());
		

	}

	system("pause");
	clean();


	return EXIT_SUCCESS;

}
