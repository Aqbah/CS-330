///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ================
// This file contains the implementation of the `SceneManager` class, which is 
// responsible for managing the preparation and rendering of 3D scenes. It 
// handles textures, materials, lighting configurations, and object rendering.
//
// AUTHOR: Brian Battersby
// INSTITUTION: Southern New Hampshire University (SNHU)
// COURSE: CS-330 Computational Graphics and Visualization
//
// INITIAL VERSION: November 1, 2023
// LAST REVISED: December 1, 2024
//
// RESPONSIBILITIES:
// - Load, bind, and manage textures in OpenGL.
// - Define materials and lighting properties for 3D objects.
// - Manage transformations and shader configurations.
// - Render complex 3D scenes using basic meshes.
//
// NOTE: This implementation leverages external libraries like `stb_image` for 
// texture loading and GLM for matrix and vector operations.
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
* LoadSceneTextures()
*
* This method is used for preparing the 3D scene by loading
* the shapes, textures in memory to support the 3D scene
* rendering
***********************************************************/
void SceneManager::LoadSceneTextures() {
	bool bReturn = false;

	bReturn = CreateGLTexture("textures/leather1.jpg", "dash");//Dashboard leather texture
	bReturn = CreateGLTexture("textures/tesla_screen.jpg", "screen");//Screen display texture
	bReturn = CreateGLTexture("textures/leatherwhite.jpg", "base");//Dashboard base texture which is a white leather base
	bReturn = CreateGLTexture("textures/metalgrid.jpg", "ground");//Ground texture for plane
	bReturn = CreateGLTexture("textures/grayleather.jpg", "dashText");//Alternative texture for the dashboard.
	bReturn = CreateGLTexture("textures/black_plastic.jpg", "plastic");//Plastic texture
	bReturn = CreateGLTexture("textures/steering_wheel.jpg", "wheel");//Steering wheel texture

	BindGLTextures();
}

/********************************************************
*DefineObjectMaterials()
*
* This method is used for configuring the various material
* settings for all of the objects within the 3D scene.
* **********************************************************/
void SceneManager::DefineObjectMaterials() {

	OBJECT_MATERIAL matteBlack;
	matteBlack.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	matteBlack.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	matteBlack.shininess = 8.0f;
	matteBlack.tag = "matteblack";
	m_objectMaterials.push_back(matteBlack);

	OBJECT_MATERIAL polishWhite;
	polishWhite.diffuseColor = glm::vec3(0.95f, 0.95f, 0.95f);
	polishWhite.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	polishWhite.shininess = 32.0f;
	polishWhite.tag = "polishwhite";
	m_objectMaterials.push_back(polishWhite);

	OBJECT_MATERIAL glassScreen;
	glassScreen.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	glassScreen.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassScreen.shininess = 128.0f;
	glassScreen.tag = "glassscreen";
	m_objectMaterials.push_back(glassScreen);

	OBJECT_MATERIAL greyLeather;
	greyLeather.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	greyLeather.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	greyLeather.shininess = 4.0f;
	greyLeather.tag = "dashmat";
	m_objectMaterials.push_back(greyLeather);

	OBJECT_MATERIAL plastic;
	plastic.diffuseColor = glm::vec3(0.15f, 0.15f, 0.15f);
	plastic.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	plastic.shininess = 16.0f;
	plastic.tag = "plastic";
	m_objectMaterials.push_back(plastic);
}

void SceneManager::SetupSceneLights() {
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	//Directional light which is simulating sunlight
	m_pShaderManager->setVec3Value("directionalLight.direction", 0.2f, -0.2f, -0.5f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.1f, 0.0f, 0.1f);
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.8f, 0.8f, 0.8f);
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	//Point Light which simulates the interior light of a car
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 2.5f, -2.0f);
	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 1.0f, 1.0f, 1.0f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 0.2f, 0.2f, 0.2f);
	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	DefineObjectMaterials();
	LoadSceneTextures();
	SetupSceneLights();
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene
	

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

