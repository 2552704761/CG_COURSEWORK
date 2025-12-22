

#include "Core.h"
#include "Window.h"
#include "Timer.h"
#include "Maths.h"
#include "Shaders.h"
#include "Mesh.h"
#include "PSO.h"
#include "GEMLoader.h"
#include "Animation.h"
#include "Texture.h"
#include "AABB.h"
#include <algorithm>

// Properties -> Linker -> System -> Windows

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")
auto randRange = [](float min, float max)
	{
		return min + (float(rand()) / float(RAND_MAX)) * (max - min);
	};
class Plane
{
public:
	Mesh mesh;
	std::string shaderName;
	std::string textureFilename = "Models/Textures/TX_Ground_Grass_05a_ALB.png";
	STATIC_VERTEX addVertex(Vec3 p, Vec3 n, float tu, float tv)
	{
		STATIC_VERTEX v;
		v.pos = p;
		v.normal = n;
		Frame frame;
		frame.fromVector(n);
		v.tangent = frame.u;
		v.tu = tu;
		v.tv = tv;
		return v;
	}
	void init(Core* core, PSOManager *psos, Shaders* shaders, TextureManager* textures)
	{
		std::vector<STATIC_VERTEX> vertices;
		vertices.push_back(addVertex(Vec3(-100, 0, -100), Vec3(0, 1, 0), 0, 0));
		vertices.push_back(addVertex(Vec3(100, 0, -100), Vec3(0, 1, 0), 1, 0));
		vertices.push_back(addVertex(Vec3(-100, 0, 100), Vec3(0, 1, 0), 0, 1));
		vertices.push_back(addVertex(Vec3(100, 0, 100), Vec3(0, 1, 0), 1, 1));
		std::vector<unsigned int> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(3);
		indices.push_back(2);
		mesh.init(core, vertices, indices);
		shaders->load(core, "GroundTextured", "VS.txt", "PSTextured.txt");
		shaderName = "GroundTextured";
		psos->createPSO(core, "GroundTexturedPSO", shaders->find("GroundTextured")->vs, shaders->find("GroundTextured")->ps, VertexLayoutCache::getStaticLayout());
		textures->loadTexture(core, textureFilename);
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix &vp, TextureManager* textures)
	{
		Matrix planeWorld;
		shaders->updateConstantVS("GroundTextured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("GroundTextured", "staticMeshBuffer", "W", &planeWorld);
		shaders->apply(core, shaderName);
		psos->bind(core, "GroundTexturedPSO");
		int heapIndex = textures->find(textureFilename);
		shaders->updateTexturePS(core, "GroundTextured", "tex", heapIndex);
		mesh.draw(core);
	}
};

class StaticModel
{
public:
	std::vector<Mesh *> meshes;
	std::vector<std::string> textureFilenames;
	std::vector<std::string> normalFilenames;
	bool isGrass = false;
	void load(Core* core, std::string filename, Shaders* shaders, PSOManager* psos, TextureManager* textures ,bool ISG)
	{
		isGrass = ISG;
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		loader.load(filename, gemmeshes);
		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh* mesh = new Mesh();
			std::vector<STATIC_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesStatic.size(); j++)
			{
				STATIC_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesStatic[j], sizeof(STATIC_VERTEX));
				vertices.push_back(v);
			}
			textureFilenames.push_back(gemmeshes[i].material.find("albedo").getValue());
			textures->loadTexture(core, gemmeshes[i].material.find("albedo").getValue());
			normalFilenames.push_back(gemmeshes[i].material.find("nh").getValue());
			textures->loadTexture(core, gemmeshes[i].material.find("nh").getValue());
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);
		}
		//shaders->load(core, "StaticModel", "VS.txt", "PSTextured.txt");
		//psos->createPSO(core, "StaticModelPSO", shaders->find("StaticModel")->vs, shaders->find("StaticModel")->ps, VertexLayoutCache::getStaticLayout());
		shaders->load(core, "GrassAlpha", "VS_grass.txt", "PS_AlphaTest.txt");
		psos->createPSO(core, "GrassAlphaPSO", shaders->find("GrassAlpha")->vs, shaders->find("GrassAlpha")->ps, VertexLayoutCache::getStaticInstancedLayout());
		shaders->load(core, "StaticNormal", "VS_Instanced.txt", "PSNormalMap.txt");
		psos->createPSO(core, "StaticNormalPSO", shaders->find("StaticNormal")->vs, shaders->find("StaticNormal")->ps, VertexLayoutCache::getStaticInstancedLayout());
	}
	void updateWorld(Shaders* shaders, Matrix& w)
	{
		shaders->updateConstantVS("StaticModel", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("GrassAlpha", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("StaticNormal", "staticMeshBuffer", "W", &w);
	}
	void drawInstanced(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textures, Matrix& vp, const std::vector<InstanceData>& instances, const std::string& psoName = "StaticNormalPSO", float t = 0)
	{
		if (instances.empty())
			return;
		if (psoName == "GrassAlphaPSO")
		{
			shaders->updateConstantVS("GrassAlpha", "staticMeshBuffer", "VP", &vp);
			shaders->updateConstantVS("GrassAlpha", "staticMeshBuffer", "time", &t);
			shaders->apply(core, "GrassAlpha");
			psos->bind(core, psoName);
			for (int i = 0; i < meshes.size(); i++)
			{
				int heapIndex = textures->find(textureFilenames[i]);
				shaders->updateTexturePS(core, "GrassAlpha", "tex", heapIndex);
				//char buffer[256];
				//sprintf_s(buffer, "drawInstancedGrass textureFilenames = %s heapIndex = %d\n", textureFilenames[i].c_str(), heapIndex);
				//OutputDebugStringA(buffer);
				meshes[i]->drawInstanced(core);
			}
		}
		else
		{
			shaders->updateConstantVS("StaticNormal", "staticMeshBuffer", "VP", &vp);
			shaders->apply(core, "StaticNormal");
			psos->bind(core, psoName);
			for (int i = 0; i < meshes.size(); i++)
			{
				int heapIndex = textures->find(textureFilenames[i]);
				shaders->updateTexturePS(core, "StaticNormal", "albedoTex", heapIndex);
				heapIndex = textures->find(normalFilenames[i]);
				shaders->updateTexturePS(core, "StaticNormal", "normalTex", heapIndex);
				//char buffer[256];
				//sprintf_s(buffer, "drawInstancedStaticModel textureFilenames = %s heapIndex = %d\n", textureFilenames[i].c_str(), heapIndex);
				//OutputDebugStringA(buffer);
				meshes[i]->drawInstanced(core);
			}
		}
	}
	AABB getLocalAABB() const
	{
		AABB aabb;
		aabb.min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		aabb.max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (Mesh* m : meshes)
		{
			aabb.min.x = std::min(aabb.min.x, m->localAABB.min.x);
			aabb.min.y = std::min(aabb.min.y, m->localAABB.min.y);
			aabb.min.z = std::min(aabb.min.z, m->localAABB.min.z);

			aabb.max.x = std::max(aabb.max.x, m->localAABB.max.x);
			aabb.max.y = std::max(aabb.max.y, m->localAABB.max.y);
			aabb.max.z = std::max(aabb.max.z, m->localAABB.max.z);
		}
		return aabb;
	}

	void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textures, Matrix &vp, const std::string& psoName = "StaticNormalPSO", float t = 0)
	{
		if (psoName == "GrassAlphaPSO")
		{
			shaders->updateConstantVS("GrassAlpha", "staticMeshBuffer", "VP", &vp);
			shaders->updateConstantVS("GrassAlpha", "staticMeshBuffer", "time", &t);
			shaders->apply(core, "GrassAlpha");
			psos->bind(core, psoName); 
			for (int i = 0; i < meshes.size(); i++)
			{
				int heapIndex = textures->find(textureFilenames[i]);
				shaders->updateTexturePS(core, "GrassAlpha", "tex", heapIndex);
				//char buffer[256];
				//sprintf_s(buffer, "drawGrass textureFilenames = %s heapIndex = %d\n", textureFilenames[i].c_str(), heapIndex);
				//OutputDebugStringA(buffer);
				meshes[i]->draw(core);
			}
		}
		else
		{
			shaders->updateConstantVS("StaticNormal", "staticMeshBuffer", "VP", &vp);
			shaders->apply(core, "StaticNormal");
			psos->bind(core, psoName);
			for (int i = 0; i < meshes.size(); i++)
			{
				int heapIndex = textures->find(textureFilenames[i]);
				shaders->updateTexturePS(core, "StaticNormal", "albedoTex", heapIndex);
				heapIndex = textures->find(normalFilenames[i]);
				shaders->updateTexturePS(core, "StaticNormal", "normalTex", heapIndex);
				//char buffer[256];
				//sprintf_s(buffer, "drawStaticModel textureFilenames = %s heapIndex = %d\n", textureFilenames[i].c_str(), heapIndex);
				//OutputDebugStringA(buffer);
				meshes[i]->draw(core);
			}
		}
	}
};

