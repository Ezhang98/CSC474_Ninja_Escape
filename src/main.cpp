/*
CPE/CSC 474 Lab base code Eckhardt/Dahl
based on CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

#include <iostream>
#include <glad/glad.h>
#include "SmartTexture.h"
#include "GLSL.h"
#include "Program.h"
#include "WindowManager.h"
#include "Shape.h"
#include "skmesh.h"
#include <string.h>
#include "line.h"

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// assimp
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "assimp/vector3.h"
#include "assimp/scene.h"
#include <assimp/mesh.h>

// irrKlang
#include <irrKlang.h>

// freetype
#include <ft2build.h>
#include FT_FREETYPE_H  

using namespace std;
using namespace glm;
using namespace Assimp;
using namespace irrklang;

ISoundEngine* soundEngine;
std::string resourceDir;

unsigned int VAO, VBO;

struct Character {
	unsigned int TextureID;  // ID handle of the glyph texture
	glm::ivec2   Size;       // Size of glyph
	glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
	long Advance;    // Offset to advance to next glyph
};

std::map<char, Character> Characters;

double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime = glfwGetTime();
	double difference = actualtime - lasttime;
	lasttime = actualtime;
	return difference;
}



class camera
{
public:
	glm::vec3 pos, rot;

	int w, a, s, d, q, e, z, c;
	glm::mat4 R;
	camera()
	{
		w = a = s = d = q = e = z = c = 0;
		pos = glm::vec3(0.232399,-21.4059,-31.5946);
		rot = glm::vec3(.7, 0, 0);
	}

	glm::mat4 process(double ftime)
	{
		float speed = 0;

        float fwdspeed = 20;
        /*if (realspeed)
            fwdspeed = 8;*/

        if (w == 1)
        {
            speed = fwdspeed*ftime;
        }
        else if (s == 1)
        {
            speed = -fwdspeed*ftime;
        }
        float yangle=0;
        if (a == 1)
            yangle = -1*ftime;
        else if(d==1)
            yangle = 1*ftime;
        rot.y += yangle;
        float zangle = 0;
        if (q == 1)
            zangle = -1 * ftime;
        else if (e == 1)
            zangle = 1 * ftime;
        rot.z += zangle;
        float xangle = 0;
        if (z == 1)
            xangle = -0.1 * ftime;
        else if (c == 1)
            xangle = 0.1 * ftime;
        rot.x += xangle;

        glm::mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
        glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
        glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
        glm::vec4 dir = glm::vec4(0, 0, speed, 1);
        R = Rz * Rx  * Ry;
        dir = dir*R;
        pos += glm::vec3(dir.x, dir.y, dir.z);
        glm::mat4 T = glm::translate(glm::mat4(1), pos);
        return R*T;
    }
    
    void get_dirpos(vec3 &up,vec3 &dir,vec3 &position)
    {
		position = pos;
		glm::mat4 Ry = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::mat4 Rz = glm::rotate(glm::mat4(1), rot.z, glm::vec3(0, 0, 1));
		glm::mat4 Rx = glm::rotate(glm::mat4(1), rot.x, glm::vec3(1, 0, 0));
		glm::vec4 dir4 = glm::vec4(0, 0, 1, 0);
		//R = Rz * Rx  * Ry;
		dir4 = dir4*R;
		dir = vec3(dir4);
		glm::vec4 up4 = glm::vec4(0, 1, 0, 0);
		up4 = R*vec4(0, 1, 0, 0);
		up4 = vec4(0, 1, 0, 0)*R;
		up = vec3(up4);
    }   
};

camera mycam;

class Application : public EventCallbacks
{
public:
	WindowManager* windowManager = nullptr;
	double global_timer = 0.0;
	int global_health = 100;
	glm::vec3 runner_pos = vec3(0, -1, -3);
	glm::vec3 runner_dir = vec3(1, 0, 0);
	float runner_rot = -3.1415926f / 2.0f;
	vector<vec3> line;
	vector<vec3> line2;
	vector<vec3> line3;
	vector<vec3> smoothline;
	vector<vec3> smoothline2;
	vector<vec3> smoothline3;
	int current = 0;
	int next = 1;
	vec3 interpolate = vec3(0, -1, -3);
	vec3 interpolate2 = vec3(0, -1, -3);
	vec3 interpolate3 = vec3(0, -1, -3);
	vec3 a;
	vec3 b;
	int shrink = 2;
	int upbound = -20;
	int leftbound = -15;
	int downbound = 20;
	int rightbound = 17;
	int framecount = 0;
	int monster_speed = 5;
	