/***************************************************************************************************/
	//Drawing the ground plane
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderMaterial("dashmat");
	SetShaderTexture("ground");

	m_basicMeshes->DrawPlaneMesh();
	/***************************************************************************************************/

	/*** COMPLETELY REDESIGNED DASHBOARD ***/
	// Main dashboard body - curved design
	scaleXYZ = glm::vec3(2.8f, 0.12f, 0.6f);  // Wider, thinner, shallower
	positionXYZ = glm::vec3(0.0f, 0.8f, -1.5f);  // Positioned closer to driver

	SetTransformations(
		scaleXYZ,
		20.0f,  // Natural angle toward driver
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("dashmat");
	SetShaderTexture("dash");
	SetTextureUVScale(3.0f, 1.0f);  // Better leather texture scaling
	m_basicMeshes->DrawCylinderMesh();  // Using cylinder for curved dashboard

	/***************************************************************************************************/

	// Dashboard top surface - minimalist design
	scaleXYZ = glm::vec3(2.8f, 0.02f, 0.2f);
	positionXYZ = glm::vec3(0.0f, 0.95f, -1.4f);  // Sits on top of main body

	SetTransformations(
		scaleXYZ,
		5.0f,  // Nearly horizontal
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("metal");
	SetShaderColor(0.15f, 0.15f, 0.15f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	/*** ACCURATE TOUCHSCREEN (VERTICAL, CENTER-MOUNTED) ***/
	// Screen frame (thin bezel)
	scaleXYZ = glm::vec3(0.49f, 0.65f, 0.04f);  // Proper 15:9 aspect ratio
	positionXYZ = glm::vec3(0.0f, 0.9f, -0.85f);  // Positioned above dashboard

	SetTransformations(
		scaleXYZ,
		5.0f,  // Slight tilt back
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("plastic");
	SetShaderColor(0.05f, 0.05f, 0.05f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	// Display surface (with Tesla UI)
	scaleXYZ = glm::vec3(0.47f, 0.63f, 0.02f);  // Slightly smaller than frame
	positionXYZ = glm::vec3(0.0f, 0.9f, -0.83f);  // Slightly in front of frame

	SetTransformations(
		scaleXYZ,
		5.0f,
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("glassscreen");
	SetShaderTexture("screen");
	SetTextureUVScale(1.0f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	/*** STEERING WHEEL (PROPER SCALE AND PLACEMENT) ***/
	// Steering wheel ring
	scaleXYZ = glm::vec3(0.26f, 0.26f, 0.05f);  // Proper diameter
	positionXYZ = glm::vec3(-0.65f, 0.85f, -0.7f);  // Driver position

	SetTransformations(
		scaleXYZ,
		20.0f,  // Angled toward driver
		0.0f,  // Slightly turned
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("plastic");
	SetShaderTexture("wheel");
	m_basicMeshes->DrawTorusMesh();

	/***************************************************************************************************/

	// Steering column
	scaleXYZ = glm::vec3(0.04f, 0.41f, 0.04f);
	positionXYZ = glm::vec3(-0.65f, 0.85f, -0.7f);  // Connecting to dashboard

	SetTransformations(
		scaleXYZ,
		-90.0f,  // Match wheel angle
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("matteblack");
	m_basicMeshes->DrawCylinderMesh();

	/***************************************************************************************************/

	// Steering wheel center (airbag cover)
	scaleXYZ = glm::vec3(0.12f, 0.12f, 0.05f);
	positionXYZ = glm::vec3(-0.65f, 0.85f, -0.7f);  // Center of wheel

	SetTransformations(
		scaleXYZ,
		20.0f,
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("plastic");
	SetShaderColor(0.2f, 0.2f, 0.2f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	/*** DRIVER'S SEAT (SIMPLE BUT RECOGNIZABLE) ***/
	// Seat base
	scaleXYZ = glm::vec3(0.5f, 0.1f, 0.5f);
	positionXYZ = glm::vec3(-0.7f, 0.1f, 0.0f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,  // Slight angle for driver position
		positionXYZ
	);
	SetShaderMaterial("dashmat");
	SetShaderTexture("dash");
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	// Seat back
	scaleXYZ = glm::vec3(0.5f, 0.7f, 0.1f);
	positionXYZ = glm::vec3(-0.699f, 0.5f, 0.35);

	SetTransformations(
		scaleXYZ,
		15.0f,
		0.0f,
		0.0f,  // Reclined angle
		positionXYZ
	);
	SetShaderMaterial("dashmat");
	SetShaderTexture("dash");
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	/*** CENTER CONSOLE (RUNNING BETWEEN SEATS) ***/
	scaleXYZ = glm::vec3(0.3f, 0.25f, 1.8f);
	positionXYZ = glm::vec3(0.0f, 0.2f, -0.2f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("plastic");
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	/***************************************************************************************************/

	// Cup holders
	scaleXYZ = glm::vec3(0.2f, 0.1f, 0.2f);
	positionXYZ = glm::vec3(0.0f, 0.3f, 0.4f);

	SetTransformations(
		scaleXYZ,
		0.0f,
		0.0f,
		0.0f,
		positionXYZ
	);
	SetShaderMaterial("matteblack");
	m_basicMeshes->DrawBoxMesh();
}

