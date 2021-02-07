/**
 * Copyright (c) 2020, Evgeniy Morozov
 * All rights reserved.
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <pthread.h>
#include <stdio.h>
#include "hittables/rt_hittable_list.h"
#include "rt_camera.h"
#include "rt_skybox_simple.h"
#include <errno.h>
#include <string.h>
#include <scenes/rt_scenes.h>
#include <assert.h>
#include "mpi.h"

#define TAMANHO_VETOR 60000 // IMAGE_HEIGHT * IMAGE_WIDTH

#define HEIGHT 200
#define WIDTH 300


static void show_usage(const char *program_name, int err);

int NUM_THREADS = 1;

static colour_t ray_colour(const ray_t *ray, const rt_hittable_list_t *list, rt_skybox_t *skybox, int child_rays)
{
    if (child_rays <= 0)
    {
        return colour(0, 0, 0);
    }

    rt_hit_record_t record;
    if (!rt_hittable_list_hit_test(list, ray, 0.001, INFINITY, &record))
    {
        return rt_skybox_value(skybox, ray);
    }

    ray_t scattered;
    colour_t attenuation;
    colour_t emitted = rt_material_emit(record.material, record.u, record.v, &record.p);
    if (rt_material_scatter(record.material, ray, &record, &attenuation, &scattered))
    {
        return vec3_sum(emitted, vec3_multiply(attenuation, ray_colour(&scattered, list, skybox, child_rays - 1)));
    }
    return emitted;
}

typedef struct thread_parameters
{
	rt_camera_t *camera ;
	long tid ;
	rt_skybox_t *skybox ;
	rt_hittable_list_t *world ;
	int number_of_samples ;
	int height ;
	int width ;
	int CHILD_RAYS ;
  int rank;
  int work;
  pixelColour_t *vetor;
} thread_parameters;

void *aThread(void *arg) 
{
	thread_parameters *param = (thread_parameters*) arg;
	long tid = param->tid ;
	int i, j;

	colour_t pixel;
	for (int x=tid; x < param-work; x+=NUM_THREADS) 
	{
    aux = x + (rank * work);
		j = aux / param->width;
		i = aux % param->width;
		pixel = colour(0, 0, 0);
		for (int s = 0; s < param->number_of_samples; ++s)
		{
			double u = (double)(i + rt_random_double(0, 1)) / (param->width - 1);
			double v = (double)(j + rt_random_double(0, 1)) / (param->height - 1);

			ray_t ray = rt_camera_get_ray(param->camera, u, v);
			vec3_add(&pixel, ray_colour(&ray, param->world, param->skybox, param->CHILD_RAYS));
		}
		rt_write_colour(&param->vetor[x], pixel, param->number_of_samples);
	} 

  int i, j, aux;
    colour_t pixel;

    for (int x=0; x < work; ++x) 
    {
      aux = x + (rank * work);
      j = aux / IMAGE_WIDTH;
      i = aux % IMAGE_WIDTH;
      
      pixel = colour(0, 0, 0);
      for (int s = 0; s < number_of_samples; ++s)
      {
        double u = (double)(i + rt_random_double(0, 1)) / (IMAGE_WIDTH - 1);
        double v = (double)(j + rt_random_double(0, 1)) / (IMAGE_HEIGHT - 1);

        ray_t ray = rt_camera_get_ray(camera, u, v);
        vec3_add(&pixel, ray_colour(&ray, world, skybox, CHILD_RAYS));
      }
      rt_write_colour(&pixels[x], pixel, number_of_samples);
    }

	pthread_exit(NULL);
}


//int main(int argc, char const *argv[])
int main(int argc, char **argv)
{
    const char *number_of_samples_str = NULL;
    const char *scene_id_str = NULL;
    const char *file_name = NULL;
    bool verbose = false;
    

    // pthread_t threads[NUM_THREADS];

    // Parse console arguments
    for (int i = 1; i < argc; ++i)
    {
        if (0 == strcmp(argv[i], "-s") || 0 == strcmp(argv[i], "--samples"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Fatal error: Argument '%s' doesn't have a value\n", argv[i]);
                show_usage(argv[0], EXIT_FAILURE);
            }
            number_of_samples_str = argv[++i];
            continue;
        }
        else if (0 == strcmp(argv[i], "--scene"))
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Fatal error: Argument '%s' doesn't have a value\n", argv[i]);
                show_usage(argv[0], EXIT_FAILURE);
            }
            scene_id_str = argv[++i];
            continue;
        }
        else if (0 == strcmp(argv[i], "-v") || 0 == strcmp(argv[i], "--verbose"))
        {
            verbose = true;
        }
        else if (0 == strcmp(argv[i], "-h"))
        {
            show_usage(argv[0], EXIT_SUCCESS);
        }
		    else if (0 == strcmp(argv[i], "--qtthreads"))
        {
            NUM_THREADS = atoi(argv[++i]);
        }
        else if ('-' == *argv[i])
        {
            fprintf(stderr, "Fatal error: Unknown argument '%s'\n", argv[i]);
            show_usage(argv[0], EXIT_FAILURE);
        }
        else if (i + 1 < argc)
        {
            fprintf(stderr, "Fatal error: Too many positional arguments (1 expected)\n");
            show_usage(argv[0], EXIT_FAILURE);
        }
        file_name = argv[i];
    }

    if (verbose)
    {
        fprintf(stderr, "Non-parsed parameters:\n");
        fprintf(stderr, "\t- number of samples: %s\n", number_of_samples_str);
        fprintf(stderr, "\t- scene ID:          %s\n", scene_id_str);
        fprintf(stderr, "\t- file_name:         %s\n", file_name);
    }

    // Parse resulting parameters
    long number_of_samples = 1000;
    if (NULL != number_of_samples_str)
    {
        char *end_ptr = NULL;
        number_of_samples = strtol(number_of_samples_str, &end_ptr, 10);
        if (*end_ptr != '\0')
        {
            fprintf(stderr, "Fatal error: Value of 'samples' is not a correct number\n");
            show_usage(argv[0], EXIT_FAILURE);
        }
    }
    rt_scene_id_t scene_id = RT_SCENE_SHOWCASE;
    if (NULL != scene_id_str)
    {
        scene_id = rt_scene_get_id_by_name(scene_id_str);
        if (RT_SCENE_NONE == scene_id)
        {
            fprintf(stderr, "Fatal error: Invalid scene identifier\n");
            show_usage(argv[0], EXIT_FAILURE);
        }
    }

    if (verbose)
    {
        fprintf(stderr, "Parsed parameters:\n");
        fprintf(stderr, "\t- number of samples: %ld\n", number_of_samples);
        fprintf(stderr, "\t- scene ID:          %d\n", scene_id);
        fprintf(stderr, "\t- file_name:         %s\n", file_name);
    }



    // Image parameters
    const double ASPECT_RATIO = 3.0 / 2.0;
    const int IMAGE_WIDTH = 300;
    const int IMAGE_HEIGHT = (int)(IMAGE_WIDTH / ASPECT_RATIO);
    const int CHILD_RAYS = 50;

    // Declare Camera parameters
    point3_t look_from, look_at;
    vec3_t up = point3(0, 1, 0);
    double focus_distance = 10.0, aperture = 0.0, vertical_fov = 40.0;

    // World
    rt_hittable_list_t *world = NULL;
    rt_skybox_t *skybox = NULL;

    // Select a scene from a pre-defined one
    switch (scene_id)
    {
        case RT_SCENE_RANDOM:
            look_from = point3(13, 2, 3);
            look_at = point3(0, 0, 0);
            aperture = 0.1;
            vertical_fov = 20.0;

            skybox = rt_skybox_new_gradient(colour(1, 1, 1), colour(0.5, 0.7, 1));
            world = rt_scene_random();
            break;

        case RT_SCENE_TWO_SPHERES:
            look_from = point3(13, 2, 3);
            look_at = point3(0, 0, 0);
            vertical_fov = 20.0;

            skybox = rt_skybox_new_gradient(colour(1, 1, 1), colour(0.5, 0.7, 1));
            world = rt_scene_two_spheres();
            break;

        case RT_SCENE_TWO_PERLIN_SPHERES:
            look_from = point3(13, 2, 3);
            look_at = point3(0, 0, 0);
            vertical_fov = 20.0;

            skybox = rt_skybox_new_gradient(colour(1, 1, 1), colour(0.5, 0.7, 1));
            world = rt_scene_two_perlin_spheres();
            break;

        case RT_SCENE_EARTH:
            look_from = point3(13, 2, 3);
            look_at = point3(0, 0, 0);
            vertical_fov = 20.0;

            skybox = rt_skybox_new_gradient(colour(1, 1, 1), colour(0.5, 0.7, 1));
            world = rt_scene_earth();
            break;

        case RT_SCENE_LIGHT_SAMPLE:
            look_from = point3(26, 3, 6);
            look_at = point3(0, 2, 0);
            vertical_fov = 20.0;

            skybox = rt_skybox_new_background(colour(0, 0, 0));
            world = rt_scene_light_sample();
            break;

        case RT_SCENE_CORNELL_BOX:
            look_from = point3(278, 278, -800);
            look_at = point3(278, 278, 0);

            skybox = rt_skybox_new_background(colour(0, 0, 0));
            world = rt_scene_cornell_box();
            break;

        case RT_SCENE_INSTANCE_TEST:
            look_from = point3(0, 5, -20);
            look_at = point3(0, 0, 0);
            vertical_fov = 20.0;

            skybox = rt_skybox_new_gradient(colour(1, 1, 1), colour(0.5, 0.7, 1));
            world = rt_scene_instance_test();
            break;

        case RT_SCENE_CORNELL_SMOKE:
            look_from = point3(278, 278, -800);
            look_at = point3(278, 278, 0);

            skybox = rt_skybox_new_background(colour(0, 0, 0));
            world = rt_scene_cornell_box_smoke_boxes();
            break;

        case RT_SCENE_SHOWCASE:
            look_from = point3(478, 278, -600);
            look_at = point3(278, 278, 0);

            skybox = rt_skybox_new_background(colour(0, 0, 0));
            world = rt_scene_showcase();
            break;

        case RT_SCENE_METAL_TEST:
            look_from = point3(0, 5, -10);
            look_at = point3(0, 2, 0);
            vertical_fov = 20.0;

            skybox = rt_skybox_new_gradient(colour(1, 1, 1), colour(0.5, 0.7, 1));
            world = rt_scene_metal_test();
            break;
        case RT_SCENE_NONE:
            fprintf(stderr, "Fatal error: scene id is undefined after parsing the parameters\n");
            return EXIT_FAILURE;
    }

    rt_camera_t *camera =
        rt_camera_new(look_from, look_at, up, vertical_fov, ASPECT_RATIO, aperture, focus_distance, 0.0, 1.0);
	


    FILE *out_file = stdout;
    if (NULL != file_name)
    {
        out_file = fopen(file_name, "w");
        if (NULL == out_file)
        {
            fprintf(stderr, "Fatal error: Unable to open file %s: %s", file_name, strerror(errno));
            
            //goto cleanup;
          
            rt_hittable_list_deinit(world);
            rt_camera_delete(camera);
            rt_skybox_delete(skybox);
            return EXIT_SUCCESS;
        }
    }

    int numtasks, rank, dest, source, tag=0;

    // MPI INIT
    MPI_Init(&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int TAMANHO_VETOR_FINAL = TAMANHO_VETOR % numtasks > 0 ? TAMANHO_VETOR + (numtasks - (TAMANHO_VETOR % numtasks)) : TAMANHO_VETOR;
	  int work = TAMANHO_VETOR_FINAL / numtasks;
    pixelColour_t pixels[work], *recvbuf;
    
    if (rank == 0) {
      // Render
      fprintf(out_file, "P3\n%d %d\n255\n", IMAGE_WIDTH, IMAGE_HEIGHT);

      recvbuf=(pixelColour_t*)malloc(TAMANHO_VETOR_FINAL*sizeof(pixelColour_t));
    }


    // create threads
    thread_parameters param[NUM_THREADS];
	
    int t;
    for (t = 0; t < NUM_THREADS; t++) 
	  {
      param[t].camera = camera ;
      param[t].tid = t ;
      param[t].skybox = skybox ;
      param[t].world = world ;
      param[t].number_of_samples = number_of_samples ;
      param[t].width = IMAGE_WIDTH ;
      param[t].height = IMAGE_HEIGHT ;
      param[t].CHILD_RAYS = CHILD_RAYS;
      param[t].vetor = pixels;
      param[t].rank = rank;
      param[t].work = work;
		
      //fprintf(stderr,"thread main: creating thread %ld\n", t);
      int ret = pthread_create(&threads[t], NULL, aThread, (void*)&param[t]);
      if (ret){
          //fprintf(stderr,"ERROR; return code from pthread_create() is %d\n", ret);
          exit(ret);
      }
    }

    /* Wait for the other threads */
    for(t=0; t<NUM_THREADS; t++) 
    {
      int ret = pthread_join(threads[t], NULL);
      if (ret) {
      //	printf("ERROR; return code from pthread_join() is %d\n", ret);
        exit(ret);
      }
      //printf("thread main: joined with thread %ld\n", t);
    }


    MPI_Datatype custom_t;
    MPI_Type_contiguous(3,MPI_INT,&custom_t);
    MPI_Type_commit(&custom_t);    
    
    MPI_Gather(pixels, work , custom_t , recvbuf, work, custom_t , 0, MPI_COMM_WORLD);
 
    if (rank == 0) {
      int k;
      
      for (int j = IMAGE_HEIGHT - 1; j >= 0; --j)
      {
            for (int i = 0; i < IMAGE_WIDTH; ++i) 
        {
          int k = j * IMAGE_WIDTH + i;
          fprintf(out_file, "%d %d %d\n", recvbuf[k].r, recvbuf[k].g, recvbuf[k].b);
        }
      }

      free(recvbuf);
    }

    MPI_Finalize();
    fprintf(stderr, "\nDone\n");
    
cleanup:
    // Cleanup
    rt_hittable_list_deinit(world);
    rt_camera_delete(camera);
    rt_skybox_delete(skybox);

    return EXIT_SUCCESS;
}

static void show_usage(const char *program_name, int err)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "%s [-s|--samples N] [--scene SCENE] [-v|--verbose] [output_file_name]\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "\t-s | --samples      <int>       Number of rays to cast for each pixel\n");
    fprintf(stderr, "\t--scene             <string>    ID of the scene to render. List of available scenes is printed below.\n");
    fprintf(stderr, "\t-v | --verbose                  Enable verbose output\n");
    fprintf(stderr, "\t-h                              Show this message and exit\n");
    fprintf(stderr, "Positional arguments:\n");
    fprintf(stderr, "\toutput_file_name                Name of the output file. Outputs image to console if not specified.\n");
    fprintf(stderr, "Available scenes:\n");
    rt_scene_print_scenes_info(stderr);

    exit(err);
}