class AnimatedModel
{
public:
	std::vector<Mesh *> meshes;
	Animation animation;
	std::vector<std::string> textureFilenames;
	void load(Core* core, std::string filename, PSOManager* psos, Shaders* shaders, TextureManager* textures)
	{
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		GEMLoader::GEMAnimation gemanimation;
		loader.load(filename, gemmeshes, gemanimation);
		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh *mesh = new Mesh();
			std::vector<ANIMATED_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesAnimated.size(); j++)
			{
				ANIMATED_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesAnimated[j], sizeof(ANIMATED_VERTEX));
				vertices.push_back(v);
			}
			textureFilenames.push_back(gemmeshes[i].material.find("albedo").getValue());
			textures->loadTexture(core, gemmeshes[i].material.find("albedo").getValue());
				// Load texture with filename: gemmeshes[i].material.find("albedo").getValue()
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh); 
		}
		shaders->load(core, "AnimatedModel", "VSAnim.txt", "PSTextured.txt");
		psos->createPSO(core, "AnimatedModelPSO", shaders->find("AnimatedModel")->vs, shaders->find("AnimatedModel")->ps, VertexLayoutCache::getAnimatedLayout());
		memcpy(&animation.skeleton.globalInverse, &gemanimation.globalInverse, 16 * sizeof(float));
		for (int i = 0; i < gemanimation.bones.size(); i++)
		{
			Bone bone;
			bone.name = gemanimation.bones[i].name;
			memcpy(&bone.offset, &gemanimation.bones[i].offset, 16 * sizeof(float));
			bone.parentIndex = gemanimation.bones[i].parentIndex;
			animation.skeleton.bones.push_back(bone);
		}
		for (int i = 0; i < gemanimation.animations.size(); i++)
		{
			std::string name = gemanimation.animations[i].name;
			AnimationSequence aseq;
			aseq.ticksPerSecond = gemanimation.animations[i].ticksPerSecond;
			for (int j = 0; j < gemanimation.animations[i].frames.size(); j++)
			{
				AnimationFrame frame;
				for (int index = 0; index < gemanimation.animations[i].frames[j].positions.size(); index++)
				{
					Vec3 p;
					Quaternion q;
					Vec3 s;
					memcpy(&p, &gemanimation.animations[i].frames[j].positions[index], sizeof(Vec3));
					frame.positions.push_back(p);
					memcpy(&q, &gemanimation.animations[i].frames[j].rotations[index], sizeof(Quaternion));
					frame.rotations.push_back(q);
					memcpy(&s, &gemanimation.animations[i].frames[j].scales[index], sizeof(Vec3));
					frame.scales.push_back(s);
				}
				aseq.frames.push_back(frame);
			}
			animation.animations.insert({ name, aseq });
		}
	}
	void updateWorld(Shaders* shaders, Matrix& w)
	{
		shaders->updateConstantVS("AnimatedModel", "staticMeshBuffer", "W", &w);
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, TextureManager* textures, AnimationInstance* instance, Matrix& vp, Matrix& w)
	{
		psos->bind(core, "AnimatedModelPSO");
		shaders->updateConstantVS("AnimatedModel", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("AnimatedModel", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("AnimatedModel", "staticMeshBuffer", "bones", instance->matrices);
		shaders->apply(core, "AnimatedModel");
		for (int i = 0; i < meshes.size(); i++)
		{
			shaders->updateTexturePS(core, "AnimatedModel", "tex", textures->find(textureFilenames[i]));
			//char buffer[256];
			//sprintf_s(buffer, "drawAnimatedModel textureFilenames = %s heapIndex = %d\n", textureFilenames[i].c_str(), textures->find(textureFilenames[i]));
			//OutputDebugStringA(buffer);
			meshes[i]->draw(core);
		}
	}
	AABB getLocalAABB() const
	{
		AABB aabb;
		aabb.min = Vec3(FLT_MAX, FLT_MAX, FLT_MAX);
		aabb.max = Vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		for (Mesh* m : meshes)
		{
			aabb.min.x = std::min(aabb.min.x, m->localAABB.min.x);
			aabb.min.y = std::min(aabb.min.y, m->localAABB.min.y);
			aabb.min.z = std::min(aabb.min.z, m->localAABB.min.z);

			aabb.max.x = std::max(aabb.max.x, m->localAABB.max.x);
			aabb.max.y = std::max(aabb.max.y, m->localAABB.max.y);
			aabb.max.z = std::max(aabb.max.z, m->localAABB.max.z);
		}
		return aabb;
	}
};

class PlayerModel
{
public:
	Vec3 offset = Vec3(0.5f, 0.5f, 1.2f);
	float scale = 0.15f;
	std::vector<Mesh*> meshes;
	Animation animation;
	std::vector<std::string> textureFilenames;
	void load(Core* core, std::string filename, PSOManager* psos, Shaders* shaders, TextureManager* textures)
	{
		GEMLoader::GEMModelLoader loader;
		std::vector<GEMLoader::GEMMesh> gemmeshes;
		GEMLoader::GEMAnimation gemanimation;
		loader.load(filename, gemmeshes, gemanimation);
		for (int i = 0; i < gemmeshes.size(); i++)
		{
			Mesh* mesh = new Mesh();
			std::vector<ANIMATED_VERTEX> vertices;
			for (int j = 0; j < gemmeshes[i].verticesAnimated.size(); j++)
			{
				ANIMATED_VERTEX v;
				memcpy(&v, &gemmeshes[i].verticesAnimated[j], sizeof(ANIMATED_VERTEX));
				vertices.push_back(v);
			}
			textureFilenames.push_back(gemmeshes[i].material.find("albedo").getValue());
			textures->loadTexture(core, gemmeshes[i].material.find("albedo").getValue());
			// Load texture with filename: gemmeshes[i].material.find("albedo").getValue()
			mesh->init(core, vertices, gemmeshes[i].indices);
			meshes.push_back(mesh);
		}
		shaders->load(core, "PlayerModel", "VSAnim.txt", "PSTextured.txt");
		psos->createPlayerPSO(core, "PlayerModelPSO", shaders->find("PlayerModel")->vs, shaders->find("PlayerModel")->ps, VertexLayoutCache::getAnimatedLayout());
		memcpy(&animation.skeleton.globalInverse, &gemanimation.globalInverse, 16 * sizeof(float));
		for (int i = 0; i < gemanimation.bones.size(); i++)
		{
			Bone bone;
			bone.name = gemanimation.bones[i].name;
			memcpy(&bone.offset, &gemanimation.bones[i].offset, 16 * sizeof(float));
			bone.parentIndex = gemanimation.bones[i].parentIndex;
			animation.skeleton.bones.push_back(bone);
		}
		for (int i = 0; i < gemanimation.animations.size(); i++)
		{
			std::string name = gemanimation.animations[i].name;
			AnimationSequence aseq;
			aseq.ticksPerSecond = gemanimation.animations[i].ticksPerSecond;
			for (int j = 0; j < gemanimation.animations[i].frames.size(); j++)
			{
				AnimationFrame frame;
				for (int index = 0; index < gemanimation.animations[i].frames[j].positions.size(); index++)
				{
					Vec3 p;
					Quaternion q;
					Vec3 s;
					memcpy(&p, &gemanimation.animations[i].frames[j].positions[index], sizeof(Vec3));
					frame.positions.push_back(p);
					memcpy(&q, &gemanimation.animations[i].frames[j].rotations[index], sizeof(Quaternion));
					frame.rotations.push_back(q);
					memcpy(&s, &gemanimation.animations[i].frames[j].scales[index], sizeof(Vec3));
					frame.scales.push_back(s);
				}
				aseq.frames.push_back(frame);
			}
			animation.animations.insert({ name, aseq });
		}
	}
	void draw(Core* core,Shaders* shaders,PSOManager* psos,TextureManager* textures, AnimationInstance* instance,const Matrix& proj)
	{
		Matrix world = Matrix::scaling(Vec3(scale, scale, scale)) *  Matrix::rotateY(PI) * Matrix::translation(offset);
		Matrix vp = Matrix() * proj;
		psos->bind(core, "PlayerModelPSO");
		shaders->updateConstantVS("PlayerModel", "staticMeshBuffer", "W", &world);
		shaders->updateConstantVS("PlayerModel", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("PlayerModel", "staticMeshBuffer", "bones", instance->matrices);
		shaders->apply(core, "PlayerModel");
		for (int i = 0; i < meshes.size(); i++)
		{
			shaders->updateTexturePS(core, "PlayerModel", "tex", textures->find(textureFilenames[i]));
			//char buffer[256];
			//sprintf_s(buffer, "drawAnimatedModel textureFilenames = %s heapIndex = %d\n", textureFilenames[i].c_str(), textures->find(textureFilenames[i]));
			//OutputDebugStringA(buffer);
			meshes[i]->draw(core);
		}
	}
};

class DebugLineRenderer
{
public:
	Mesh lineMesh;
	std::vector<LineVertex> vertices;

	void clear()
	{
		vertices.clear();
	}

	void addAABB(const AABB& aabb, Vec3 color)
	{
		buildAABBLineVertices(aabb, vertices, color);
	}
	void addLine(const Vec3& a, const Vec3& b, const Vec3& color)
	{
		LineVertex v0;
		v0.pos = a;
		v0.color = color;
		LineVertex v1;
		v1.pos = b;
		v1.color = color;
		vertices.push_back(v0);
		vertices.push_back(v1);
	}


	void draw(Core* core, Shaders* shaders, PSOManager* psos, Matrix& vp)
	{
		if (vertices.empty()) return;

		lineMesh.updateDynamic(core, vertices);

		shaders->updateConstantVS("Line", "Camera", "VP", &vp);
		shaders->apply(core, "Line");
		psos->bind(core, "LinePSO");

		core->getCommandList()->IASetPrimitiveTopology(
			D3D_PRIMITIVE_TOPOLOGY_LINELIST
		);

		lineMesh.drawLines(core);
	}
};

class SkyDome
{
public:
	Mesh domeMesh;
	int texHeapOffset = -1;
	float radius = 500.0f;
	int latBands = 32;
	int lonBands = 32;
	void init(Core* core, PSOManager* psos, TextureManager* textures, Shaders* shaders)
	{
		std::vector<STATIC_VERTEX> verts;
		std::vector<unsigned int> indices;
		for (int lat = 0; lat <= latBands; lat++) {
			float theta = lat * 3.1415926f / latBands;
			float sinTheta = sinf(theta);
			float cosTheta = cosf(theta);
			for (int lon = 0; lon <= lonBands; lon++) {
				float phi = lon * 2.0f * 3.1415926f / lonBands;
				float sinPhi = sinf(phi);
				float cosPhi = cosf(phi);
				float x = cosPhi * sinTheta;
				float y = cosTheta;
				float z = sinPhi * sinTheta;
				STATIC_VERTEX v;
				v.pos = Vec3(x * radius, y * radius, z * radius);
				v.normal = Vec3(-x, -y, -z);
				v.tangent = Vec3(1, 0, 0);
				v.tu = (float)lon / lonBands;
				v.tv = (float)lat / latBands;
				verts.push_back(v);
			}
		}
		for (int lat = 0; lat < latBands; lat++) {
			for (int lon = 0; lon < lonBands; lon++) {
				int first = lat * (lonBands + 1) + lon;
				int second = first + lonBands + 1;
				indices.push_back(first);
				indices.push_back(second);
				indices.push_back(first + 1);
				indices.push_back(second);
				indices.push_back(second + 1);
				indices.push_back(first + 1);
			}
		}
		domeMesh.init(core, verts, indices);
		textures->loadTexture(core, "sky/sky1.png");
		texHeapOffset = textures->find("sky/sky1.png");
		shaders->load(core, "SkyBox", "VS_Skybox.txt", "PS_Skybox.txt");
		psos->createSkyPSO(core, "SkyBoxPSO", shaders->find("SkyBox")->vs, shaders->find("SkyBox")->ps, VertexLayoutCache::getStaticLayout());
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix& vp, const Vec3& camPos)
	{
		Matrix W = Matrix::translation(camPos);
		shaders->updateConstantVS("SkyBox", "staticMeshBuffer", "W", &W);
		shaders->updateConstantVS("SkyBox", "staticMeshBuffer", "VP", &vp);
		shaders->apply(core, "SkyBox");
		psos->bind(core, "SkyBoxPSO");
		shaders->updateTexturePS(core, "SkyBox", "tex", texHeapOffset);
		//char buffer[256];
		//sprintf_s(buffer, "drawSkyBox textureFilenames = %s heapIndex = %d\n", "SKY", texHeapOffset);
		//OutputDebugStringA(buffer);
		domeMesh.draw(core);
	}
};
enum class State
{
	Walking,
	Eating,
	Attacking,
	Roaring,
	Running,
	Standing,
	Dead
};
struct Entity
{
	StaticModel* staticModel = nullptr;
	AnimatedModel* animModel = nullptr;
	AnimationInstance* animInstance = nullptr;
	AABB worldAABB;

	Vec3 position = Vec3(0, 0, 0);
	Vec3 rotation = Vec3(0, 0, 0);
	float scale = 1.0f;
	Vec3 moveDir = Vec3(0, 0, 0);
	float moveSpeed = 1.0f;
	float yaw = 0.0f;
	bool isAnimated = false;
	bool useAlphaTest = false;
	bool isanimal = false;
	bool isTree = false;
	bool alive = true;
	State State = State::Walking;
};
AABB computeWorldAABB(const AABB& local, const Entity& e)
{
	AABB world;
	Vec3 s(e.scale, e.scale, e.scale);
	world.min = local.min * s + e.position;
	world.max = local.max * s + e.position;
	return world;
}
AABB shrinkAABB(const AABB& aabb, float trunkRatio = 0.25f)
{
	Vec3 center = (aabb.min + aabb.max) * 0.5f;
	Vec3 half = (aabb.max - aabb.min) * 0.5f;

	half.x *= trunkRatio;
	half.z *= trunkRatio;

	AABB out;
	out.min = Vec3(center.x - half.x, aabb.min.y, center.z - half.z);
	out.max = Vec3(center.x + half.x, aabb.max.y, center.z + half.z);
	return out;
}
void updateBullAnimation(Entity& e, float dt)
{
	if (e.animInstance)
	{
		if (e.State == State::Walking)
		{
			e.animInstance->update("walk forward", dt);
			e.position += e.moveDir * e.moveSpeed * dt;
		}
		else if (e.State == State::Dead)
		{
			e.animInstance->update("death", dt);
		}
		else if (e.State == State::Eating)
		{
			e.animInstance->update("eating", dt);
		}
		else
		{
			e.animInstance->update("idle sitting", dt);
		}
		if (e.animInstance->animationFinished())
		{
			if (e.State == State::Dead)
			{
				e.alive = false;
			}
			else {
				e.animInstance->resetAnimationTime();
				float r = randRange(0.0f, 1.0f);
				if (r < 0.25f)
				{
					e.State = State::Walking;
					float angle = randRange(0.0f, 6.28318f);
					e.moveDir = Vec3(cos(angle), 0, sin(angle));
					Vec3 visualDir = -e.moveDir;
					e.yaw = atan2(-visualDir.x, visualDir.z);
				}
				else if (r < 0.5f)
				{
					e.State = State::Standing;
				}
				else
				{
					e.State = State::Eating;
				}
			}
		}
	}
}
void updateTRex(Entity& e, const Vec3& playerPos, float dt)
{
	if (e.animInstance)
	{
		Vec3 toPlayer = playerPos - e.position;
		float distToPlayer = toPlayer.length();
		toPlayer.y = 0.0f;
		Vec3 dirToPlayer = toPlayer.normalize();

		if (e.State == State::Walking)
		{
			e.animInstance->update("walk", dt);
			e.position += e.moveDir * e.moveSpeed * dt;
			if (distToPlayer < 25.0f)
			{
				e.State = State::Running;
			}
		}
		else if (e.State == State::Dead)
		{
			e.animInstance->update("death", dt);
		}
		else if (e.State == State::Standing)
		{
			e.animInstance->update("idle", dt);
			if (distToPlayer < 25.0f)
			{
				e.State = State::Running;
			}
		}
		else if (e.State == State::Running)
		{
			e.animInstance->update("run", dt);
			e.moveDir = dirToPlayer;
			e.position += e.moveDir * e.moveSpeed * 2.0f * dt;
			e.yaw = atan2(-dirToPlayer.x, dirToPlayer.z);
			if (distToPlayer < 8.0f)
			{
				e.State = State::Attacking;
			}
		}
		else if (e.State == State::Attacking)
		{
			e.animInstance->update("attack", dt);
		}
		if (e.animInstance->animationFinished())
		{
			if (e.State == State::Dead)
			{
				e.alive = false;
			}
			else {
				e.animInstance->resetAnimationTime();
				if (distToPlayer < 25.0f)
				{
					if (distToPlayer < 8.0f)
					{
						e.State = State::Attacking;
					}
					else
					{
						e.State = State::Running;
						e.yaw = atan2(-dirToPlayer.x, dirToPlayer.z);
					}
				}
				else
				{
					float r = randRange(0.0f, 1.0f);
					if (r < 0.5f)
					{
						e.State = State::Standing;
					}
					else
					{
						e.State = State::Walking;
					}
				}
				if (e.State == State::Walking)
				{
					float angle = randRange(0.0f, 6.28318f);
					e.moveDir = Vec3(cos(angle), 0, sin(angle));
					e.yaw = atan2(-e.moveDir.x, e.moveDir.z);
				}
			}
		}
	}
}
void updateEntity(Entity& e, const Vec3& playerPos, float dt)
{
	//Distinguishing Unit Logic
	if (e.isanimal)
	{
		updateBullAnimation(e, dt);
	}
	else
	{
		updateTRex(e, playerPos, dt);
	}
}
void loadLevel(const std::string& filename,std::vector<Entity>& worldObjects, const std::unordered_map<std::string, StaticModel*>& modelTable)
{
	std::ifstream file(filename);
	std::string line;
	while (std::getline(file, line))
	{
		if (line.empty() || line[0] == '#')
			continue;
		std::stringstream ss(line);
		std::string type;
		std::string modelName;
		float x, y, z;
		float scale;
		std::string flag;
		ss >> type >> modelName >> x >> y >> z >> scale;
		Entity e;
		e.position = Vec3(x, y, z);
		e.scale = scale;
		if (type == "GRASS")
		{
			e.useAlphaTest = true;
		}
		if (type == "TREE")
		{
			e.isTree = true;
		}
		auto it = modelTable.find(modelName);
		if (it == modelTable.end())
			continue;
		e.staticModel = it->second;
		worldObjects.push_back(e);
	}
}
void generateLevel(const std::string& filename)
{
	std::ofstream file(filename);
	for (int i = 0; i < 1000; i++)
	{
		float randX = -100.0f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (200.0f)));
		float randZ = -100.0f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (200.0f)));
		file << "GRASS grassmix" << char('A' + rand() % 6) << " " << randX << " 0 " << randZ << " 5.0\n";
	}
	for (int i = 0; i < 200; i++)
	{
		float randX = -100.0f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (200.0f)));
		float randZ = -100.0f + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (200.0f)));
		file << "TREE " << (rand() % 2 == 0 ? "acacia" : "oak") << " " << randX << " 0 " << randZ << " 0.01\n";
	}
	file.close();
}
struct Player
{
	Vec3 position = Vec3(0, 2, 5); // 初始位置
	float yaw = 0.0f;              // 左右旋转（鼠标X）
	float pitch = 0.0f;            // 上下旋转（鼠标Y）
	float moveSpeed = 6.0f;        // 移动速度
	float radius = 0.3f;
	float height = 1.8f;
	float attackCD = 1.0f;
	AABB getAABB() const
	{
		AABB aabb;
		float baseY = 0.0f;
		aabb.min = Vec3(position.x - radius, baseY, position.z - radius);
		aabb.max = Vec3(position.x + radius, baseY + height, position.z + radius);
		return aabb;
	}
} player;
struct Ray
{
	Vec3 origin;
	Vec3 dir;
};
bool intersectRayAABB(const Ray& ray,const AABB& aabb,float& tHit)
{
	float tmin = 0.0f;
	float tmax = FLT_MAX;
	for (int i = 0; i < 3; i++)
	{
		float origin = ray.origin[i];
		float dir = ray.dir[i];
		float minB = aabb.min[i];
		float maxB = aabb.max[i];

		if (fabs(dir) < 1e-6f)
		{
			if (origin < minB || origin > maxB)
				return false;
		}
		else
		{
			float ood = 1.0f / dir;
			float t1 = (minB - origin) * ood;
			float t2 = (maxB - origin) * ood;
			if (t1 > t2) std::swap(t1, t2);
			tmin = std::max(tmin, t1);
			tmax = std::min(tmax, t2);
			if (tmin > tmax)
				return false;
		}
	}
	tHit = tmin;
	return true;
}
void shoot(const Vec3& camPos,const Vec3& forward,std::vector<Entity>& worldObjects)
{
	Ray ray;
	ray.origin = camPos;
	ray.dir = forward;
	float closestHit = FLT_MAX;
	Entity* hitEntity = nullptr;

	for (Entity& e : worldObjects)
	{
		if (!e.alive) continue;
		if (!e.isAnimated) continue;
		float t;
		if (intersectRayAABB(ray, e.worldAABB, t))
		{
			if (t < closestHit)
			{
				closestHit = t;
				hitEntity = &e;
			}
		}
	}
	if (hitEntity)
	{
		hitEntity->State = State::Dead;
	}
}
struct LightCB
{
	Vec3 lightDir;
	float pad0;
	Vec3 lightColor;
	float pad1;
};

