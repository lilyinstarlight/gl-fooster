#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "shader.h"
#include "texture.h"
#include "object.h"

typedef struct {
	GLfloat coords[3];
} obj_v;

typedef struct {
	GLfloat texture_coords[2];
} obj_vt;

typedef struct {
	GLfloat normal[3];
} obj_vn;

int object_init_list(object_list * objects) {
	for (int i = 0; i < objects->num; i++) {
		if (!object_init(objects->list[i]))
			return 0;
	}

	return 1;
}

int object_init(object * current_object) {
	// load shader program
	current_object->program = program_load(current_object->shaders, arrlen(current_object->shaders));

	// set attributes
	current_object->attribute_coords = attrib_bind(current_object->program, "coords");
	current_object->attribute_texture_coords = attrib_bind(current_object->program, "texture_coords");

	// set uniforms
	current_object->uniform_obj_transform = uniform_bind(current_object->program, "obj_transform");
	current_object->uniform_world_transform = uniform_bind(current_object->program, "world_transform");
	current_object->uniform_perspective = uniform_bind(current_object->program, "perspective");
	current_object->uniform_texture = uniform_bind(current_object->program, "texture");

	// check bound variables
	if (current_object->attribute_coords < 0 || current_object->attribute_texture_coords < 0 || current_object->uniform_obj_transform < 0 || current_object->uniform_world_transform < 0 || current_object->uniform_perspective < 0 || current_object->uniform_texture < 0) {
		fprintf(stderr, "Couldn't bind variables in ");
		fprintf(stderr, "%s", current_object->shaders[0]);
		for (unsigned int i = 1; i < arrlen(current_object->shaders); i++)
			fprintf(stderr, ",%s", current_object->shaders[i]);
		fprintf(stderr, "\n");
		return 0;
	}

	// load the object model file
	current_object->buffers = object_load(current_object->filename);
	if (current_object->buffers.vbo == 0 || current_object->buffers.ibo == 0) {
		fprintf(stderr, "Couldn't load object model %s\n", current_object->filename);
		return 0;
	}

	// load texture file
	current_object->texture = texture_load(current_object->texture_filename);
	if (current_object->texture == 0) {
		fprintf(stderr, "Couldn't load texture %s\n", current_object->texture_filename);
		return 0;
	}

	return 1;
}

