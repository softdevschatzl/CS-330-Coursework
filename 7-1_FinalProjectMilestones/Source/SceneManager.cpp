///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

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
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
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
 * 
 * Update by Student 6/12/2024: Added some control flow 
 * with the if statement to ensure the textures were
 * being set correctly.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = FindTextureSlot(textureTag);
		if (textureID != -1) 
		{
			glActiveTexture(GL_TEXTURE0 + textureID);
			glBindTexture(GL_TEXTURE_2D, m_textureIDs[textureID].ID);
			m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
		}
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
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
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
*	LoadSceneTextures()
* 
*	This method is used for loading the textures for the 3D
*	scene.
************************************************************/
void SceneManager::LoadSceneTextures()
{
	// debug log
	std::cout << "Loading textures for the 3D scene..." << std::endl;

	CreateGLTexture("textures/ashberrysmooth.jpg", "ashberry");
	CreateGLTexture("textures/flagstonerubble.jpg", "flagstone");
	CreateGLTexture("textures/granite.jpg", "granite");
	CreateGLTexture("textures/marmoreal.jpg", "marmoreal");
	CreateGLTexture("textures/oak.jpg", "oak");
	CreateGLTexture("textures/charredtimber.jpg", "charredtimber");
	CreateGLTexture("textures/black-leather.jpg", "black-leather");
	CreateGLTexture("textures/fabric.jpg", "fabric");
	CreateGLTexture("textures/gray-surface.jpg", "gray-surface");
	CreateGLTexture("textures/green-blue-surface.jpg", "green-blue-surface");
	CreateGLTexture("textures/clock-face.jpg", "clock-face");

	// debug log
	std::cout << "Finished loading textures for the 3D scene." << std::endl;
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring various material
 *  settings for the 3D objects in the scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// debug log
	std::cout << "Defining object materials..." << std::endl;

	OBJECT_MATERIAL material;

	// define the material for the first object
	material.tag = "charredtimber";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f); // Slightly visible in ambient light
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.2f, 0.1f, 0.05f); // Dark brown
	material.specularColor = glm::vec3(0.5f, 0.5f, 0.5f); // Medium specular reflection
	material.shininess = 32.0f; // Shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the second object
	material.tag = "ashberry";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.6f, 0.2f, 0.2f); // Berry color
	material.specularColor = glm::vec3(0.7f, 0.7f, 0.7f); // High specular reflection
	material.shininess = 64.0f; // Higher shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the third object
	material.tag = "flagstone";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.4f, 0.4f, 0.4f); // Grey
	material.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Low specular reflection
	material.shininess = 16.0f; // Lower shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the fourth object
	material.tag = "granite";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Light grey
	material.specularColor = glm::vec3(0.8f, 0.8f, 0.8f); // High specular reflection
	material.shininess = 128.0f; // Very shiny
	m_objectMaterials.push_back(material);

	// define the material for the fifth object
	material.tag = "marmoreal";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // White
	material.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Very high specular reflection
	material.shininess = 256.0f; // Extremely shiny
	m_objectMaterials.push_back(material);

	// define the material for the sixth object
	material.tag = "black-leather";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f); // Black
	material.specularColor = glm::vec3(0.2f, 0.2f, 0.2f); // Low specular reflection
	material.shininess = 8.0f; // Low shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the seventh object
	material.tag = "fabric";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f); // Dark grey
	material.specularColor = glm::vec3(0.3f, 0.3f, 0.3f); // Low specular reflection
	material.shininess = 16.0f; // Lower shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the eighth object
	material.tag = "gray-surface";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f); // Light grey
	material.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // Medium specular reflection
	material.shininess = 32.0f; // Shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the ninth object
	material.tag = "green-blue-surface";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.0f, 0.5f, 0.5f); // Greenish Aqua
	material.specularColor = glm::vec3(0.6f, 0.6f, 0.6f); // Medium specular reflection
	material.shininess = 32.0f; // Shininess factor
	m_objectMaterials.push_back(material);

	// define the material for the tenth object
	material.tag = "clock-face";
	material.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	material.ambientStrength = 0.1f;
	material.diffuseColor = glm::vec3(0.8f, 0.8f, 0.8f); // White
	material.specularColor = glm::vec3(0.9f, 0.9f, 0.9f); // Very high specular reflection
	material.shininess = 256.0f; // Extremely shiny
	m_objectMaterials.push_back(material);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is used for configuring various light
 *  sources in the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// debug log
	std::cout << "Setting up scene lights..." << std::endl;

	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// General ambient light level
	glm::vec3 ambientLight = glm::vec3(0.1f, 0.1f, 0.1f);

	// Variables for other light settings to simulate sunlight.
	glm::vec3 directionalLightDirection = glm::vec3(-0.2f, -1.0f, -0.3f);
	glm::vec3 directionalLightAmbient = glm::vec3(0.3f, 0.24f, 0.1f); // Slightly yellowish
	glm::vec3 directionalLightDiffuse = glm::vec3(0.8f, 0.7f, 0.5f); // Warm sunlight
	glm::vec3 directionalLightSpecular = glm::vec3(1.0f, 0.9f, 0.8f); // Soft white

	// Light source 1
	m_pShaderManager->setVec3Value("lightSources[0].position", 3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", directionalLightAmbient);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", directionalLightDiffuse);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", directionalLightSpecular);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Light source 2
	m_pShaderManager->setVec3Value("lightSources[1].position", -3.0f, 14.0f, 0.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", directionalLightAmbient);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", directionalLightDiffuse);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", directionalLightSpecular);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.05f);

	// Light source 3 - Slightly Blue
	m_pShaderManager->setVec3Value("lightSources[2].position", 0.6f, 5.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.2f, 0.2f, 0.4f);
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.4f, 0.4f, 0.8f);
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.5f, 0.5f, 1.0f);
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 0.5f);

	// Light source 4
	m_pShaderManager->setVec3Value("lightSources[3].position", -0.6f, 7.0f, -6.0f);
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.1f, 0.1f, 0.1f);
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.6f, 0.6f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.9f, 0.9f, 0.9f);
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 12.0f);
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.5f);

	m_pShaderManager->setBoolValue("bUseLighting", true);
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
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadConeMesh();

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

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
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

	//SetShaderColor(1, 1, 1, 1);
	SetShaderTexture("charredtimber");
	SetShaderMaterial("charredtimber");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	// BEGIN STUDENT CODE

	// Set needed transformations before drawing the basic mesh.

	//
	// BOTTOM CYLINDER
	//
	// Setting transformations for the bottom cylinder.
	glm::vec3 bottomCylinderScale = glm::vec3(1.0f, 2.0f, 1.0f);
	float bottomCylinderXrotationDegrees = 0.0f;
	float bottomCylinderYrotationDegrees = 0.0f;
	float bottomCylinderZrotationDegrees = 0.0f;
	glm::vec3 bottomCylinderPosition = glm::vec3(9.0f, 0.0f, 0.0f);

	// Apply transformations to the bottom cylinder.
	SetTransformations(
		bottomCylinderScale,
		bottomCylinderXrotationDegrees,
		bottomCylinderYrotationDegrees,
		bottomCylinderZrotationDegrees,
		bottomCylinderPosition);

	// Set the color values into the shader.
	//SetShaderColor(0.0f, 1.0f, 0.7f, 1.0f); // Greenish Aqua?
	SetShaderTexture("ashberry");
	SetShaderMaterial("ashberry");

	// Draw the bottom cylinder with transformation values.
	m_basicMeshes->DrawCylinderMesh();

	//
	// TOP CYLINDER
	//
	// Setting transformations for the top cylinder.
	glm::vec3 topCylinderScale = glm::vec3(1.0f, 0.5f, 1.0f); // Shorter cylinder
	float topCylinderXrotationDegrees = 0.0f;
	float topCylinderYrotationDegrees = 0.0f;
	float topCylinderZrotationDegrees = 0.0f;
	glm::vec3 topCylinderPosition = glm::vec3(9.0f, 2.0f, 0.0f); // Positioned on top of the bottom cylinder

	// Apply transformations to the top cylinder.
	SetTransformations(
		topCylinderScale,
		topCylinderXrotationDegrees,
		topCylinderYrotationDegrees,
		topCylinderZrotationDegrees,
		topCylinderPosition);

	// Set the color values into the shader.
	//SetShaderColor(0.0f, 1.0f, 0.7f, 1.0f); // Greenish Aqua?
	SetShaderTexture("flagstone");
	SetShaderMaterial("flagstone");

	// Draw the top cylinder with transformation values.
	m_basicMeshes->DrawTaperedCylinderMesh();

	//
	// BOTTOM TORUS
	//
	// Setting transformations for the bottom torus.
	glm::vec3 bottomTorusScale = glm::vec3(0.8f, 0.8f, 0.2f);
	float bottomTorusXrotationDegrees = 90.0f;
	float bottomTorusYrotationDegrees = 0.0f;
	float bottomTorusZrotationDegrees = 0.0f;
	glm::vec3 bottomTorusPosition = glm::vec3(9.0f, 2.2f, 0.0f); // Positioned at the top of the bottom cylinder

	// Apply transformations to the bottom torus.
	SetTransformations(
		bottomTorusScale,
		bottomTorusXrotationDegrees,
		bottomTorusYrotationDegrees,
		bottomTorusZrotationDegrees,
		bottomTorusPosition);

	// Set the color values into the shader.
	//SetShaderColor(0.0f, 1.0f, 0.7f, 1.0f); // Greenish Aqua?
	SetShaderTexture("granite");
	SetShaderMaterial("granite");

	// Draw the bottom torus with transformation values.
	m_basicMeshes->DrawTorusMesh();

	//
	// MID-TORI CYLINDER
	//
	// Setting transformations for the mid cylinder.
	glm::vec3 midCylinderScale = glm::vec3(0.75f, 0.5f, 0.75f);
	float midCylinderXrotationDegrees = 0.0f;
	float midCylinderYrotationDegrees = 0.0f;
	float midCylinderZrotationDegrees = 0.0f;
	glm::vec3 midCylinderPosition = glm::vec3(9.0f, 2.0f, 0.0f);

	// Apply transformations to the mid cylinder.
	SetTransformations(
		midCylinderScale,
		midCylinderXrotationDegrees,
		midCylinderYrotationDegrees,
		midCylinderZrotationDegrees,
		midCylinderPosition);

	// Set the color values into the shader.
	//SetShaderColor(0.0f, 1.0f, 0.7f, 1.0f); // Greenish Aqua?
	SetShaderTexture("flagstone");
	SetShaderMaterial("flagstone");

	// Draw the bottom cylinder with transformation values.
	m_basicMeshes->DrawCylinderMesh();

	//
	// TOP TORUS
	//
	// Setting transformations for the top torus.
	glm::vec3 topTorusScale = glm::vec3(0.8f, 0.8f, 0.2f);
	float topTorusXrotationDegrees = 90.0f;
	float topTorusYrotationDegrees = 0.0f;
	float topTorusZrotationDegrees = 0.0f;
	glm::vec3 topTorusPosition = glm::vec3(9.0f, 2.4f, 0.0f); // Positioned at the top of the top cylinder

	// Apply transformations to the top torus.
	SetTransformations(
		topTorusScale,
		topTorusXrotationDegrees,
		topTorusYrotationDegrees,
		topTorusZrotationDegrees,
		topTorusPosition);

	// Set the color values into the shader.
	//SetShaderColor(0.0f, 1.0f, 0.7f, 1.0f); // Greenish Aqua?
	SetShaderTexture("granite");
	SetShaderMaterial("granite");

	// Draw the top torus with transformation values.
	m_basicMeshes->DrawTorusMesh();

	//
	// PEN 1
	//
	// Setting transformations for the first pen.
	glm::vec3 pen1Scale = glm::vec3(0.1f, 0.7f, 0.1f); // Thin and tall cylinder
	float pen1XrotationDegrees = -30.0f;
	float pen1YrotationDegrees = 0.0f;
	float pen1ZrotationDegrees = 0.0f;
	glm::vec3 pen1Position = glm::vec3(8.8f, 2.5f, 0.0f); // Positioned at the top of the cup

	// Apply transformations to the first pen.
	SetTransformations(
		pen1Scale,
		pen1XrotationDegrees,
		pen1YrotationDegrees,
		pen1ZrotationDegrees,
		pen1Position);

	// Set the color values into the shader.
	SetShaderColor(1.0f, 0.0f, 0.0f, 1.0f); // Red pen
	SetShaderMaterial("flagstone");


	// Draw the first pen with transformation values.
	m_basicMeshes->DrawCylinderMesh();

	//
	// PEN 2
	//
	// Setting transformations for the second pen.
	glm::vec3 pen2Scale = glm::vec3(0.1f, 0.7f, 0.1f); // Thin and tall cylinder
	float pen2XrotationDegrees = 30.0f;
	float pen2YrotationDegrees = 0.0f;
	float pen2ZrotationDegrees = 0.0f;
	glm::vec3 pen2Position = glm::vec3(9.4f, 2.5f, 0.0f); // Positioned at the top of the cup, slightly to the side of the first pen

	// Apply transformations to the second pen.
	SetTransformations(
		pen2Scale,
		pen2XrotationDegrees,
		pen2YrotationDegrees,
		pen2ZrotationDegrees,
		pen2Position);

	// Set the color values into the shader.
	SetShaderColor(0.0f, 0.0f, 1.0f, 1.0f); // Blue pen
	SetShaderMaterial("flagstone");

	// Draw the second pen with transformation values.
	m_basicMeshes->DrawCylinderMesh();

	// 
	// LAMP
	//
	// Setting transformations for the lamp.
	glm::vec3 lampBaseScale = glm::vec3(1.0f, 0.2f, 1.0f);
	float lampBaseXrotationDegrees = 0.0f;
	float lampBaseYrotationDegrees = 0.0f;
	float lampBaseZrotationDegrees = 0.0f;
	glm::vec3 lampBasePosition = glm::vec3(-5.0f, 0.1f, 0.0f);

	SetTransformations(
		lampBaseScale,
		lampBaseXrotationDegrees,
		lampBaseYrotationDegrees,
		lampBaseZrotationDegrees,
		lampBasePosition);

	SetShaderTexture("gray-surface");
	SetShaderMaterial("gray-surface");

	m_basicMeshes->DrawCylinderMesh();

	// Lamp Stem (Thin Cylinder)
	glm::vec3 lampStemScale = glm::vec3(0.2f, 3.0f, 0.2f);
	float lampStemXrotationDegrees = 0.0f;
	float lampStemYrotationDegrees = 0.0f;
	float lampStemZrotationDegrees = 0.0f;
	glm::vec3 lampStemPosition = glm::vec3(-5.0f, 0.5f, 0.0f);

	SetTransformations(
		lampStemScale,
		lampStemXrotationDegrees,
		lampStemYrotationDegrees,
		lampStemZrotationDegrees,
		lampStemPosition);

	SetShaderTexture("gray-surface");
	SetShaderMaterial("gray-surface");

	m_basicMeshes->DrawCylinderMesh();

	// Lamp Shade (Cone)
	glm::vec3 lampShadeScale = glm::vec3(1.5f, 1.5f, 1.5f);
	float lampShadeXrotationDegrees = 0.0f;
	float lampShadeYrotationDegrees = 0.0f;
	float lampShadeZrotationDegrees = 0.0f;
	glm::vec3 lampShadePosition = glm::vec3(-5.0f, 3.0f, 0.0f);

	SetTransformations(
		lampShadeScale,
		lampShadeXrotationDegrees,
		lampShadeYrotationDegrees,
		lampShadeZrotationDegrees,
		lampShadePosition);

	SetShaderTexture("fabric");
	SetShaderMaterial("fabric");

	m_basicMeshes->DrawConeMesh();

	//
	// CLOCK
	//
	// Clock (Box)
	glm::vec3 clockScale = glm::vec3(1.0f, 0.5f, 1.0f);
	float clockXrotationDegrees = 0.0f;
	float clockYrotationDegrees = 0.0f;
	float clockZrotationDegrees = 0.0f;
	glm::vec3 clockPosition = glm::vec3(-7.0f, 0.5f, 0.0f);

	SetTransformations(
		clockScale,
		clockXrotationDegrees,
		clockYrotationDegrees,
		clockZrotationDegrees,
		clockPosition);

	SetShaderTexture("black-leather");
	SetShaderMaterial("black-leather");

	m_basicMeshes->DrawBoxMesh();

	//
	// CLOCK SCREEN
	//
	// Clock Screen (Box)
	glm::vec3 clockScreenScale = glm::vec3(0.9f, 0.4f, 0.9f);
	float clockScreenXrotationDegrees = 0.0f;
	float clockScreenYrotationDegrees = 0.0f;
	float clockScreenZrotationDegrees = 0.0f;
	glm::vec3 clockScreenPosition = glm::vec3(-7.0f, 0.5f, 0.075f);

	SetTransformations(
		clockScreenScale,
		clockScreenXrotationDegrees,
		clockScreenYrotationDegrees,
		clockScreenZrotationDegrees,
		clockScreenPosition);

	SetShaderTexture("clock-face");
	SetShaderMaterial("clock-face");

	m_basicMeshes->DrawBoxMesh();

	//
	// HANDSOAP BOTTLE

	// HandSoap bottle (Cylinder)
	glm::vec3 lotionBodyScale = glm::vec3(0.5f, 1.5f, 0.5f);
	float lotionBodyXrotationDegrees = 0.0f;
	float lotionBodyYrotationDegrees = 0.0f;
	float lotionBodyZrotationDegrees = 0.0f;
	glm::vec3 lotionBodyPosition = glm::vec3(7.0f, 0.1f, 0.0f);

	SetTransformations(
		lotionBodyScale,
		lotionBodyXrotationDegrees,
		lotionBodyYrotationDegrees,
		lotionBodyZrotationDegrees,
		lotionBodyPosition);

	SetShaderTexture("black-leather");
	SetShaderMaterial("green-blue-surface");

	m_basicMeshes->DrawCylinderMesh();

	// Bottle Pump (Cylinder)
	glm::vec3 lotionPumpScale = glm::vec3(0.2f, 0.5f, 0.2f);
	float lotionPumpXrotationDegrees = 0.0f;
	float lotionPumpYrotationDegrees = 0.0f;
	float lotionPumpZrotationDegrees = 0.0f;
	glm::vec3 lotionPumpPosition = glm::vec3(7.0f, 1.5f, 0.0f);

	SetTransformations(
		lotionPumpScale,
		lotionPumpXrotationDegrees,
		lotionPumpYrotationDegrees,
		lotionPumpZrotationDegrees,
		lotionPumpPosition);

	SetShaderTexture("gray-surface");
	SetShaderMaterial("gray-surface");

	m_basicMeshes->DrawCylinderMesh();

	// Middle part of bottle pump (Cylinder)
	glm::vec3 lotionPumpMiddleScale = glm::vec3(0.1f, 0.2f, 0.1f);
	float lotionPumpMiddleXrotationDegrees = 0.0f;
	float lotionPumpMiddleYrotationDegrees = 0.0f;
	float lotionPumpMiddleZrotationDegrees = 0.0f;
	glm::vec3 lotionPumpMiddlePosition = glm::vec3(7.0f, 2.0f, 0.0f);

	SetTransformations(
		lotionPumpMiddleScale,
		lotionPumpMiddleXrotationDegrees,
		lotionPumpMiddleYrotationDegrees,
		lotionPumpMiddleZrotationDegrees,
		lotionPumpMiddlePosition);

	SetShaderTexture("gray-surface");
	SetShaderMaterial("gray-surface");

	m_basicMeshes->DrawCylinderMesh();
}
