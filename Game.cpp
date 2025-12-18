

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

// Properties -> Linker -> System -> Windows

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

struct InstanceData
{
	Matrix world;
};

class Plane
{
public:
	Mesh mesh;
	std::string shaderName;
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
	void init(Core* core, PSOManager *psos, Shaders* shaders)
	{
		std::vector<STATIC_VERTEX> vertices;
		vertices.push_back(addVertex(Vec3(-1, 0, -1), Vec3(0, 1, 0), 0, 0));
		vertices.push_back(addVertex(Vec3(1, 0, -1), Vec3(0, 1, 0), 1, 0));
		vertices.push_back(addVertex(Vec3(-1, 0, 1), Vec3(0, 1, 0), 0, 1));
		vertices.push_back(addVertex(Vec3(1, 0, 1), Vec3(0, 1, 0), 1, 1));
		std::vector<unsigned int> indices;
		indices.push_back(0);
		indices.push_back(1);
		indices.push_back(2);
		indices.push_back(1);
		indices.push_back(3);
		indices.push_back(2);
		mesh.init(core, vertices, indices);
		shaders->load(core, "StaticModelUntextured", "VS.txt", "PSUntextured.txt");
		shaderName = "StaticModelUntextured";
		psos->createPSO(core, "StaticModelUntexturedPSO", shaders->find("StaticModelUntextured")->vs, shaders->find("StaticModelUntextured")->ps, VertexLayoutCache::getStaticLayout());
	}
	void draw(Core* core, PSOManager* psos, Shaders* shaders, Matrix &vp)
	{
		Matrix planeWorld;
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "VP", &vp);
		shaders->updateConstantVS("StaticModelUntextured", "staticMeshBuffer", "W", &planeWorld);
		shaders->apply(core, shaderName);
		psos->bind(core, "StaticModelUntexturedPSO");
		mesh.draw(core);
	}
};

