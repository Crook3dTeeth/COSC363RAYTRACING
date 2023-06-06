/*==================================================================================
* COSC 363  Computer Graphics (2023)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab06.pdf   for details.
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <iostream>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "SceneObject.h"
#include "Ray.h"
#include "TextureBMP.h"
#include "Plane.h"
#include <GL/freeglut.h>

#include <thread>

using namespace std;

const float EDIST = 40.0;
const int NUMDIV = 1000;
const int MAX_STEPS = 50;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;

// Extra rendering features
const int AAType = 1;		// 0 = off, 1 = basic, 2 = Depth of field
const float DDIST = 80;		// Depth of field distance
const bool FOG = false;		// Fog rendering
const float fogZ1 = -40.0;
const float fogZ2 = -130.0;

vector<SceneObject*> sceneObjects;
TextureBMP texture;

//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);						//Background colour = (0,0,0)
	glm::vec3 lightPos(2, 10, -5);					//Light's position
	glm::vec3 lightPos2(-2, 10, -5);					//Light's position
	glm::vec3 color(0);
	glm::vec3 refrColor(0);							// refraction color
	SceneObject* obj;

    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found

	color = obj->lighting(lightPos, lightPos2, ray.dir, ray.hit);


	//chequered pattern on the floor plane
	if (ray.index == 1)
	{
		int stripeWidth = 5;
		int iz = (ray.hit.z) / stripeWidth;
		int ix = (ray.hit.x) / stripeWidth;
		

		int k = iz % 2; //2 colors
		int b = ix % 2;

		if (ray.hit.x > 0) {
			if (k == 0)
			{
				if (b == 0) {
					color = glm::vec3(1, 1, 1);
				}
				else {
					color = glm::vec3(0, 0, 0);
				}
			}
			else
			{
				if (b == 0) {
					color = glm::vec3(0, 0, 0);
				}
				else {
					color = glm::vec3(1, 1, 1);
				}
			}
		}
		else {
			if (k == 0)
			{
				if (b == 0) {
					color = glm::vec3(0, 0, 0);
				}
				else {
					color = glm::vec3(1, 1, 1);
				}
			}
			else
			{
				if (b == 0) {
					color = glm::vec3(1, 1, 1);
				}
				else {
					color = glm::vec3(0, 0, 0);
				}
			}
		}
		obj->setColor(color);
	}
	else if (ray.index == 2) { // textured sphere
		glm::vec3 vec = glm::normalize(ray.hit - glm::vec3(-5, 0.0, -40));

		float s = 0.5 + (atan2(vec.z, vec.x)) / (2 * 3.1415);
		float t = 0.5 + (asin(vec.y)) / (2 * 3.1415);

		color = texture.getColorAt(s, t);
		obj->setColor(color);
	}


	//Object's colour
	// light source 1
	glm::vec3 lightVec = lightPos - ray.hit;
	Ray shadowRay(ray.hit, lightVec);
	shadowRay.closestPt(sceneObjects);
	if (shadowRay.index > -1 && shadowRay.dist < glm::length(lightVec)) {
		if (sceneObjects[shadowRay.index]->isTransparent()) {
			float transparencyCoeff = sceneObjects[shadowRay.index]->getTransparencyCoeff() * 0.5;
			color *= ((transparencyCoeff) * obj->getColor());
		} else {
			color *= 0.1f * obj->getColor();
		}
	}

	// Light source 2
	glm::vec3 lightVec2 = lightPos2 - ray.hit;
	Ray shadowRay2(ray.hit, lightVec2);
	shadowRay2.closestPt(sceneObjects);
	if (shadowRay2.index > -1 && shadowRay2.dist < glm::length(lightVec2)) {
		if (sceneObjects[shadowRay2.index]->isTransparent()) {
			float transparencyCoeff = sceneObjects[shadowRay2.index]->getTransparencyCoeff() * 0.5;
			color *= ((transparencyCoeff)*obj->getColor());
		}
		else {
			color *= 0.1f * obj->getColor();
		}
	}
	

	// Reflective surfaces
	if (obj->isReflective() && step < MAX_STEPS)
	{
		float rho = obj->getReflectionCoeff(); // for blending colors

		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
		Ray reflectedRay(ray.hit, reflectedDir);
		glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
		color = color + (rho * reflectedColor);
	}

	// transparency and refractivity
	if (obj->isTransparent() && step < MAX_STEPS) {
		if (obj->isRefractive()) {
			// refractive sphere
			if (ray.index == 0) {
				float eta = (1 / (obj->getRefractiveIndex()));

				glm::vec3 n = obj->normal(ray.hit);
				glm::vec3 g = glm::refract(ray.dir, n, eta);

				Ray refrRay(ray.hit, g);
				refrRay.closestPt(sceneObjects);

				glm::vec3 m = obj->normal(refrRay.hit);
				glm::vec3 h = glm::refract(g, -m, 1.0f / eta);

				Ray refrRay2(refrRay.hit, h);
				refrRay2.closestPt(sceneObjects);

				if (refrRay2.index == -1) return backgroundCol;
				glm::vec3 refColor2 = trace(refrRay2, step + 1);
				
				refrColor += refColor2 * (1 - 0.2f);
				return refrColor + (obj->getColor() * (1-obj->getTransparencyCoeff()));
			}
		}
	}
	color += refrColor;

	//std::cout << ray.hit.z << std::endl;

	// calculates fog
	if (FOG) {
		glm::vec3 fogCol(0.5, 0.5, 0.5);
		if (ray.hit.z < fogZ1 && ray.hit.z >= fogZ2) {
			float lamda = (ray.hit.z - fogZ1) / (fogZ2 - fogZ1);
			color = ((1 - lamda) * color) + (lamda * fogCol);
		}
		else if(ray.hit.z>fogZ1) {
			float lamda = 0;
			color = ((1 - lamda) * color) + (lamda * fogCol);
		}
		else if (ray.hit.z < fogZ2) {
			float lamda = 1;
			color = ((1 - lamda) * color) + (lamda * fogCol);
		}
	}



	return color;
}

//--- Basic Anti Aliasing ---------------------------------------------------------------
// Performs basic anit aliasing on the scene
// Splits each pixel into 4 sub pixels and calculates the average of the
// 4 colors
//---------------------------------------------------------------------------------------
glm::vec3 basicAA(Ray ray, float xp, float yp, float cellXSub, float cellYSub, glm::vec3 eye)
{
	float x1 = xp;
	float y1 = yp;

	float x2 = xp + cellXSub;
	float y2 = yp;

	float x3 = xp + cellXSub;
	float y3 = yp + cellYSub;

	float x4 = xp;
	float y4 = yp + cellYSub;

	glm::vec3 col1 = trace(Ray(eye, glm::vec3(x1 + cellXSub, y1 + cellYSub, -EDIST)), 1);
	glm::vec3 col2 = trace(Ray(eye, glm::vec3(x2 + cellXSub, y2 + cellYSub, -EDIST)), 1);
	glm::vec3 col3 = trace(Ray(eye, glm::vec3(x3 + cellXSub, y3 + cellYSub, -EDIST)), 1);
	glm::vec3 col4 = trace(Ray(eye, glm::vec3(x4 + cellXSub, y4 + cellYSub, -EDIST)), 1);


	return (col1 + col2 + col3 + col4) / 4.0f;
}


//--- Depth of Field --------------------------------------------------------------------
// Creates a depth of field effect
//---------------------------------------------------------------------------------------
glm::vec3 depthOfField(Ray ray, glm::vec3 eye)
{
	glm::vec3 colorSum(0);
	float rayCount = 10.0f; // Number of rays to be sent out per pixel
	const float convergeDistance = 0.1f;

	for (int i = 0; i < rayCount; i++) {
		// generate random ray origin offset
		float theta = (rand() / float(RAND_MAX)) * 2.0f * 3.1415f;
		float x = convergeDistance * cos(theta);
		float y = convergeDistance * sin(theta);

		glm::vec3 offset(x, y, 0);
		glm::vec3 target = eye + (ray.dir * DDIST);
		glm::vec3 eyeOffset = eye + offset;
		
		Ray eyeRay = Ray(eyeOffset, glm::normalize(target-eyeOffset));
		// Add the color of the new ray
		colorSum += trace(eyeRay, 1);
	}

	return (colorSum / rayCount);
}


//---The main display module ------------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX - XMIN) / NUMDIV;  //cell width
	float cellY = (YMAX - YMIN) / NUMDIV;  //cell height

	float cellXSub = cellX / 2.0f;
	float cellYSub = cellY / 2.0f;

	glm::vec3 eye(0., 0., 0.);

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for (int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i * cellX;
		for (int j = 0; j < NUMDIV; j++)
		{

			yp = YMIN + j * cellY;

			glm::vec3 dir(xp + 0.5 * cellX, yp + 0.5 * cellY, -EDIST);	//direction of the primary ray
			Ray ray = Ray(eye, dir);
			glm::vec3 col(0);

			switch (AAType)
			{
				case 0:
					col = trace(ray, 1); //Trace the primary ray and get the colour value
					break;
				case 1:
					col = basicAA(ray, xp, yp, cellXSub, cellYSub, eye);
					break;
				case 2:
					col = depthOfField(ray, eye);
					break;

				default:
					col = trace(ray, 1); //Trace the primary ray and get the colour value
					break;
			}


			glColor3f(col.r, col.g, col.b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp + cellX, yp);
			glVertex2f(xp + cellX, yp + cellY);
			glVertex2f(xp, yp + cellY);
		}
	}

    glEnd();
    glFlush();
}



//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL 2D orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);

    glClearColor(0, 0, 0, 1);

	
	Sphere* refractiveSphere = new Sphere(glm::vec3(5, 0.0, -EDIST), 2.0);

	refractiveSphere->setColor(glm::vec3(0, 0, 1));   //Set colour to blue
	refractiveSphere->setTransparency(true, 1);
	refractiveSphere->setRefractivity(true, 1.5, 1.01);


	// Half width
	float width = 30.0;
	float height = 20.0;
	float frontDepth = -130;
	float backDepth = 10;
	// Coords for the 6 planes
	glm::vec3 frontBottomLeft(-width, -15, frontDepth);
	glm::vec3 frontBottomRight(width, -15, frontDepth);
	glm::vec3 backBottomRight(width, -15,  backDepth);
	glm::vec3 backBottomLeft(-width, -15,  backDepth);

	glm::vec3 frontTopLeft(-width, height, frontDepth);
	glm::vec3 frontTopRight(width, height, frontDepth);
	glm::vec3 backTopLeft(-width,  height, backDepth);
	glm::vec3 backTopRight(width,  height, backDepth);


	#pragma region walls.floors.roof

	// Chequered floor plane
	Plane* chequeredFloor = new Plane(
		frontBottomLeft,
		backBottomLeft,
		backBottomRight,
		frontBottomRight);
	chequeredFloor->setSpecularity(false);

	Plane* frontWall = new Plane(
		frontBottomRight,
		frontTopRight,
		frontTopLeft,
		frontBottomLeft
	);
	frontWall->setColor(glm::vec3(0, 0, 1)); // blue

	Plane* leftWall = new Plane(
		frontBottomLeft,
		frontTopLeft,
		backTopLeft,
		backBottomLeft
	);
	leftWall->setColor(glm::vec3(0, 1, 0)); // green

	Plane* rightWall = new Plane(
		frontBottomRight,
		backBottomRight,
		backTopRight,
		frontTopRight
	);
	rightWall->setColor(glm::vec3(1, 0, 0)); // red

	Plane* topWall = new Plane(
		frontTopLeft,
		frontTopRight,
		backTopRight,
		backTopLeft
			
	);
	topWall->setColor(glm::vec3(1, 0.6, 0.1)); // orange

	Plane* backWall = new Plane(
		backTopLeft,
		backTopRight,
		backBottomRight,
		backBottomLeft
		
			
	);
	backWall->setColor(glm::vec3(1, 1, 0)); // yellow

	#pragma endregion

	int mirrorXoffset = 5;
	Plane* frontMirror = new Plane(
		glm::vec3(-width + mirrorXoffset, -10, frontDepth + 10),
		glm::vec3(-width + mirrorXoffset, height - 5, frontDepth + 10),
		glm::vec3(width - mirrorXoffset, height - 5, frontDepth + 10),
		glm::vec3(width - mirrorXoffset, -10, frontDepth + 10)


	);
	frontMirror->setColor(glm::vec3(0, 0, 0));
	frontMirror->setReflectivity(true, 1);

	Plane* backMirror = new Plane(
		glm::vec3(-width + mirrorXoffset, -10, backDepth - 1),
		glm::vec3(-width + mirrorXoffset, height - 5, backDepth - 1),
		glm::vec3(width - mirrorXoffset, height - 5, backDepth - 1),
		glm::vec3(width - mirrorXoffset, -10, backDepth - 1)
	);
	backMirror->setColor(glm::vec3(0, 0, 0));
	backMirror->setReflectivity(true, 1);	


	Sphere* texturedSphere = new Sphere(glm::vec3(-5, 0.0, -EDIST), 2.0);
	texturedSphere->setColor(glm::vec3(1, 0, 0));
	
	Sphere* mirrorSphere = new Sphere(glm::vec3(0.0, -5, -DDIST), 2.5);
	mirrorSphere->setColor(glm::vec3(0, 0, 0));
	mirrorSphere->setReflectivity(true, 1);


	sceneObjects.push_back(refractiveSphere);		 //Add sphere to scene objects
	sceneObjects.push_back(chequeredFloor);
	sceneObjects.push_back(texturedSphere);
	sceneObjects.push_back(frontWall);
	sceneObjects.push_back(leftWall);
	sceneObjects.push_back(rightWall);
	sceneObjects.push_back(topWall);
	sceneObjects.push_back(backWall);
	sceneObjects.push_back(frontMirror);
	sceneObjects.push_back(mirrorSphere);
	sceneObjects.push_back(backMirror);

	texture = TextureBMP("earth.bmp");
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(800, 800);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");
	
    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