	vector<mat4> Marr;
	

	// Our shader program
	std::shared_ptr<Program> psky, skinProg, textProg, plantsProg, monsterProg;

	// skinnedMesh
	SkinnedMesh skmesh;
	SkinnedMesh skmonster;

	// textures
	shared_ptr<SmartTexture> skyTex, shrubTex, groundTex, treeTex;

	// shapes
	shared_ptr<Shape> skyShape, cube, shrub, tree, monster;
	vector<vec3> shrub_positions, tree_positions;

	void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_W && (action == GLFW_PRESS ||action == GLFW_REPEAT) )
		{
			runner_dir = vec3(0, 0, -3);
			if(runner_pos.z > upbound){
				runner_pos += runner_dir;
			}
			runner_rot = 3.1415926f / 2.0f;
		}
		if (key == GLFW_KEY_S && (action == GLFW_PRESS ||action == GLFW_REPEAT))
		{
			runner_dir = vec3(0, 0, 3);
			if(runner_pos.z < downbound){
				runner_pos += runner_dir;
			}
			runner_rot = -3.1415926f / 2.0f;
		}
		if (key == GLFW_KEY_A && (action == GLFW_PRESS ||action == GLFW_REPEAT))
		{
			runner_dir = vec3(-3, 0, 0);
			if(runner_pos.x > leftbound){
				runner_pos += runner_dir;
			}
			runner_rot = 3.1415926f;
		}
		if (key == GLFW_KEY_D && (action == GLFW_PRESS ||action == GLFW_REPEAT))
		{
			runner_dir = vec3(3, 0, 0);
			if(runner_pos.x < rightbound){
				runner_pos += runner_dir;
			}
			runner_rot = -3.1415926f;
		}

		if (key == GLFW_KEY_I && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		else if (key == GLFW_KEY_I && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_K && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		else if (key == GLFW_KEY_K && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_J && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		else if (key == GLFW_KEY_J && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_L && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		else if (key == GLFW_KEY_L && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
        {
            mycam.q = 1;
        }
        if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
        {
            mycam.q = 0;
        }
        if (key == GLFW_KEY_E && action == GLFW_PRESS)
        {
            mycam.e = 1;
        }
        if (key == GLFW_KEY_E && action == GLFW_RELEASE)
        {
            mycam.e = 0;
        }
        if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            mycam.z = 1;
        }
        if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            mycam.z = 0;
        }
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            mycam.c = 1;
        }
        if (key == GLFW_KEY_C && action == GLFW_RELEASE)
        {
            mycam.c = 0;
        }

		//if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		//{
			//mycam.pos = vec3(mycam.pos.x, mycam.pos.y-0.1, mycam.pos.z);
			//skmesh.print_animations(0);
		//}
		if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS)
		{
			vec3 dir, pos, up;
			mycam.get_dirpos(up, dir, pos);
			cout << endl;
			cout << "point position:" << pos.x << "," << pos.y << "," << pos.z << endl;
			cout << "Zbase:" << dir.x << "," << dir.y << "," << dir.z << endl;
			cout << "Ybase:" << up.x << "," << up.y << "," << up.z << endl;
		}
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow* window, int button, int action, int mods)
	{

	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow* window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}
	/*Note that any gl calls must always happen after a GL state is initialized */

	void initChars(FT_Face &face)
	{
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

		for (unsigned char c = 0; c < 128; c++)
		{
			// load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
				continue;
			}

			// generate texture
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);

			// set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// now store character for later use
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				face->glyph->advance.x
			};

			Characters.insert(std::pair<char, Character>(c, character));
		}
	}

	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom(const std::string& resourceDirectory)
	{
		if (!skmesh.LoadMesh(resourceDirectory + "/Ninja.fbx")) {
			printf("Mesh load failed\n");
			return;
		}

		if (!skmonster.LoadMesh(resourceDirectory + "/Golem.fbx")) {
			printf("Mesh load failed\n");
			return;
		}

		// Initialize mesh.
		skyShape = make_shared<Shape>();
		skyShape->loadMesh(resourceDirectory + "/sphere.obj");
		skyShape->resize();
		skyShape->init();

		// ground
		cube = make_shared<Shape>();
        cube->loadMesh(resourceDirectory + "/cube.obj");
        cube->resize();
        cube->init();

		auto strGround = resourceDirectory + "/ground.jpeg";
        groundTex = SmartTexture::loadTexture(strGround, true);
        if (!groundTex)
            cerr << "error: texture " << strGround << " not found" << endl;

		// tree
		tree = make_shared<Shape>();
        tree->loadMesh(resourceDirectory + "/tree.obj");
        tree->resize();
        tree->init();
		auto strTree = resourceDirectory + "/treetexture.jpg";
        treeTex = SmartTexture::loadTexture(strTree, true);
        if (!treeTex)
            cerr << "error: texture " << strTree << " not found" << endl;
		
        

		// shrub
		shrub = make_shared<Shape>();
        shrub->loadMesh(resourceDirectory + "/shrub.obj");
        shrub->resize();
        shrub->init();
        

		// sky texture
		auto strSky = resourceDirectory + "/sky.jpg";
		skyTex = SmartTexture::loadTexture(strSky, true);
		if (!skyTex)
			cerr << "error: texture " << strSky << " not found" << endl;

		//texture 3
		auto strHeight = resourceDirectory + "/height.jpg";
		skyTex = SmartTexture::loadTexture(strSky, true);
		if (!skyTex)
			cerr << "error: texture " << strSky << " not found" << endl;



		// shrub positions
        for (int i = 0; i < 40; i++) {
            int randX = (rand() % 50) + -25;
            int randZ = (rand() % 50) + -25;
            shrub_positions.push_back(vec3(randX, -.5, randZ));
        }

		// tree positions
        for (int i = 0; i < 60; i++) {
            int randX = (rand() % 50) + -25;
            int randZ = (rand() % 50) + -25;
            tree_positions.push_back(vec3(randX, 1, randZ));
        }

		line.push_back(vec3((rand() % 50) + -25,-1,(rand() % 50) + -25));
		line2.push_back(vec3((rand() % 50) + -25,-1,(rand() % 50) + -25));
		line3.push_back(vec3((rand() % 50) + -25,-1,(rand() % 50) + -25));
	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);
		//glDisable(GL_DEPTH_TEST);
		// Initialize the GLSL program.
		psky = std::make_shared<Program>();
		psky->setVerbose(true);
		psky->setShaderNames(resourceDirectory + "/skyvertex.glsl", resourceDirectory + "/skyfrag.glsl");
		if (!psky->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		psky->addUniform("P");
		psky->addUniform("V");
		psky->addUniform("M");
		psky->addUniform("tex");
		psky->addUniform("camPos");
		psky->addAttribute("vertPos");
		psky->addAttribute("vertNor");
		psky->addAttribute("vertTex");

		skinProg = std::make_shared<Program>();
		skinProg->setVerbose(true);
		skinProg->setShaderNames(resourceDirectory + "/skinning_vert.glsl", resourceDirectory + "/skinning_frag.glsl");
		if (!skinProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		skinProg->addUniform("P");
		skinProg->addUniform("V");
		skinProg->addUniform("M");
		skinProg->addUniform("tex");
		skinProg->addUniform("camPos");
		skinProg->addAttribute("vertPos");
		skinProg->addAttribute("vertNor");
		skinProg->addAttribute("vertTex");
		skinProg->addAttribute("BoneIDs");
		skinProg->addAttribute("Weights");

		textProg = std::make_shared<Program>();
		textProg->setVerbose(true);
		textProg->setShaderNames(resourceDirectory + "/text_vert.glsl", resourceDirectory + "/text_frag.glsl");
		if (!textProg->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}

		textProg->addUniform("projection");
		textProg->addUniform("text");
		textProg->addUniform("textColor");
		textProg->addAttribute("vertex");

		glm::mat4 projection = glm::ortho(0.0f, 1920.0f, 0.0f, 1080.0f);
		textProg->bind();
		glUniformMatrix4fv(glGetUniformLocation(textProg->pid, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		textProg->unbind();

		plantsProg = std::make_shared<Program>();
        plantsProg->setVerbose(true);
        plantsProg->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
        if (!plantsProg->init())
        {
            std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
            exit(1);
        }
        plantsProg->addUniform("P");
        plantsProg->addUniform("V");
        plantsProg->addUniform("M");
        plantsProg->addUniform("tex");
        plantsProg->addUniform("camPos");
        plantsProg->addAttribute("vertPos");
        plantsProg->addAttribute("vertNor");
        plantsProg->addAttribute("vertTex");
	}

	void renderText(std::shared_ptr<Program> prog, std::string text, float x, float y, float scale, glm::vec3 color)
	{
		// activate corresponding render state	
		prog->bind();
		glUniform3f(glGetUniformLocation(prog->pid, "textColor"), color.x, color.y, color.z);
		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);

		// iterate through all characters
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); c++)
		{
			Character ch = Characters[*c];

			float xpos = x + ch.Bearing.x * scale;
			float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

			float w = ch.Size.x * scale;
			float h = ch.Size.y * scale;
			// update VBO for each character
			float vertices[6][4] = {
				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos,     ypos,       0.0f, 1.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },

				{ xpos,     ypos + h,   0.0f, 0.0f },
				{ xpos + w, ypos,       1.0f, 1.0f },
				{ xpos + w, ypos + h,   1.0f, 0.0f }
			};
			// render glyph texture over quad
			glBindTexture(GL_TEXTURE_2D, ch.TextureID);
			// update content of VBO memory
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			// render quad
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
			x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);
		prog->unbind();
	}

	void spline(vector<vec3> &result_path, vector<vec3> &original_path,  int lod, float curly)
	{
		
		if (original_path.size()<3) return;
		result_path.clear();
		int n1 = original_path.size() + 1;
		vec3 *P, *d, *A, first, last;
		vec4 *B;

		double *Bi = NULL;
		P = new vec3[n1];
		d = new vec3[n1];
		A = new vec3[n1];
		B = new vec4[lod];

		//punkte setzen
		for (int ii = 0; ii<original_path.size(); ii++)
			P[ii] = original_path[ii];



		d[0] = vec3(0, 0, 0);
		d[original_path.size() - 1] = vec3(0, 0, 0);

		Bi = new double[n1];
		double t = 0;
		double tt = 1. / (lod - 1.);
		for (int i = 0; i< lod; i++)
		{
			double t1 = 1 - t, t12 = t1*t1, t2 = t*t;
			B[i] = vec4(t1*t12, 3 * t*t12, 3 * t2*t1, t*t2);
			t += tt;
		}

		//findpoints
		Bi[1] = -0.25*(4. / curly);

		A[1].x = (P[2].x - P[0].x - d[0].x) / 4;
		A[1].y = (P[2].y - P[0].y - d[0].y) / 4;
		A[1].z = (P[2].z - P[0].z - d[0].z) / 4;
		for (int i = 2; i < original_path.size() - 1; i++)
		{
			Bi[i] = -1 / (4 + Bi[i - 1]);
			A[i].x = -(P[i + 1].x - P[i - 1].x - A[i - 1].x)*Bi[i];
			A[i].y = -(P[i + 1].y - P[i - 1].y - A[i - 1].y)*Bi[i];
			A[i].z = -(P[i + 1].z - P[i - 1].z - A[i - 1].z)*Bi[i];
		}
		for (int i = original_path.size() - 2; i > 0; i--)
		{
			d[i].x = A[i].x + d[i + 1].x*Bi[i];
			d[i].y = A[i].y + d[i + 1].y*Bi[i];
			d[i].z = A[i].z + d[i + 1].z*Bi[i];
		}
		//points

		float X, Y, Z;
		float Xo = (float)P[0].x;
		float Yo = (float)P[0].y;
		float Zo = (float)P[0].z;



		result_path.push_back(vec3(Xo, Yo, 0));
		
		int ii = 0;
		for (int i = 0; i < original_path.size() - 1; i++)
		{
			for (int k = 0; k < lod; k++)
			{
				X = (P[i].x						*	B[k].x
					+ (P[i].x + d[i].x)	*	B[k].y
					+ (P[i + 1].x - d[i + 1].x)	*	B[k].z
					+ P[i + 1].x					*	B[k].w);

				Y = (P[i].y						*	B[k].x
					+ (P[i].y + d[i].y)		*	B[k].y
					+ (P[i + 1].y - d[i + 1].y)	*	B[k].z
					+ P[i + 1].y					*	B[k].w);

				Z = (P[i].z						*	B[k].x
					+ (P[i].z + d[i].z)		*	B[k].y
					+ (P[i + 1].z - d[i + 1].z)	*	B[k].z
					+ P[i + 1].z					*	B[k].w);

				if (i == 0 && k == 0)	
					result_path.push_back(vec3(X, Y, Z));
				if (k>0)				
					result_path.push_back(vec3(X, Y, Z));
				
			}
		}


		if (P != NULL)	delete[] P;
		if (d != NULL)	delete[] d;
		if (A != NULL)	delete[] A;
		if (B != NULL)	delete[] B;
		if (Bi)			delete[] Bi;

	}

	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		double frametime = get_last_elapsed_time();
		static double totaltime = 0;
		totaltime += frametime;
		

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width / (float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now

		glm::mat4 V, M, P; //View, Model and Perspective matrix
		V = mycam.process(frametime);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);
		if (width < height)
		{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect, 1.0f / aspect, -2.0f, 100.0f);
		}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width / (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones
		auto sangle = -3.1415926f / 2.0f;
		glm::mat4 RotateXSky = glm::rotate(glm::mat4(1.0f), sangle, glm::vec3(1.0f, 0.0f, 0.0f));
		glm::vec3 camp = -mycam.pos;
		glm::mat4 TransSky = glm::translate(glm::mat4(1.0f), camp);
		glm::mat4 SSky = glm::scale(glm::mat4(1.0f), glm::vec3(0.8f, 0.8f, 0.8f));

		M = TransSky * RotateXSky * SSky;

		// Draw the sky using GLSL.
		psky->bind();
		GLuint texLoc = glGetUniformLocation(psky->pid, "tex");
		skyTex->bind(texLoc);
		glUniformMatrix4fv(psky->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(psky->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(psky->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniform3fv(psky->getUniform("camPos"), 1, &mycam.pos[0]);
		glm::mat4 down = glm::translate(glm::mat4(1.0f), vec3(0, -.75, 0));
		glm::mat4 Trans;
		glm::mat4 Scale;
		glm::mat4 RotX;

		glDisable(GL_DEPTH_TEST);
		skyShape->draw(psky, false);
		glEnable(GL_DEPTH_TEST);
		skyTex->unbind();
		psky->unbind();

		// Shrubs
		texLoc = glGetUniformLocation(plantsProg->pid, "tex");
        treeTex->bind(texLoc);
        for (int i = 0; i < 40; i++) {
            plantsProg->bind();
            Trans = glm::translate(glm::mat4(1.0f), shrub_positions[i]);
            Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.25f, 0.25f, 0.25f));
            M = down * Trans * Scale; // T R S
            
            glUniform3fv(plantsProg->getUniform("camPos"), 1, &mycam.pos[0]);
            glUniformMatrix4fv(plantsProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
            glUniformMatrix4fv(plantsProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
            glUniformMatrix4fv(plantsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            shrub->draw(plantsProg, false);
            plantsProg->unbind();
        }

		// Trees
        for (int i = 0; i < 60; i++) {
            plantsProg->bind();
			
            Trans = glm::translate(glm::mat4(1.0f), tree_positions[i]);
            Scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
            M = down * Trans * Scale; // T R S
            
            glUniform3fv(plantsProg->getUniform("camPos"), 1, &mycam.pos[0]);
            glUniformMatrix4fv(plantsProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
            glUniformMatrix4fv(plantsProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
            glUniformMatrix4fv(plantsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
            tree->draw(plantsProg, false);
			
            plantsProg->unbind();
        }
		treeTex->unbind();
        
        
        // // draw ground
        plantsProg->bind();
        texLoc = glGetUniformLocation(plantsProg->pid, "tex");
        groundTex->bind(texLoc);
        Trans = glm::translate(glm::mat4(1.0f), vec3(0, -1, 0));
        Scale = glm::scale(glm::mat4(1.0f), glm::vec3(50.f, 0.5f, 50.f));
        M = down * Trans * Scale; // T R S
        
        glUniform3fv(plantsProg->getUniform("camPos"), 1, &mycam.pos[0]);
        glUniformMatrix4fv(plantsProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
        glUniformMatrix4fv(plantsProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
        glUniformMatrix4fv(plantsProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
        cube->draw(plantsProg, false);
        groundTex->unbind();
        plantsProg->unbind();

		
		// draw the skinned mesh
		skinProg->bind();
		texLoc = glGetUniformLocation(skinProg->pid, "tex");
		sangle = -3.1415926f / 2.0f;
		Trans = glm::translate(glm::mat4(1.0f), runner_pos);
		RotX = glm::rotate(glm::mat4(1.0f), runner_rot, vec3(0, 1, 0));
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.06f, 0.06f));
		M = Trans * Scale;

		glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skmesh.setBoneTransformations(skinProg->pid, totaltime * 6.);
		skmesh.Render(texLoc);



		// monsters

		if(next >= smoothline.size()){
			line.clear();
			line2.clear();
			line3.clear();
			smoothline.clear();
			smoothline2.clear();
			smoothline3.clear();
			rightbound -= shrink;
			upbound += shrink;
			downbound -= shrink;
			leftbound += shrink;
			
			current = 0;
			next = 1;
			if(monster_speed > 1){
				monster_speed -= 1;
			}
			
			for(int i = 0; i < 10; i++){
				line.push_back(vec3((rand() % 50) + -25,-1,(rand() % 50) + -25));
				line2.push_back(vec3((rand() % 50) + -25,-1,(rand() % 50) + -25));
				line3.push_back(vec3((rand() % 50) + -25,-1,(rand() % 50) + -25));
			}
			spline(smoothline, line, 30, 5.0);
			spline(smoothline2, line2, 30, 5.0);
			spline(smoothline3, line3, 30, 5.0);
		}

		
		static float totalTime = 0;
		totalTime += frametime * 5;
		int intTime = (int)totalTime;
		float deltaTime = totalTime - intTime;
		// Draw the plane using GLSL.
		if (framecount % monster_speed == 0){
			// std::cout << deltaTime << std::endl;
			interpolate = smoothline[current] * deltaTime + ((1.f - deltaTime) * smoothline[next]);
			interpolate2 = smoothline2[current] * deltaTime + ((1.f - deltaTime) * smoothline2[next]);
			interpolate3 = smoothline3[current] * deltaTime + ((1.f - deltaTime) * smoothline3[next]);
			current++;
			next++;
			check_collision();
		}
		framecount++;
		texLoc = glGetUniformLocation(skinProg->pid, "tex");
		Scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.06f, 0.06f, 0.06f));
		glm::mat4 TransMonster = glm::translate(glm::mat4(1.0f), interpolate);
		glm::mat4 FaceDirection = glm::mat4(1.0f);
		glm::vec3 Z = normalize(smoothline[next] - interpolate) * 1.f;
		glm::vec3 Y = vec3(0,1,0);
		glm::vec3 X = normalize(cross(Z, Y));
		FaceDirection[0][0] = X.x;
		FaceDirection[0][1] = X.y;
		FaceDirection[0][2] = X.z;
		FaceDirection[0][3] = 0;
		FaceDirection[1][0] = Y.x;
		FaceDirection[1][1] = Y.y;
		FaceDirection[1][2] = Y.z;
		FaceDirection[1][3] = 0;
		FaceDirection[2][0] = Z.x;
		FaceDirection[2][1] = Z.y;
		FaceDirection[2][2] = Z.z;
		FaceDirection[2][3] = 0;
		FaceDirection[3][0] = 0;
		FaceDirection[3][1] = 0;
		FaceDirection[3][2] = 0;
		FaceDirection[3][3] = 1.0f;

		M = TransMonster*FaceDirection*Scale;
		glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skmonster.setBoneTransformations(skinProg->pid, totaltime * 6.);
		skmonster.Render(texLoc);


		glm::mat4 TransMonster2 = glm::translate(glm::mat4(1.0f), interpolate2);
		FaceDirection = glm::mat4(1.0f);
		Z = normalize(smoothline2[next] - interpolate2) * 1.f;
		Y = vec3(0,1,0);
		X = normalize(cross(Z, Y));
		FaceDirection[0][0] = X.x;
		FaceDirection[0][1] = X.y;
		FaceDirection[0][2] = X.z;
		FaceDirection[0][3] = 0;
		FaceDirection[1][0] = Y.x;
		FaceDirection[1][1] = Y.y;
		FaceDirection[1][2] = Y.z;
		FaceDirection[1][3] = 0;
		FaceDirection[2][0] = Z.x;
		FaceDirection[2][1] = Z.y;
		FaceDirection[2][2] = Z.z;
		FaceDirection[2][3] = 0;
		FaceDirection[3][0] = 0;
		FaceDirection[3][1] = 0;
		FaceDirection[3][2] = 0;
		FaceDirection[3][3] = 1.0f;

		M = TransMonster2*FaceDirection*Scale;
		glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skmonster.setBoneTransformations(skinProg->pid, totaltime * 6.);
		skmonster.Render(texLoc);

		glm::mat4 TransMonster3 = glm::translate(glm::mat4(1.0f), interpolate3);
		FaceDirection = glm::mat4(1.0f);
		Z = normalize(smoothline3[next] - interpolate3) * 1.f;
		Y = vec3(0,1,0);
		X = normalize(cross(Z, Y));
		FaceDirection[0][0] = X.x;
		FaceDirection[0][1] = X.y;
		FaceDirection[0][2] = X.z;
		FaceDirection[0][3] = 0;
		FaceDirection[1][0] = Y.x;
		FaceDirection[1][1] = Y.y;
		FaceDirection[1][2] = Y.z;
		FaceDirection[1][3] = 0;
		FaceDirection[2][0] = Z.x;
		FaceDirection[2][1] = Z.y;
		FaceDirection[2][2] = Z.z;
		FaceDirection[2][3] = 0;
		FaceDirection[3][0] = 0;
		FaceDirection[3][1] = 0;
		FaceDirection[3][2] = 0;
		FaceDirection[3][3] = 1.0f;

		M = TransMonster3*FaceDirection*Scale;
		glUniform3fv(skinProg->getUniform("camPos"), 1, &mycam.pos[0]);
		glUniformMatrix4fv(skinProg->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniformMatrix4fv(skinProg->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		skmonster.setBoneTransformations(skinProg->pid, totaltime * 6.);
		skmonster.Render(texLoc);


		skinProg->unbind();
		
		
		global_timer += frametime;
		renderText(textProg, ("Time: " + to_string(global_timer)), 0.0f, 1000.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
		renderText(textProg, ("Health: " + to_string(global_health)), 0.0f, 900.0f, 1.0f, glm::vec3(0.8, 0.1f, 0.2f));
	}
	void gameover()
	{
		cout << "\n\n-------- Game Over --------\n\n" << endl;
		cout << "Total Time Survived: " << to_string(global_timer) << "\n" << endl;
	}
	void check_collision(){
		if(glm::distance(runner_pos, smoothline[current]) < 2.0f || glm::distance(runner_pos, smoothline2[current]) < 2.0f || glm::distance(runner_pos, smoothline3[current]) < 2.0f){
			global_health -= 5;
		}
	}
	bool check_gameover(){
		if(global_health <= 0){
			return true;
		}
		return false;
	}
};

//******************************************************************************************
int main(int argc, char** argv)
{
	resourceDir = "../resources"; // Where the resources are loaded from
	std::string missingTexture = "missing.jpg";

	SkinnedMesh::setResourceDir(resourceDir);
	SkinnedMesh::setDefaultTexture(missingTexture);

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	soundEngine = createIrrKlangDevice();

	if (!soundEngine)
	{
		cerr << "error starting irrklang audio engine" << endl;
		return 0; // error starting up the engine
	}
	
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return -1;
	}

	string fontPath = resourceDir + "/fonts/arial.ttf";
	FT_Face face;
	if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
	{
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return -1;
	}

	FT_Set_Pixel_Sizes(face, 0, 48);

	Application* application = new Application();

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager* windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;
	string s = resourceDir + "/chase.mp3";
	soundEngine->play2D(s.c_str(), true);

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
		// Initialize scene.
	application->init(resourceDir);
	application->initGeom(resourceDir);
	application->initChars(face);

	// configure VAO/VBO for texture quads
	// -----------------------------------
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	
	// Loop until the user closes the window.
	while (!glfwWindowShouldClose(windowManager->getHandle()))
	{
		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
		if(application->check_gameover() == true){
			break;
		}
	}


	//
	// Quit program.
	application->gameover();
	windowManager->shutdown();
	return 0;
}