class StaticModel
{
public:
	std::vector<Mesh *> meshes;
	std::vector<std::string> textureFilenames;
	std::vector<std::string> normalFilenames;
	void load(Core* core, std::string filename, Shaders* shaders, PSOManager* psos, TextureManager* textures)
	{
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
		shaders->load(core, "StaticModel", "VS.txt", "PSTextured.txt");
		psos->createPSO(core, "StaticModelPSO", shaders->find("StaticModel")->vs, shaders->find("StaticModel")->ps, VertexLayoutCache::getStaticLayout());
		shaders->load(core, "GrassAlpha", "VS_grass.txt", "PS_AlphaTest.txt");
		psos->createPSO(core, "GrassAlphaPSO", shaders->find("GrassAlpha")->vs, shaders->find("GrassAlpha")->ps, VertexLayoutCache::getStaticLayout());
		shaders->load(core, "StaticNormal", "VS.txt", "PSNormalMap.txt");
		psos->createPSO(core, "StaticNormalPSO", shaders->find("StaticNormal")->vs, shaders->find("StaticNormal")->ps, VertexLayoutCache::getStaticLayout());
	}
	void updateWorld(Shaders* shaders, Matrix& w)
	{
		shaders->updateConstantVS("StaticModel", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("GrassAlpha", "staticMeshBuffer", "W", &w);
		shaders->updateConstantVS("StaticNormal", "staticMeshBuffer", "W", &w);
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

struct Entity
{
	StaticModel* staticModel = nullptr;
	AnimatedModel* animModel = nullptr;

	Vec3 position = Vec3(0, 0, 0);
	Vec3 rotation = Vec3(0, 0, 0);
	float scale = 1.0f;

	bool isAnimated = false;
	bool useAlphaTest = false;
};
std::vector<Entity> worldObjects;
struct Player
{
	Vec3 position = Vec3(0, 2, 5); // 初始位置
	float yaw = 0.0f;              // 左右旋转（鼠标X）
	float pitch = 0.0f;            // 上下旋转（鼠标Y）
	float moveSpeed = 6.0f;        // 移动速度
} player;
auto randRange = [](float min, float max)
	{
		return min + (float(rand()) / float(RAND_MAX)) * (max - min);
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
	Plane plane;
	plane.init(&core, &psos, &shaders);

	StaticModel acacia;
	acacia.load(&core, "Models/acacia.gem", &shaders, &psos, &textures);
	StaticModel oak;
	oak.load(&core, "Models/oak.gem", &shaders, &psos, &textures);
	std::vector<StaticModel*> treeModels;
	treeModels.push_back(&acacia);
	treeModels.push_back(&oak);
	StaticModel grassmixA;
	grassmixA.load(&core, "Models/Grass_Mix_Full_01a.gem", &shaders, &psos, &textures);
	StaticModel grassmixB;
	grassmixB.load(&core, "Models/Grass_Mix_Full_01b.gem", &shaders, &psos, &textures);
	StaticModel grassmixC;
	grassmixC.load(&core, "Models/Grass_Mix_Full_01c.gem", &shaders, &psos, &textures);
	StaticModel grassmixD;
	grassmixD.load(&core, "Models/Grass_Mix_Full_01d.gem", &shaders, &psos, &textures);
	StaticModel grassmixE;
	grassmixE.load(&core, "Models/Grass_Mix_Full_01e.gem", &shaders, &psos, &textures); 
	StaticModel grassmixF;
	grassmixF.load(&core, "Models/Grass_Mix_Full_01f.gem", &shaders, &psos, &textures);
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
	}
	/**/AnimatedModel animatedModel;
	animatedModel.load(&core, "Models/TRex.gem", &psos, &shaders, &textures);
	Entity e3;
	e3.animModel = &animatedModel;
	e3.position = Vec3(0, 0, 0);
	e3.scale = 0.01f;
	e3.isAnimated = true;
	worldObjects.push_back(e3);
	AnimationInstance animatedInstance;
	animatedInstance.init(&animatedModel.animation, 0); 
	skyDome.init(&core, &psos, &textures, &shaders);
	Timer timer;
	float t = 0;
	while (1)
	{
		core.beginFrame();
		float dt = timer.dt();
		t += dt;
		window.checkInput();
		if (window.keys[VK_ESCAPE] == 1)
		{
			break;
		}
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
		player.position += move;

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
		plane.draw(&core, &psos, &shaders, vp);

		//Matrix W;
		//W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translation(Vec3(5, 0, 0));
		//staticModel.updateWorld(&shaders, W);
		//staticModel.draw(&core, &psos, &shaders, vp);
		//W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f)) * Matrix::translation(Vec3(10, 0, 0));
		//staticModel.updateWorld(&shaders, W);
		//staticModel.draw(&core, &psos, &shaders, vp);

		animatedInstance.update("run", dt);
		if (animatedInstance.animationFinished() == true)
		{
			animatedInstance.resetAnimationTime();
		} 
		//shaders.updateConstantVS("AnimatedModel", "staticMeshBuffer", "VP", &vp);
		//W = Matrix::scaling(Vec3(0.01f, 0.01f, 0.01f));
		//animatedModel.draw(&core, &psos, &shaders, &textures, &animatedInstance, vp, W);
		for (Entity& e : worldObjects)
			{ 
			Matrix W = Matrix::scaling(Vec3(e.scale, e.scale, e.scale)) * Matrix::translation(e.position);
			if (e.isAnimated == true)
			{
				e.animModel->draw(&core, &psos, &shaders, &textures, &animatedInstance, vp, W);
			} else if(e.useAlphaTest == true)
			{
				e.staticModel->updateWorld(&shaders, W);
				e.staticModel->draw(&core, &psos, &shaders, &textures, vp, "GrassAlphaPSO", t);
			}
			else
			{
				e.staticModel->updateWorld(&shaders, W);
				e.staticModel->draw(&core, &psos, &shaders, &textures, vp);
			}
		}
		skyDome.draw(&core, &psos, &shaders, vp, player.position);

		core.drawToScreen();
		core.finishFrame();
	}
	core.flushGraphicsQueue();
}