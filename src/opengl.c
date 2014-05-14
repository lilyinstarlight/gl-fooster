#include <math.h>
#include <stdio.h>

#include "matrix.h"

#include "opengl.h"

int initGL(float fov_v, float near_v, float far_v, float width, float height, float cam_x, float cam_y, float cam_z, float center_x, float center_y, float center_z, float cam_roll, object * init_objects[], int init_num_objects) {
	//Set up OpenGL parameters
	glEnable(GL_BLEND); //For transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); //Also for transparancy
	glEnable(GL_DEPTH_TEST); //For 3D

	//Set up view matrix
	fov = fov_v;
	near = near_v;
	far = far_v;
	resizeGL(width, height);

	//Prepare world variables
	x = y = z = rot_x = rot_y = rot_z = 0;

	//Create world matrix
	createIdentity(world_matrix);
	translateGL(cam_x, cam_y, cam_z);
	rotateGL(atan2(y - center_y, z - center_z), -atan2(x - center_x, z - center_z), cam_roll);

	//Init objects
	if(!initObjects(init_objects, init_num_objects)) {
		fprintf(stderr, "Error initializing objects\n");
		destroyObjects();
		return 1;
	}

	//Run everything once to initialize it
	animateGL(0);
	updateGL();

	return 0;
}

void deinitGL() {
	destroyObjects(); //Free up memory again
}

void resizeGL(int width_v, int height_v) {
	width = width_v;
	height = height_v;

	//Reset view matrix
	createPerspectiveMatrix(perspective, fov, width / height, near, far);
	for(int i = 0; i < num_objects; i++)
		glUniformMatrix4fv(objects[i]->uniform_perspective, 1, GL_FALSE, perspective);

	glViewport(0, 0, width, height);
}

void animateGL(float secs) {
	//Set the animation
	for(int i = 0; i < num_objects; i++) {
		glUseProgram(objects[i]->program);
		GLfloat matrix[16];
		createIdentity(matrix); //Load up the identity
		if(objects[i]->x_rotation_speed)
			xRotateMatrix(matrix, secs * objects[i]->x_rotation_speed); //And modify the matrix to rotate the object over time
		if(objects[i]->y_rotation_speed)
			yRotateMatrix(matrix, secs * objects[i]->y_rotation_speed);
		if(objects[i]->z_rotation_speed)
			zRotateMatrix(matrix, secs * objects[i]->z_rotation_speed);
		glUniformMatrix4fv(objects[i]->uniform_obj_transform, 1, GL_FALSE, matrix); //Apply the transformation matrix
	}
}

void updateGL() {
	//Put objects in buffer
	displayObjects();

	//Apply camera matrix
	for(int i = 0; i < num_objects; i++)
		glUniformMatrix4fv(objects[i]->uniform_world_transform, 1, GL_FALSE, world_matrix);
}

void translateGL(float dx, float dy, float dz) {
	translateMatrix(world_matrix, -dx, -dy, -dz);

	x += dx;
	y += dy;
	z += dz;
}

void rotateGL(float drot_x, float drot_y, float drot_z) {
	translateMatrix(world_matrix, x, y, z);
	rotateMatrix(world_matrix, drot_x, drot_y, drot_z);
	translateMatrix(world_matrix, -x, -y, -z);

	rot_x += drot_x;
	rot_y += drot_y;
	rot_z += drot_z;
}
