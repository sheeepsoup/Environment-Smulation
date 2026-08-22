#include"lve_terrain.h"

namespace lve {
	void LveTerrain::initNoise(int seed) {
		//决定大区域是平原还是山地
		noise.biomeNoise.SetSeed(seed);
		noise.biomeNoise.SetNoiseType(
			FastNoiseLite::NoiseType_OpenSimplex2S);
		noise.biomeNoise.SetFrequency(0.002f);
		noise.biomeNoise.SetFractalType(
			FastNoiseLite::FractalType_FBm);
		noise.biomeNoise.SetFractalOctaves(2);
		noise.biomeNoise.SetFractalLacunarity(2.0f);
		noise.biomeNoise.SetFractalGain(0.5f);

		//丘陵主体
		noise.terrainNoise.SetSeed(seed + 1);
		noise.terrainNoise.SetNoiseType(
			FastNoiseLite::NoiseType_OpenSimplex2S);
		noise.terrainNoise.SetFrequency(0.012f);
		noise.terrainNoise.SetFractalType(
			FastNoiseLite::FractalType_FBm);
		noise.terrainNoise.SetFractalOctaves(5);
		noise.terrainNoise.SetFractalLacunarity(2.0f);
		noise.terrainNoise.SetFractalGain(0.5f);

		//山脊
		noise.mountainNoise.SetSeed(seed + 2);
		noise.mountainNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
		noise.mountainNoise.SetFrequency(0.006f);
		noise.mountainNoise.SetFractalType(FastNoiseLite::FractalType_Ridged);
		noise.mountainNoise.SetFractalOctaves(5);
		noise.mountainNoise.SetFractalLacunarity(2.0f);
		noise.mountainNoise.SetFractalGain(0.5f);

		//小尺度细节
		noise.detailNoise.SetSeed(seed + 3);
		noise.detailNoise.SetNoiseType(
			FastNoiseLite::NoiseType_OpenSimplex2S);
		noise.detailNoise.SetFrequency(0.06f);
		noise.detailNoise.SetFractalType(
			FastNoiseLite::FractalType_FBm);
		noise.detailNoise.SetFractalOctaves(2);
		noise.detailNoise.SetFractalGain(0.5f);

		//扭曲采样坐标
		noise.warpNoise.SetSeed(seed + 4);
		noise.warpNoise.SetDomainWarpType(
			FastNoiseLite::DomainWarpType_OpenSimplex2);
		noise.warpNoise.SetFrequency(0.003f);
		noise.warpNoise.SetDomainWarpAmp(25.0f);
		noise.warpNoise.SetFractalType(
			FastNoiseLite::FractalType_DomainWarpProgressive);
		noise.warpNoise.SetFractalOctaves(3);
		noise.warpNoise.SetFractalLacunarity(2.0f);
		noise.warpNoise.SetFractalGain(0.5f);

		//地域分布
		noise.matrialNoise.SetSeed(seed + 5);
		noise.matrialNoise.SetNoiseType(
			FastNoiseLite::NoiseType_OpenSimplex2S);
		noise.matrialNoise.SetFrequency(0.01f);
		noise.matrialNoise.SetFractalType(
			FastNoiseLite::FractalType_FBm);
		noise.matrialNoise.SetFractalOctaves(3);
		noise.matrialNoise.SetFractalLacunarity(2.0f);
		noise.matrialNoise.SetFractalGain(0.5f);

		//雪地流线
		noise.snowStream.SetSeed(seed + 6);
		noise.snowStream.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
		noise.snowStream.SetFrequency(0.01f);
		noise.snowStream.SetFractalType(FastNoiseLite::FractalType_Ridged);
		noise.snowStream.SetFractalOctaves(2);
		noise.snowStream.SetFractalLacunarity(1.0f);
		noise.snowStream.SetFractalGain(0.3f);
		noise.snowStream.SetFractalWeightedStrength(2.0f);

		//基础山体
		noise.mountainBaseNoise.SetSeed(seed + 2);
		noise.mountainBaseNoise.SetNoiseType(
			FastNoiseLite::NoiseType_OpenSimplex2S);

		noise.mountainBaseNoise.SetFrequency(0.004f);

		noise.mountainBaseNoise.SetFractalType(
			FastNoiseLite::FractalType_FBm);

		noise.mountainBaseNoise.SetFractalOctaves(4);
		noise.mountainBaseNoise.SetFractalLacunarity(2.0f);
		noise.mountainBaseNoise.SetFractalGain(0.45f);


	}
	float LveTerrain::smoothstep(float edge0, float edge1, float value) {
		float t = glm::clamp(
			(value - edge0) / (edge1 - edge0),
			0.0f,
			1.0f);
		return t * t * (3.0f - 2.0f * t);
	}
	glm::vec4 LveTerrain::getGroundWeight(float height, glm::vec3 normal, glm::vec2 WorldPos) {
		float noiseValue = noise.matrialNoise.GetNoise(WorldPos.x, WorldPos.y) * 0.5f + 0.5f;
		//草地权重,跟高度关系最大
		float grassNormalWeight = 1 - smoothstep(0.0f, 0.35f, glm::dot(WorldUp, normal));//草原法线权重
		float grassHeightWeight = 1 - smoothstep(0.0f, 5.0f, height);//草原高度权重
		float matriaNoise = noise.matrialNoise.GetNoise(WorldPos.x, WorldPos.y);//地域噪声
		float grassWeight = glm::mix(grassHeightWeight, grassNormalWeight, 0.5f) * noiseValue;//草原权重

		//泥土权重
		float dirtNormalWeight = smoothstep(0.0f, 0.35f, glm::dot(WorldUp, normal));//泥土法线权重
		float dirtHeightWeight = smoothstep(3.0f, 6.0f, height);//泥土高度权重
		if (dirtHeightWeight >= 0.99f)dirtHeightWeight = 0.0f;//防止太高导致成1
		float dirtWeight = glm::mix(dirtHeightWeight, dirtNormalWeight, 0.5f) * noiseValue;//泥土权重
		float highAltitudeDirtFade = 1.0f - smoothstep(6.5f, 9.0f, height);
		dirtWeight *= highAltitudeDirtFade;//去掉泥土环
		//岩石权重
		float rockNormalWeight = smoothstep(0.0f, 0.85f, glm::dot(WorldUp, normal));//岩石法线权重
		float rockHeightWeight = smoothstep(3.0f, 9.0f, height);//岩石高度权重
		if (rockHeightWeight >= 0.99f)rockHeightWeight = 0.0f;//防止太高导致成1
		float rockWeight = glm::mix(rockHeightWeight, rockNormalWeight, 0.5f) * noiseValue;//岩石权重

		//雪地权重
		float snowNormalWeight = smoothstep(0.0f, 0.35f, glm::dot(WorldUp, normal));//雪地法线权重
		float snowHeightWeight = smoothstep(9.0f, 12.0f, height);//雪地高度权重
		float snowWeight = snowHeightWeight;//雪地权重

		return glm::vec4(grassWeight, dirtWeight, rockWeight, snowWeight);

	}
	void LveTerrain::processArea(int seed) {
		//清理顶点
		vertices.clear();
		indices.clear();
		
		initNoise(seed);//初始化噪声
		const int chunkCount = 2 * BlockNum - 1;
		const int mapVertexCount = chunkCount * (BlockVertexNum - 1) + 1;
		this->mapVertexCount = mapVertexCount;
		const float step = BlockDistance / (BlockVertexNum - 1);
		const float mapOrigin = (-BlockNum + 1) * BlockDistance;

		//分配顶点
		heightData.resize(mapVertexCount * mapVertexCount);
		vertices.resize(static_cast<size_t>(mapVertexCount) * mapVertexCount);

		//多线程生成区块
		std::atomic<int> nextRow{ 0 };
		uint32_t threadCount = std::thread::hardware_concurrency();
		if (threadCount == 0)threadCount = 4;//失败后为4
		threadCount = glm::min(threadCount,static_cast<uint32_t>(mapVertexCount));//避免创建空闲的线程
		std::vector<std::thread> workers;
		workers.reserve(threadCount);
		for (unsigned int threadIndex = 0;threadIndex < threadCount;threadIndex++) {
			workers.emplace_back([this,&nextRow,mapVertexCount,mapOrigin,step]() {
					while (true) {
						int globalY =nextRow.fetch_add(1,std::memory_order_relaxed);
						if (globalY >= mapVertexCount)break;
						const float worldY =mapOrigin + globalY * step;
						for (int globalX = 0;globalX < mapVertexCount;globalX++) {
							const float worldX =mapOrigin + globalX * step;
							const size_t vertexIndex =static_cast<size_t>(globalY) *mapVertexCount +globalX;
							const float height = getHeight(worldX, worldY, true);
							heightData[vertexIndex] = height;
							vertices[vertexIndex] = {glm::vec3(worldX,worldY,height),glm::vec3(1.0f),WorldUp,0.0f};
						}
					}
				});
		}
		//等待线程完毕
		for (auto& worker : workers)worker.join();
		indices.resize((mapVertexCount - 1) * (mapVertexCount - 1) * 6);
		//生成区块[0,0->初始地区]
		for (int x = -BlockNum + 1; x < BlockNum; x++) {//遍历2 * n - 1次[例如区块为6,x方向遍历11次]
			for (int y = -BlockNum + 1; y < BlockNum; y++) {//x,y表示当前对应的区块
				/*这块是单线程生成,单纯保留纪念一下
				uint32_t baseVertex = static_cast<uint32_t>(vertices.size());
				//暂时先全部正方形生成,后面改成圆形
				float orgianX = x * BlockDistance;
				float orgianY = y * BlockDistance;
				float step = BlockDistance / (BlockVertexNum - 1);//每次的步进距离
				const int chunkX = x + BlockNum - 1;
				const int chunkY = y + BlockNum - 1;
				for (int i = 0; i < BlockVertexNum; i++) {//每个区块生成顶点
					for (int j = 0; j < BlockVertexNum; j++) {
						const float worldX = orgianX + j * step;
						const float worldY = orgianY + i * step;
						const int globalX = chunkX * (BlockVertexNum - 1) + j;
						const int globalY = chunkY * (BlockVertexNum - 1) + i;
						const uint32_t vertexIndex = static_cast<uint32_t>(globalY * mapVertexCount + globalX);
						glm::vec3 chunkColor; //当前颜色
						float height = getHeight(worldX, worldY, true);
						glm::vec3 normal = WorldUp;//下面统一算
						//	chunkColor = getMatrial(getGroundWeight(height, normal, glm::vec2(worldX, worldY)));
						chunkColor = glm::vec3(1.0f, 1.0f, 1.0f);
						heightData[vertexIndex] = height;
						vertices[vertexIndex] = {
							glm::vec3(worldX, worldY, height),
							chunkColor,
							normal
						};
					}

				}
				*/
				//填充indices
				const int chunkX = x + BlockNum - 1;
				const int chunkY = y + BlockNum - 1;
				for (int i = 0; i < BlockVertexNum - 1; i++) {
					for (int j = 0; j < BlockVertexNum - 1; j++) {
						const int globalX = chunkX * (BlockVertexNum - 1) + j;
						const int globalY = chunkY * (BlockVertexNum - 1) + i;
						const uint32_t nowPoint = static_cast<uint32_t>(globalY * mapVertexCount + globalX);
						const size_t offset =(static_cast<size_t>(globalY) * (mapVertexCount - 1) +static_cast<size_t>(globalX)) * 6;
						indices[offset] = nowPoint;
						indices[offset + 1] = nowPoint + 1;
						indices[offset + 2] = nowPoint + mapVertexCount;
						indices[offset + 3] = nowPoint + 1;
						indices[offset + 4] = nowPoint + 1 + mapVertexCount;
						indices[offset + 5] = nowPoint + mapVertexCount;
					}
				}
			}
		}
		//锈蚀模拟
		//threadRunErosion();


	}
	void LveTerrain::calculateNormal() {

		//多线程计算法线
		std::atomic<int> nextRow{ 1 };
		unsigned int threadCount = std::thread::hardware_concurrency();
		if (threadCount == 0)threadCount = 4;
		std::vector<std::thread> workers;
		workers.reserve(threadCount);

		for (unsigned int threadIndex = 0;
			threadIndex < threadCount;
			threadIndex++) {

			workers.emplace_back([this, &nextRow]() {
				while (true) {
					const int chunkCount = 2 * BlockNum - 1;
					const int mapVertexCountw = chunkCount * (BlockVertexNum - 1) + 1;
					const int gridY =nextRow.fetch_add(1,std::memory_order_relaxed);

					if (gridY >= mapVertexCountw - 1) {
						break;
					}

					for (int gridX = 1;
						gridX < mapVertexCountw - 1;
						gridX++) {

						const glm::vec2 gridPosition{static_cast<float>(gridX),static_cast<float>(gridY)};

						const uint32_t index = getVertexIndex(gridPosition);

						vertices[index].normal =calculateNormalNew(gridPosition);
					}
				}
				});
		}

		for (auto& worker : workers) {
			worker.join();
		}
		/*依旧单线程纪念
		for (int gridX = 1;gridX < mapVertexCount - 1;gridX++) {
			for (int gridY = 1;gridY < mapVertexCount - 1;gridY++) {

				glm::vec2 gridPosition{ static_cast<float>(gridX),static_cast<float>(gridY) };

				uint32_t index = getVertexIndex(gridPosition);

				vertices.at(index).normal = calculateNormalNew(gridPosition);
			}
		}
		*/
	}
	float LveTerrain::getHeight(float WorldX, float WorldY, bool isFirst) {
		const float biomeValue = noise.biomeNoise.GetNoise(WorldX, WorldY);
		const float biome01 = biomeValue * 0.5f + 0.5f;
		float MontainMask = smoothstep(0.58f, 0.72f, biome01);

		float warpedX = WorldX;
		float warpedY = WorldY;
		noise.warpNoise.DomainWarp(warpedX, warpedY);

		const float terrainValue = noise.terrainNoise.GetNoise(warpedX, warpedY);

		float ridge = 1.0 - abs(terrainValue);
		// 再乘方让他变得更尖锐
		ridge = ridge * ridge;
		float terrain01 = terrainValue * 0.5f + 0.5f;
		float hillShape = smoothstep(0.30f, 0.72f, terrain01);
		float hillHeight = hillShape * 3.5f;

		const float mountainValue = noise.mountainNoise.GetNoise(warpedX, warpedY);
		const float mountain01 = mountainValue * 0.5f + 0.5f;
		const float mountainShape = mountain01 * mountain01;
		const float mountainHeight = mountainShape * 20.0f;

		const float detailValue = noise.detailNoise.GetNoise(warpedX, warpedY);
		const float detailStrength = glm::mix(0.15f, 0.60f, MontainMask);

		const float finalHeight = glm::mix(hillHeight, mountainHeight, MontainMask);
		return finalHeight + detailValue * detailStrength;
	}
	glm::vec3 LveTerrain::calculateNormal(float worldX, float worldY, float sampleDistance) {
		const float heightLeft = getHeight(worldX - sampleDistance, worldY, true);
		const float heightRight = getHeight(worldX + sampleDistance, worldY, true);
		const float heightDown = getHeight(worldX, worldY - sampleDistance, true);
		const float heightUp = getHeight(worldX, worldY + sampleDistance, true);
		const glm::vec3 tangentX{ sampleDistance * 2.0f,0.0f,heightRight - heightLeft };//切线
		const glm::vec3 tangentY{ 0.0f,sampleDistance * 2.0f,heightUp - heightDown };
		return glm::normalize(glm::cross(tangentX, tangentY));
	}
	glm::vec3 LveTerrain::calculateNormalNew(
		const glm::vec2& gridPosition)
	{
		const glm::vec3 left =
			vertices.at(getVertexIndex(
				gridPosition + glm::vec2(-1.0f, 0.0f))).pos;

		const glm::vec3 right =
			vertices.at(getVertexIndex(
				gridPosition + glm::vec2(1.0f, 0.0f))).pos;

		const glm::vec3 up =
			vertices.at(getVertexIndex(
				gridPosition + glm::vec2(0.0f, -1.0f))).pos;

		const glm::vec3 down =
			vertices.at(getVertexIndex(
				gridPosition + glm::vec2(0.0f, 1.0f))).pos;

		glm::vec3 tangentX = right - left;
		glm::vec3 tangentY = down - up;

		glm::vec3 result =
			glm::cross(tangentX, tangentY);

		float normalLength = glm::length(result);

		if (!std::isfinite(normalLength) ||
			normalLength < 0.00001f) {
			return WorldUp;
		}

		return result / normalLength;
	}
	glm::vec3 LveTerrain::getMatrial(glm::vec4 weight) {
		const glm::vec3 grassColor{ 0.18f, 0.38f, 0.14f };

		const glm::vec3 dirtColor{ 0.34f, 0.24f, 0.14f };

		const glm::vec3 rockColor{ 0.38f, 0.39f, 0.37f };

		const glm::vec3 snowColor{ 0.88f, 0.92f, 0.95f };

		if (weight.r >= weight.g &&
			weight.r >= weight.b &&
			weight.r >= weight.a) {
			return grassColor;
		}

		if (weight.g >= weight.r &&
			weight.g >= weight.b &&
			weight.g >= weight.a) {
			return dirtColor;
		}

		if (weight.b >= weight.r &&
			weight.b >= weight.g &&
			weight.b >= weight.a) {
			return rockColor;
		}

		return snowColor;
	}
	void LveTerrain::cacluateErosion() {
		const int chunkCount = 2 * BlockNum - 1;
		const int mapVertexCount = chunkCount * (BlockVertexNum - 1) + 1;
		erosion.resize(EROSON_EXTENT);//预设大小
		for (int i = 0; i < EROSON_EXTENT; i++) {
			int x = randomInt(1, mapVertexCount - 3);
			int y = randomInt(1, mapVertexCount - 3);
			erosion[i].position = glm::vec2(x, y);
			erosion[i].speed = 1;//初始速度
			erosion[i].sediment = 0.0f;//初始没泥沙
			erosion[i].water = 1.0f;//初始水量
			erosion[i].step = 0;
			erosion[i].direction = glm::vec2(0.0f);
			Erosion(erosion[i]);
		}
	}
	void LveTerrain::Erosion(WaterDrop& water) {
		if (water.step >= WATER_MAX_STEP) {
			changeHeightAround(water.position, water.sediment);//结束生命沉积
			return;
		}// 生命周期结束
		if (water.water < MIN_WATER) {
			changeHeightAround(water.position, water.sediment);//结束生命沉积
			return;
		}// 水基本蒸发完
		if (water.speed < MIN_SPEED) {
			changeHeightAround(water.position, water.sediment);
			return;
		}// 几乎不再移动

		water.water *= 0.99;//蒸发
		water.step++;//增加步数
		glm::vec2 oldPosition = water.position;
		//计算方向
	//	float nowH =  vertices[getVertexIndex(water.position)].pos.z;
	//	float leftH = vertices[getVertexIndex(water.position + glm::vec2(-1,0))].pos.z;
	//	float rightH = vertices[getVertexIndex(water.position + glm::vec2(1, 0))].pos.z;
	//	float upH = vertices[getVertexIndex(water.position + glm::vec2(0, -1))].pos.z;
	//	float downH = vertices[getVertexIndex(water.position + glm::vec2(0, 1))].pos.z;
		if (isOutOfTerrain(water.position)) {
			return;
		}
		float nowH = sampleHeight(water.position);
		float leftH = sampleHeight(water.position + glm::vec2(-1, 0));//water.direction = -glm::normalize(glm::vec2(
		float rightH = sampleHeight(water.position + glm::vec2(1, 0));//		 rightH - leftH,
		float upH = sampleHeight(water.position + glm::vec2(0, -1));//		 downH - upH
		float downH = sampleHeight(water.position + glm::vec2(0, 1));//	));


		glm::vec2 gradient{ rightH - leftH,downH - upH };

		glm::vec2 newDirection =
			water.direction * inertia -
			gradient * (1.0f - inertia);

		float directionLength =
			glm::length(newDirection);
		if (!std::isfinite(directionLength) || directionLength < 0.00001f)return;
		water.direction = newDirection / directionLength;
		//出地图了
		if (isOutOfTerrain(water.position + water.direction)) {
			return;
		}
		if (glm::length(glm::vec2(rightH - leftH, upH - downH)) < 0.00001f) {
			return;//防止停滞
		}

		water.position += water.direction;

		float oldHeight = nowH;
		float newHeight = sampleHeight(water.position);
		float deltaHeight = newHeight - oldHeight;
		float slope = std::max(-deltaHeight, 0.0f);//坡度


		water.maxSediment = std::max(slope, 0.01f) * water.speed * water.water * capacityFactor;
		water.speed = std::sqrt(glm::max(0.0f, water.speed * water.speed - deltaHeight * gravity));

		if (deltaHeight > 0) {
			//水滴上坡,减速后退出就行了
			float depositedAmount = std::min(deltaHeight, water.sediment);
			depositedAmount = glm::clamp(depositedAmount, 0.0f, 0.05f);
			changeHeightAround(oldPosition, depositedAmount);
			water.sediment -= depositedAmount;
			Erosion(water);
			return;
		}
		if (water.sediment < water.maxSediment) {
			//腐蚀
			float erodedAmount = (water.maxSediment - water.sediment) * erosionRate;
			erodedAmount = std::min(erodedAmount, std::max(-deltaHeight, 0.0f));
			erodedAmount = glm::clamp(erodedAmount, 0.0f, 0.05f);
			water.sediment += erodedAmount;//带泥
			changeHeightAround(
				oldPosition,
				-erodedAmount);
			Erosion(water);
		}
		else
		{
			//沉积
			float depositedAmount = (water.sediment - water.maxSediment) * depositionRate;
			depositedAmount = std::min(depositedAmount, water.sediment);
			depositedAmount = glm::clamp(depositedAmount, 0.0f, 0.05f);
			changeHeightAround(oldPosition, depositedAmount);
			water.sediment -= depositedAmount;
			Erosion(water);
		}



	}
	void LveTerrain::changeHeightAround(const glm::vec2& position, float heightChange)
	{
		// position 必须保证四周仍在地图内
		if (isOutOfTerrain(position))return;

		const float floorX = std::floor(position.x);

		const float floorY = std::floor(position.y);

		// 当前点在一个网格内部的局部位置，范围为 0~1
		const float offsetX = position.x - floorX;

		const float offsetY = position.y - floorY;

		// 网格的四个顶点
		const glm::vec2 bottomLeft{ floorX,	floorY };

		const glm::vec2 bottomRight{ floorX + 1.0f,floorY };

		const glm::vec2 topLeft{ floorX,floorY + 1.0f };

		const glm::vec2 topRight{ floorX + 1.0f,floorY + 1.0f };

		// 双线性权重
		const float bottomLeftWeight = (1.0f - offsetX) * (1.0f - offsetY);

		const float bottomRightWeight = offsetX * (1.0f - offsetY);

		const float topLeftWeight = (1.0f - offsetX) * offsetY;

		const float topRightWeight = offsetX * offsetY;

		// 将总高度变化按权重分给四个顶点
		vertices.at(getVertexIndex(bottomLeft)).pos.z += heightChange * bottomLeftWeight;

		vertices.at(getVertexIndex(bottomRight)).pos.z += heightChange * bottomRightWeight;

		vertices.at(getVertexIndex(topLeft)).pos.z += heightChange * topLeftWeight;

		vertices.at(getVertexIndex(topRight)).pos.z += heightChange * topRightWeight;
	}
	uint32_t LveTerrain::getVertexIndex(glm::vec2 pos) {
		const int chunkCount = 2 * BlockNum - 1;
		const int mapVertexCount = chunkCount * (BlockVertexNum - 1) + 1;

		const int gridX = static_cast<int>(std::floor(pos.x));
		const int gridY = static_cast<int>(std::floor(pos.y));

		if (gridX < 0 || gridY < 0 || gridX >= mapVertexCount || gridY >= mapVertexCount) {
			throw std::out_of_range("terrain vertex index out of range");
		}

		return static_cast<uint32_t>(gridY * mapVertexCount + gridX);
	}
	bool LveTerrain::isOutOfTerrain(const glm::vec2& position) const {
		const int chunkCount = 2 * BlockNum - 1;
		const int mapVertexCount = chunkCount * (BlockVertexNum - 1) + 1;

		return position.x < 1.0f ||
			position.y < 1.0f ||
			position.x >= static_cast<float>(mapVertexCount - 2) ||
			position.y >= static_cast<float>(mapVertexCount - 2);
	}
	float LveTerrain::sampleHeight(const glm::vec2& position) {
		const float floorX = std::floor(position.x);
		const float floorY = std::floor(position.y);
		const float offsetX = position.x - floorX;
		const float offsetY = position.y - floorY;

		const float bottomLeftHeight = vertices.at(getVertexIndex(glm::vec2(floorX, floorY))).pos.z;
		const float bottomRightHeight = vertices.at(getVertexIndex(glm::vec2(floorX + 1.0f, floorY))).pos.z;
		const float topLeftHeight = vertices.at(getVertexIndex(glm::vec2(floorX, floorY + 1.0f))).pos.z;
		const float topRightHeight = vertices.at(getVertexIndex(glm::vec2(floorX + 1.0f, floorY + 1.0f))).pos.z;

		const float bottomHeight = glm::mix(bottomLeftHeight, bottomRightHeight, offsetX);
		const float topHeight = glm::mix(topLeftHeight, topRightHeight, offsetX);

		return glm::mix(bottomHeight, topHeight, offsetY);
	}
	void LveTerrain::updateHeightFlow(std::vector<int32_t>& heightUint, std::vector<uint32_t>& flowUint, float SCALE) {
		//更新流量
		const uint32_t maxFlow =
		flowUint.empty() ? 0 : *std::max_element(flowUint.begin(), flowUint.end());
		const float maxValue =std::log1p(static_cast<float>(maxFlow));
		for (size_t i = 0;i < flowUint.size();i++) {

			const float flow =std::log1p(static_cast<float>(flowUint[i]));
			vertices[i].flow =maxValue > 0.0f? flow / maxValue: 0.0f;
		}


		//更新顶点高度
		for (int i = 0; i < vertices.size(); i++) {
			vertices[i].pos.z = heightUint[i] / SCALE;
			heightData[i] = static_cast<float>(heightUint[i]) / SCALE;
		}
	}
}