#define WIDTH 1920
#define HEIGHT 1080


int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	srand((unsigned int)time(nullptr));
	Window window;
	window.create(WIDTH, HEIGHT, "My Window");
	Core core;
	core.init(window.hwnd, WIDTH, HEIGHT);
	Shaders shaders;
	PSOManager psos;
	TextureManager textures;
	SkyDome skyDome; 
	skyDome.init(&core, &psos, &textures, &shaders);
	Plane plane;
	plane.init(&core, &psos, &shaders, &textures);
	std::vector<Entity> worldObjects;

	// Timing variables
	float fpsTimer = 0.0f;
	int frameCount = 0;
	float currentFPS = 0.0f;


	// Set up debug line renderer
	DebugLineRenderer debugRenderer;
	debugRenderer.lineMesh.initDynamic(&core, 65536, sizeof(LineVertex));
	shaders.load(&core, "Line", "VS_Line.txt", "PS_Line.txt");
	psos.createLinePSO(&core, "LinePSO", shaders.find("Line")->vs, shaders.find("Line")->ps, VertexLayoutCache::getlineLayout());
	
	// Set up lighting
	LightCB light = {};
	light.lightDir = Vec3(0.3f, -1.0f, 0.2f).normalize();
	light.lightColor = Vec3(1, 1, 1);
	float sunAngle = 0.0f;
	Vec3 sunDir;

	// Load Player Model
	PlayerModel playermodel;
	playermodel.load(&core, "Models/Uzi.gem", &psos, &shaders, &textures);
	AnimationInstance playerAnimInstance;
	playerAnimInstance.animation = &playermodel.animation;
	playerAnimInstance.init(&playermodel.animation, 0);
	Ray playerRay;

	std::unordered_map<std::string, StaticModel*> modelTable;
	// Load Static Models
	StaticModel acacia;
	acacia.load(&core, "Models/acacia.gem", &shaders, &psos, &textures, false);
	StaticModel oak;
	oak.load(&core, "Models/oak.gem", &shaders, &psos, &textures, false);
	StaticModel grassmixA;
	grassmixA.load(&core, "Models/Grass_Mix_Full_01a.gem", &shaders, &psos, &textures, true);
	StaticModel grassmixB;
	grassmixB.load(&core, "Models/Grass_Mix_Full_01b.gem", &shaders, &psos, &textures, true);
	StaticModel grassmixC;
	grassmixC.load(&core, "Models/Grass_Mix_Full_01c.gem", &shaders, &psos, &textures, true);
	StaticModel grassmixD;
	grassmixD.load(&core, "Models/Grass_Mix_Full_01d.gem", &shaders, &psos, &textures, true);
	StaticModel grassmixE;
	grassmixE.load(&core, "Models/Grass_Mix_Full_01e.gem", &shaders, &psos, &textures, true);
	StaticModel grassmixF;
	grassmixF.load(&core, "Models/Grass_Mix_Full_01f.gem", &shaders, &psos, &textures, true);
	modelTable["grassmixA"] = &grassmixA;
	modelTable["grassmixB"] = &grassmixB;
	modelTable["grassmixC"] = &grassmixC;
	modelTable["grassmixD"] = &grassmixD;
	modelTable["grassmixE"] = &grassmixE;
	modelTable["grassmixF"] = &grassmixF;
	modelTable["acacia"] = &acacia;
	modelTable["oak"] = &oak;
	

	/*std::vector<StaticModel*> treeModels;
	treeModels.push_back(&acacia);
	treeModels.push_back(&oak);
	std::vector<StaticModel*> grassModels;
	grassModels.push_back(&grassmixA);
	grassModels.push_back(&grassmixB);
	grassModels.push_back(&grassmixC);
	grassModels.push_back(&grassmixD);
	grassModels.push_back(&grassmixE);
	grassModels.push_back(&grassmixF);
	for (int i = 0; i < 500; i++)
	{
		Entity grass;
		grass.staticModel = grassModels[rand() % grassModels.size()];
		float randX = randRange(-100.0f, 100.0f);
		float randZ = randRange(-100.0f, 100.0f);
		grass.position = Vec3(randX, 0, randZ);
		grass.scale = 5.0f;
		grass.useAlphaTest = true;
		worldObjects.push_back(grass);
	}
	for (int i = 0; i < 100; i++)
	{
		Entity tree;
		tree.staticModel = treeModels[rand() % treeModels.size()];
		float randX = randRange(-100.0f, 100.0f);
		float randZ = randRange(-100.0f, 100.0f);
		tree.position = Vec3(randX, 0, randZ);
		tree.scale = 0.01f;
		worldObjects.push_back(tree);
	}*/
	
	//generateLevel("level1.txt");
	loadLevel("level1.txt", worldObjects, modelTable);

	std::unordered_map<StaticModel*, std::vector<InstanceData>> instanceGroups;
	for (Entity& e : worldObjects)
	{
		if (e.staticModel)
		{
			InstanceData inst;
			inst.world =
				Matrix::scaling(Vec3(e.scale, e.scale, e.scale)) *
				Matrix::translation(e.position);
			instanceGroups[e.staticModel].push_back(inst);
		}
	}

	char buffer[512];
	sprintf_s(buffer,
		"===== InstanceGroups Debug =====\n"
	);
	OutputDebugStringA(buffer);
	int modelIndex = 0;
	for (auto& pair : instanceGroups)
	{
		StaticModel* model = pair.first;
		auto& instances = pair.second;
		sprintf_s(buffer,
			"[%d] StaticModel ptr = 0x%p | instance count = %zu\n",
			modelIndex,
			model,
			instances.size()
		);
		OutputDebugStringA(buffer);
		sprintf_s(buffer,
			"    isGrass = %s\n",
			model->isGrass ? "true" : "false"
		);
		OutputDebugStringA(buffer);
		modelIndex++;
	}
	OutputDebugStringA("===============================\n");

	// Initialize instance buffers
	for (auto& pair : instanceGroups)
	{
		StaticModel* model = pair.first;
		std::vector<InstanceData>& instances = pair.second;

		for (Mesh* m : model->meshes)
		{
			m->initInstanceBuffer(&core, instances);
		}
	}

	// Load Animated Models
	AnimatedModel bull;
	bull.load(&core, "Models/Bull-dark.gem", &psos, &shaders, &textures);
	for (int i = 0; i < 25; i++)
	{
		Entity ani;
		ani.animModel = &bull;
		float randX = randRange(-50.0f, 50.0f);
		float randZ = randRange(-50.0f, 50.0f);
		ani.position = Vec3(randX, 0, randZ);
		ani.scale = 0.01f;
		float angle = randRange(0.0f, 6.28318f);
		ani.moveDir = Vec3(cos(angle), 0, sin(angle));
		Vec3 visualDir = -ani.moveDir;
		ani.yaw = atan2(-visualDir.x, visualDir.z);
		ani.moveSpeed = 1.0f;
		ani.isAnimated = true;
		ani.isanimal = true;
		ani.animInstance = new AnimationInstance();
		ani.animInstance->init(&bull.animation, 0);
		worldObjects.push_back(ani);
	}
	AnimatedModel animatedModel;
	animatedModel.load(&core, "Models/TRex.gem", &psos, &shaders, &textures);
	AnimationInstance animatedInstance;
	animatedInstance.init(&animatedModel.animation, 0);
	Entity ani;
	ani.animModel = &animatedModel;
	ani.animInstance = &animatedInstance;
	ani.position = Vec3(30, 0, 30);
	ani.scale = 0.01f;
	ani.isAnimated = true;
	worldObjects.push_back(ani);

	Timer timer;
	float t = 0;
	while (1)
	{
		core.beginFrame();
		float dt = timer.dt();
		t += dt;
		// Calculate FPS
		fpsTimer += dt;
		frameCount++;
		if (fpsTimer >= 1.0f)
		{
			currentFPS = frameCount / fpsTimer;
			char buffer[128];
			sprintf_s(buffer, "FPS: %.2f\n", currentFPS);
			OutputDebugStringA(buffer);
			fpsTimer = 0.0f;
			frameCount = 0;
		}


		// Update light
		if (player.attackCD > 0.0f)
			player.attackCD -= dt;
		sunAngle += dt * 0.2f;
		sunDir.x = cos(sunAngle);
		sunDir.y = sin(sunAngle);
		sunDir.z = 0.2f;
		sunDir = sunDir.normalize();
		float h = clamp(sunDir.y, 0.0f, 1.0f);
		Vec3 sunriseColor = Vec3(1.0f, 0.5f, 0.3f);
		Vec3 noonColor = Vec3(1.0f, 1.0f, 0.95f);
		Vec3 nightColor = Vec3(0.1f, 0.1f, 0.3f);
		Vec3 sunColor;
		if (sunDir.y > 0)
			sunColor = Vec3().lerp(sunriseColor, noonColor, h);
		else
			sunColor = nightColor;
		light.lightDir = sunDir;
		light.lightColor = sunColor;
		shaders.updateConstantPS("StaticNormal", "LightCB", "lightDir", &light.lightDir);
		shaders.updateConstantPS("StaticNormal", "LightCB", "lightColor", &light.lightColor);

		window.checkInput();
		if (window.keys[VK_ESCAPE] == 1)
		{
			break;
		}

		// Update player
		window.updateMouseFPS();
		float sensitivity = 0.002f;
		player.yaw -= window.mouseDx * sensitivity;
		player.pitch -= window.mouseDy * sensitivity;
		if (player.pitch > 1.5f) player.pitch = 1.5f;
		if (player.pitch < -1.5f) player.pitch = -1.5f;
		Vec3 forward;
		forward.x = cos(player.pitch) * cos(player.yaw);
		forward.y = sin(player.pitch);
		forward.z = cos(player.pitch) * sin(player.yaw);
		forward = forward.normalize();
		Vec3 right = Cross(forward, Vec3(0, 1, 0)).normalize();
		Vec3 up = Cross(right, forward);
		Vec3 move(0, 0, 0);
		if (window.keys['W']) move += forward * Vec3(1, 0, 1);
		if (window.keys['S']) move -= forward * Vec3(1, 0, 1);
		if (window.keys['A']) move += right;
		if (window.keys['D']) move -= right;
		if (move.length() > 0.001f)
			move = move.normalize() * player.moveSpeed * dt;
		if (window.keys[VK_SHIFT]) move = move * 2.0f;

		Matrix vp;
		Matrix p = Matrix::perspective(0.01f, 10000.0f, 1920.0f / 1080.0f, 60.0f);
		//Vec3 from = Vec3(11 * cos(t), 5, 11 * sinf(t));
		//Matrix v = Matrix::lookAt(from, Vec3(0, 0, 0), Vec3(0, 1, 0));
		Vec3 camPos = player.position;
		Matrix v = Matrix::lookAt(
			camPos,
			camPos + forward,
			up
		);
		vp = v * p;

		shaders.updateConstantVS("StaticModel", "staticMeshBuffer", "VP", &vp);
		core.beginRenderPass();
		plane.draw(&core, &psos, &shaders, vp, &textures);

		// Update and draw world objects----just animated
		for (Entity& e : worldObjects)
			{ 
			if (!e.alive) continue;
			//Matrix W = Matrix::scaling(Vec3(e.scale, e.scale, e.scale)) * Matrix::translation(e.position);
			if (e.isAnimated == true)
			{
				updateEntity(e, player.position, dt);
				//e.yaw += dt;
				//e.worldAABB = computeWorldAABB(e.animModel->getLocalAABB(), e);
				Matrix W = Matrix::scaling(Vec3(e.scale, e.scale, e.scale)) * Matrix::rotateY(e.yaw) * Matrix::translation(e.position);
				e.worldAABB = e.animModel->getLocalAABB().transformed(W);
				e.animModel->draw(&core, &psos, &shaders, &textures, e.animInstance, vp, W);
			}// else if(e.useAlphaTest == true)
			//{
			//	e.staticModel->updateWorld(&shaders, W);
			//	e.staticModel->draw(&core, &psos, &shaders, &textures, vp, "GrassAlphaPSO", t);
			//}
			else
			{
				if (e.isTree)
				{
					Matrix W = Matrix::scaling(Vec3(e.scale, e.scale, e.scale)) * Matrix::translation(e.position);
					e.worldAABB = e.staticModel->getLocalAABB().transformed(W);
					e.worldAABB = shrinkAABB(e.worldAABB, 0.25f);
				}
				//e.worldAABB = computeWorldAABB(e.staticModel->getLocalAABB(), e);
			//	e.staticModel->updateWorld(&shaders, W);
			//	e.staticModel->draw(&core, &psos, &shaders, &textures, vp);
			}
		}

		// Draw instanced static models
		for (auto& pair : instanceGroups)
		{
			StaticModel* model = pair.first;
			auto& instances = pair.second;
			if (model->isGrass)
				model->drawInstanced(&core, &psos, &shaders, &textures, vp, instances, "GrassAlphaPSO", t);
			else
				model->drawInstanced(&core, &psos, &shaders, &textures, vp, instances);
		}
		skyDome.draw(&core, &psos, &shaders, vp, player.position);

		// Draw player model and check behavior
		debugRenderer.clear();
		std::string playerAnim = "04 idle";
		if (window.keys[VK_LBUTTON] || window.keys[VK_SPACE]) {
			playerAnim = "08 fire";
			playerRay.origin = player.position + Vec3(0, -0.25f, 0);
			playerRay.dir = forward;
			Vec3 end = playerRay.origin + playerRay.dir * 100.0f;
			debugRenderer.addLine(playerRay.origin, end, Vec3(0, 1, 0));
			// 发射射线
			if (player.attackCD <= 0.0f) {
				shoot(playerRay.origin, forward, worldObjects);
				player.attackCD = 0.5f;
			}
		}
		else if (move.length() > 0.001f) {
			if (window.keys[VK_SHIFT]) playerAnim = "07 run";
			else playerAnim = "06 walk";
		}
		if (playerAnimInstance.animation != nullptr && playerAnimInstance.animation->hasAnimation(playerAnim)) {
			playerAnimInstance.update(playerAnim, dt);
		}
		else {
			if (playerAnimInstance.animation != nullptr && playerAnimInstance.animation->hasAnimation("00 pose"))
				playerAnimInstance.update("00 pose", dt);
		}
		playermodel.draw(&core, &shaders, &psos, &textures, &playerAnimInstance, p);
		if (playerAnimInstance.animationFinished() == true)
		{
			playerAnimInstance.resetAnimationTime();
		}
		// 玩家
		debugRenderer.addAABB(player.getAABB(), Vec3(1, 0, 0));
		// 世界物体
		for (Entity& e : worldObjects)
		{
			if (!e.alive) continue;
			if (e.useAlphaTest == false) {
				debugRenderer.addAABB(e.worldAABB, Vec3(0, 1, 0));
			}
		}
		debugRenderer.draw(&core, &shaders, &psos, vp);

		//check collided
		Vec3 oldPos = player.position;
		player.position = Vec3(oldPos.x + move.x, oldPos.y, oldPos.z);
		AABB playerBox = player.getAABB();
		bool collided = false;
		for (Entity& e : worldObjects)
		{
			if (!e.alive) continue;
			if (e.useAlphaTest) continue;
			if (playerBox.intersects(e.worldAABB))
			{
				collided = true;
				break;
			}
		}
		if (collided)
		{
			// 回退 X
			player.position.x = oldPos.x;
		}
		player.position = Vec3(player.position.x, oldPos.y, oldPos.z + move.z);
		playerBox = player.getAABB();
		collided = false;
		for (Entity& e : worldObjects)
		{
			if (!e.alive) continue;
			if (e.useAlphaTest) continue;
			if (playerBox.intersects(e.worldAABB))
			{
				collided = true;
				break;
			}
		}
		if (collided)
		{
			// 回退 Z
			player.position.z = oldPos.z;
		}
		// Avoid permanent jamming
		playerBox = player.getAABB();
		for (Entity& e : worldObjects)
		{
			if (!e.alive) continue;
			if (e.useAlphaTest) continue;
			if (playerBox.intersects(e.worldAABB))
			{
				Vec3 center = (e.worldAABB.min + e.worldAABB.max) * 0.5f;
				Vec3 dir = player.position - center;
				dir.y = 0.0f;
				float len = dir.length();
				if (len < 1e-5f)
				{
					dir = Vec3(1.0f, 0.0f, 0.0f);
					len = 1.0f;
				}
				dir = dir / len;

				player.position += dir * 0.05f;
				playerBox = player.getAABB();
			}
		}

		core.drawToScreen();
		core.finishFrame();
	}
	core.flushGraphicsQueue();
}