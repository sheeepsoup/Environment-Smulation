#pragma once
#include<vulkan/vulkan.h>
#include<glm/glm.hpp>
#include <stdint.h>
#include<iostream>
#include <array>
#include<vector>
#include<FastNoiseLite.h>
#include <random>
#include<thread>
#include"lve_model.h"

namespace lve {
	class LveTerrain
	{
	public:
		void initNoise(int seed);//初始化噪声
		void processArea(int seed);//生成地形
		void calculateNormal();//计算法线
		void SetModelSize(uint32_t scale) {//缩放地形大小
			for (auto& vertex : vertices) {
				vertex.pos *= scale;
			}
		};
		void updateHeightFlow(std::vector<int32_t>& heightData, std::vector<uint32_t>& flowData, float SCALE);//更新从gpu拿到的侵蚀数据

		uint32_t getMapVertexNum() { return mapVertexCount; };
		std::vector<uint32_t>& getIndices() { return indices; };
		const std::vector<uint32_t>& getIndices() const { return indices; };
		std::vector<LveModel::Vertex> &getVertices() { return vertices; };
		const std::vector<LveModel::Vertex>& getVertices() const { return vertices; };
		std::vector<float> &getHeightData() { return heightData; };

		int mapVertexCount;//地图顶点大小[x/y方向]
	private:
		#define EROSON_EXTENT 1000000//侵蚀n次
		#define WATER_MAX_STEP 500//最大步数
		#define MIN_WATER 0.01f//蒸发最小水量
		#define MIN_SPEED 0.01f//最小速度
		#define THREAD_PROCESS_NUM 1//线程处理次数

		float capacityFactor = 4.0f;
		float erosionRate = 0.1f;
		float depositionRate = 0.03f;
		float inertia = 0.6f;
		float gravity = 4.0f;
		glm::vec3 WorldUp = glm::vec3(0.0f, 0.0f, 1.0f);
		float terrainHeighLimite = 6.0f;//丘陵的最大高度
		int BlockNum = 50;//区块数量
		int BlockVertexNum = 20;//每个区块x/y对应的顶点数,该区块含有n*n个顶点
		float BlockDistance = 5.0;//每个区块的x/y对应的距离大小
		uint32_t cpuThreadNum;//cpu线程数

		class CNoise
		{
		public:
			FastNoiseLite biomeNoise;
			FastNoiseLite terrainNoise;
			FastNoiseLite mountainNoise;
			FastNoiseLite detailNoise;
			FastNoiseLite warpNoise;
			FastNoiseLite matrialNoise;//材质噪声,实现草地/岩石等的地域分布稍打乱
			FastNoiseLite snowStream;//雪地的流线图
			FastNoiseLite mountainBaseNoise;
			FastNoiseLite mountainRidgeNoise;
			FastNoiseLite erosionNoise;
		private:

		};
		float smoothstep(float edge0, float edge1, float value);
		CNoise noise;//噪声
		struct WaterDrop
		{
			glm::vec2 position; //当前位置
			glm::vec2 direction;//移动方向
			uint32_t step;      //步数
			float speed;        //速度
			float water;        //剩余水量
			float sediment;     //当前携带的泥沙量
			float maxSediment;  //最大携沙量
		};
		std::vector<LveModel::Vertex> vertices;
		std::vector<float> heightData;//高度数据
		std::vector<WaterDrop> erosion;//腐蚀
		std::vector<uint32_t> indices;//这里indice用于索引缓冲区,数字代表第n个三角形的点,详细问gpt不好解释


		float getHeight(float WorldX, float WorldY, bool isFirst);
		glm::vec3 calculateNormal(float worldX, float worldY, float sampleDistance);
		glm::vec3 calculateNormalNew(const glm::vec2& gridPosition);//专门用于腐蚀计算的法线计算函数
		glm::vec4 getGroundWeight(float height, glm::vec3 normal, glm::vec2 WorldPos);//获取当前土地的属性权重[r]草 [g]泥土 [b]岩石 [a]雪
		glm::vec3 getMatrial(glm::vec4 weight);//获取材质,输入权重
		void cacluateErosion();//模拟侵蚀
		int randomInt(int minValue, int maxValue)
		{
			static std::mt19937 generator{
				std::random_device{}()
			};

			std::uniform_int_distribution<int> distribution(
				minValue,
				maxValue
			);

			return distribution(generator);
		}
		void Erosion(WaterDrop& water);//用于迭代的函数,正常用cacluate就行
		uint32_t getVertexIndex(glm::vec2 pos);
		bool isOutOfTerrain(const glm::vec2& position) const;//是否出地图了
		void changeHeightAround(const glm::vec2& position, float heightChange);
		float sampleHeight(const glm::vec2& position);//四点取样获取新的高度
		void threadRunErosion();//线程跑腐蚀


	};
}
