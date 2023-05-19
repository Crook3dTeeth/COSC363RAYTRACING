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
const int NUMDIV = 800;
const int MAX_STEPS = 10;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;

vector<SceneObject*> sceneObjects;
TextureBMP texture;

//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);						//Background colour = (0,0,0)
	glm::vec3 lightPos(0, 10, -5);					//Light's position
	glm::vec3 color(0);
	SceneObject* obj;

    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found

	color = obj->lighting(lightPos, ray.dir, ray.hit);


	if (ray.index == 1)
	{
		//chequered pattern
		int stripeWidth = 5;
		int iz = (ray.hit.z) / stripeWidth;
		int ix = (ray.hit.x) / stripeWidth;
		int k = iz % 2; //2 colors
		int b = ix % 2;


		if (k == 0) 
		{
			if (b == 0) {
				color = glm::vec3(1, 1, 1);
			} else {
				color = glm::vec3(0, 0, 0);
			}
		} else 
		{
			if (b == 0) {
				color = glm::vec3(0, 0, 0);
			} else {
				color = glm::vec3(1, 1, 1);
			}
		}
		obj->setColor(color);
	}


							//Object's colour
	glm::vec3 lightVec = lightPos - ray.hit;
	Ray shadowRay(ray.hit, lightVec);
	shadowRay.closestPt(sceneObjects);
	if (shadowRay.index > -1 && shadowRay.dist < glm::length(lightVec))
		color = 0.2f * obj->getColor(); //0.2 = ambient scale factor

	if (obj->isReflective() && step < MAX_STEPS)
	{
		float rho = obj->getReflectionCoeff();
		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
		Ray reflectedRay(ray.hit, reflectedDir);
		glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
		color = color + (rho * reflectedColor);
	}

	return color;
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

			bool AA = false;
			if (AA) {
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


				


				col = (col1 + col2 + col3 + col4) / 4.0f;
			} else {
				col = trace(ray, 1); //Trace the primary ray and get the colour value
			}

			bool AA2 = false;
			if (AA2) {
				int iOffset;
				int jOffset;
				if (i > 0) {
					if (j > 0) {
						unsigned char leftPixel[4];
						unsigned char topLeftPixel[4];
						unsigned char topPixel[4];

						iOffset = i - 1;
						jOffset = j;
						xp = XMIN + iOffset * cellX;
						yp = YMIN + jOffset * cellY;
						glReadPixels(xp, yp, cellX, cellY, GL_RGBA, GL_UNSIGNED_BYTE, leftPixel);
						glm::vec3 leftP(leftPixel[0] / 255.0f, leftPixel[1] / 255.0f, leftPixel[2] / 255.0f);


						iOffset = i - 1;
						jOffset = j - 1;
						xp = XMIN + iOffset * cellX;
						yp = YMIN + jOffset * cellY;
						glReadPixels(xp, yp, cellX, cellY, GL_RGBA, GL_UNSIGNED_BYTE, topLeftPixel);
						glm::vec3 topLeftP(topLeftPixel[0] / 255.0f, topLeftPixel[1] / 255.0f, topLeftPixel[2] / 255.0f);

						iOffset = i;
						jOffset = j - 1;
						xp = XMIN + iOffset * cellX;
						yp = YMIN + jOffset * cellY;
						glReadPixels(xp, yp, cellX, cellY, GL_RGB, GL_UNSIGNED_BYTE, topPixel);
						glm::vec3 topP(topPixel[0] / 255.0f, topPixel[1] / 255.0f, topPixel[2] / 255.0f);

						glm::vec3 leftDif = glm::abs(col - leftP);
						glm::vec3 topLeftDif = glm::abs(col-topLeftP);
						glm::vec3 topDif = glm::abs(col - topP);

						float avgDifLeft = (leftDif.r + leftDif.g + leftDif.b) / 3.0f;
						float avgDifTopLeft = (topLeftDif.r + topLeftDif.g + topLeftDif.b) / 3.0f;
						float avgDifTop = (topDif.r + topDif.g + topDif.b) / 3.0f;		

						float aaThreshhold = 0.5;

						if (avgDifTop > aaThreshhold || avgDifLeft > aaThreshhold || avgDifTopLeft > aaThreshhold) {
							xp = XMIN + i * cellX;
							yp = YMIN + j * cellY;
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

							col = (col1 + col2 + col3 + col4) / 4.0f;
						}
					}
				}



			}

			bool AA3 = false;
			if (AA3) {
				// Cast ray in corner of pixel to blend, free AA for not much performance hit : https://www.youtube.com/watch?v=A61S_2swwAc

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

	
	Sphere* sphere1 = new Sphere(glm::vec3(-5, 0.0, -50.0), 2.0);

	sphere1->setColor(glm::vec3(0, 0, 1));   //Set colour to blue
	sphere1->setReflectivity(true, 0.8);
	//sphere1->setTransparency(true, 1);
	//sphere1->setRefractivity(true, 1.01, 2);


	// Half width
	float width = 30.0;
	float height = 20.0;
	float frontDepth = -200;
	float backDepth = 20;
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
	//chequeredFloor->setColor(glm::vec3(0.8, 0.8, 0));
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

	sceneObjects.push_back(sphere1);		 //Add sphere to scene objects

	sceneObjects.push_back(chequeredFloor);
	sceneObjects.push_back(frontWall);
	sceneObjects.push_back(leftWall);
	sceneObjects.push_back(rightWall);
	sceneObjects.push_back(topWall);
	sceneObjects.push_back(backWall);
	//sceneObjects.push_back(frontMirror);
	//sceneObjects.push_back(backMirror);
}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(700, 700);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