buffer object_load(const char * filename) {
	FILE * file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "Error opening %s: ", filename);
		perror("");
		return (buffer){0, 0};
	}

	// get number of vertices and indices
	int vertices_size = 0, texture_coords_size = 0, normals_size, indices_size = 0;
	while (!feof(file)) {
		int output;
		char type[16];

		(void)output;

		// get type
		output = fscanf(file, "%15s", type);

		// vertex
		if (strcmp(type, "v") == 0)
			vertices_size++;
		// texture coords
		else if (strcmp(type, "vt") == 0)
			texture_coords_size++;
		// normal
		else if (strcmp(type, "vn") == 0)
			normals_size++;
		// face
		else if (strcmp(type, "f") == 0)
			indices_size++;

		output = fscanf(file, "%*[^\r\n]%*[\r\n]");
	}

	rewind(file);

	// get data
	obj_v * obj_vertices = (obj_v *)malloc(vertices_size * sizeof(obj_v));
	unsigned int ovi = 0;
	obj_vt * obj_texture_coords = (obj_vt *)malloc(texture_coords_size * sizeof(obj_vt));
	unsigned int oti = 0;
	obj_vn * obj_normals = (obj_vn *)malloc(normals_size * sizeof(obj_vn));
	unsigned int oni = 0;

	GLushort * obj_indices = (GLushort *)malloc(indices_size * 3 * sizeof(GLushort));
	GLushort * obj_texture_indices = (GLushort *)malloc(indices_size * 3 * sizeof(GLushort));
	GLushort * obj_normal_indices = (GLushort *)malloc(indices_size * 3 * sizeof(GLushort));
	unsigned int oii = 0;

	// read full file
	while (!feof(file)) {
		int output;
		char type[16];

		// get type
		output = fscanf(file, "%15s", type);

		if (output != 1)
			continue;

		// coordinate
		if (strcmp(type, "v") == 0) {
			output = fscanf(file, "%f %f %f", &obj_vertices[ovi].coords[0], &obj_vertices[ovi].coords[1], &obj_vertices[ovi].coords[2]);
			if (output != 3) {
				fprintf(stderr, "Invalid object file %s\n", filename);
				fclose(file);
				free(obj_vertices);
				free(obj_texture_coords);
				free(obj_normals);
				free(obj_indices);
				free(obj_texture_indices);
				free(obj_normal_indices);
				return (buffer){0, 0};
			}

			ovi++;
		}
		// texture coordinate
		else if (strcmp(type, "vt") == 0) {
			output = fscanf(file, "%f %f", &obj_texture_coords[oti].texture_coords[0], &obj_texture_coords[oti].texture_coords[1]);
			if (output != 2) {
				fprintf(stderr, "Invalid object file %s\n", filename);
				fclose(file);
				free(obj_vertices);
				free(obj_texture_coords);
				free(obj_normals);
				free(obj_indices);
				free(obj_texture_indices);
				free(obj_normal_indices);
				return (buffer){0, 0};
			}

			oti++;
		}
		// normal
		else if (strcmp(type, "vn") == 0) {
			output = fscanf(file, "%f %f %f", &obj_normals[oni].normal[0], &obj_normals[oni].normal[1], &obj_normals[oni].normal[2]);
			if (output != 3) {
				fprintf(stderr, "Invalid object file %s\n", filename);
				fclose(file);
				free(obj_vertices);
				free(obj_texture_coords);
				free(obj_normals);
				free(obj_indices);
				free(obj_texture_indices);
				free(obj_normal_indices);
				return (buffer){0, 0};
			}

			oni++;
		}
		// face
		else if (strcmp(type, "f") == 0) {
			output = fscanf(file, "%hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu", &obj_indices[oii], &obj_texture_indices[oii], &obj_normal_indices[oii], &obj_indices[oii + 1], &obj_texture_indices[oii + 1], &obj_normal_indices[oii + 1], &obj_indices[oii + 2], &obj_texture_indices[oii + 2], &obj_normal_indices[oii + 2]);
			if (output != 9) {
				fprintf(stderr, "Invalid object file %s\n", filename);
				fclose(file);
				free(obj_vertices);
				free(obj_texture_coords);
				free(obj_normals);
				free(obj_indices);
				free(obj_texture_indices);
				free(obj_normal_indices);
				return (buffer){0, 0};
			}

			oii += 3;
		}

		output = fscanf(file, "%*[^\r\n]%*[\r\n]");
	}

	fclose(file);

	// create indexed vertices
	vertex * vertices = (vertex *)malloc(indices_size * sizeof(vertex));
	vertex_normal * normals = (vertex_normal *)malloc(indices_size * sizeof(vertex_normal));
	GLushort * indices = (GLushort *)malloc(indices_size * sizeof(GLushort));
	for (int vidx = 0, iidx = 0, idx = 0; idx < indices_size; idx++) {
		// vertex parameters
		GLfloat x, y, z, u, v, i, j, k;

		// coords
		x = obj_vertices[obj_indices[idx] - 1].coords[0];
		y = obj_vertices[obj_indices[idx] - 1].coords[1];
		z = obj_vertices[obj_indices[idx] - 1].coords[2];

		// texture coords
		u = obj_texture_coords[obj_texture_indices[idx] - 1].texture_coords[0];
		v = obj_texture_coords[obj_texture_indices[idx] - 1].texture_coords[1];

		// normal
		i = obj_normals[obj_normal_indices[idx] - 1].normal[0];
		j = obj_normals[obj_normal_indices[idx] - 1].normal[1];
		k = obj_normals[obj_normal_indices[idx] - 1].normal[2];

		int near = -1;
		for (int v = 0; v < vidx; v++) {
			if (IS_NEAR(vertices[v].coords[0], x) &&
			    IS_NEAR(vertices[v].coords[1], x) &&
			    IS_NEAR(vertices[v].coords[2], x) &&
			    IS_NEAR(vertices[v].texture_coords[0], u) &&
			    IS_NEAR(vertices[v].texture_coords[1], u) &&
			    IS_NEAR(normals[v].normal[0], i) &&
			    IS_NEAR(normals[v].normal[0], j) &&
			    IS_NEAR(normals[v].normal[0], k)) {

				near = v;
				break;
			}
		}

		if (near >= 0) {
			indices[iidx] = near;
		}
		else {
			vertices[vidx].coords[0] = x;
			vertices[vidx].coords[1] = y;
			vertices[vidx].coords[2] = z;
			vertices[vidx].texture_coords[0] = u;
			vertices[vidx].texture_coords[1] = v;

			normals[vidx].normal[0] = i;
			normals[vidx].normal[1] = j;
			normals[vidx].normal[2] = k;

			indices[iidx] = vidx;

			vidx++;
		}

		iidx++;
	}

	free(obj_vertices);
	free(obj_texture_coords);
	free(obj_normals);
	free(obj_indices);
	free(obj_texture_indices);
	free(obj_normal_indices);

	// generate buffers
	buffer buffers;
	glGenBuffers(1, &buffers.vbo);
	glGenBuffers(1, &buffers.ibo);

	// fill vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
	glBufferData(GL_ARRAY_BUFFER, indices_size * sizeof(vertex), vertices, GL_STATIC_DRAW);

	// fill element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_size * sizeof(GLushort), indices, GL_STATIC_DRAW);

	free(vertices);
	free(indices);

	return buffers;
}

void object_display_list(object_list * objects) {
	for (int i = 0; i < objects->num; i++)
		object_display(objects->list[i]);
}

void object_display(object * current_object) {
	// use the shader program
	glUseProgram(current_object->program);

	// specify vertex attribute arrays and bind to vertex buffer
	glEnableVertexAttribArray(current_object->attribute_coords);
	glEnableVertexAttribArray(current_object->attribute_texture_coords);
	glBindBuffer(GL_ARRAY_BUFFER, current_object->buffers.vbo);
	glVertexAttribPointer(current_object->attribute_coords, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), 0); // (attribute, # of elements per vertex, type, take values as they are, stride, offset)
	glVertexAttribPointer(current_object->attribute_texture_coords, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid *)offsetof(vertex, texture_coords));

	// bind 2D texture to the fragment shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, current_object->texture);
	glUniform1i(current_object->uniform_texture, 0);

	// bind element buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, current_object->buffers.ibo);

	// get size of array buffer
	int size;
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &size);

	// draw all of the elements
	glDrawElements(GL_TRIANGLES, size/sizeof(GLushort), GL_UNSIGNED_SHORT, 0);

	// unspecify vertex attribute arrays
	glDisableVertexAttribArray(current_object->attribute_coords);
	glDisableVertexAttribArray(current_object->attribute_texture_coords);
}

void object_destroy_list(object_list * objects) {
	for (int i = 0; i < objects->num; i++)
		object_destroy(objects->list[i]);
}

void object_destroy(object * current_object) {
	glDeleteProgram(current_object->program);
	glDeleteBuffers(1, &current_object->buffers.vbo);
	glDeleteBuffers(1, &current_object->buffers.ibo);
	glDeleteTextures(1, &current_object->texture);
}
