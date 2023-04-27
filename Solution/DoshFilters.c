/**
* a program that utilizes multithreading to apply a filter to an image
*
* Completion time: 12 hours
*
* @author Kelston Dosh
* @version 1.0
*/

////////////////////////////////////////////////////////////////////////////////
//INCLUDES
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include "BmpProcessor.h"


////////////////////////////////////////////////////////////////////////////////
//MACRO DEFINITIONS
#define THREAD_COUNT 4


////////////////////////////////////////////////////////////////////////////////
//DATA STRUCTURES

struct img{
    struct Pixel** pArr;
    int width;
    int height;
};

struct hole{
    int x;
    int y;
    int radius;
};

struct thread_info{
    int width;
    int height;
    int num;
    struct Pixel** pArr;
    struct hole* holes;
    int numHoles;
};


////////////////////////////////////////////////////////////////////////////////
//MAIN PROGRAM CODE
int seed;

void* apply_blur(void* data){
    struct thread_info* info = (struct thread_info*) data;

    int width = info->width;
    int height = info->height;

    //create temp array to hold altered pixels
    struct Pixel** new_pArr = (struct Pixel**) malloc(sizeof(struct Pixel*) * height);
    for (int i = 0; i < height; i++){
        //the first and last threads have 1 extra column of overlap, the rest have 2 columns of overlap.
        if (info->num == 0 || info->num == THREAD_COUNT - 1){
            new_pArr[i] = (struct Pixel*) malloc(sizeof(struct Pixel) * width + 1);
        } else {
            new_pArr[i] = (struct Pixel*) malloc(sizeof(struct Pixel) * width + 2);
        }

    }

    for (int i = 0; i < height; i++){
        //for all threads except the first thread, start j at 1
        for (int j = info->num > 0 ? 1 : 0; j < width + 1; j++){
            int rSum = 0;
            int gSum = 0;
            int bSum = 0;
            int neighbors = 0;

            //current pixel
            rSum += info->pArr[i][j].red;
            gSum += info->pArr[i][j].green;
            bSum += info->pArr[i][j].blue;
            neighbors++;

            //top-left pixel
            if (i > 0 && j > 0){
                rSum += info->pArr[i - 1][j - 1].red;
                gSum += info->pArr[i - 1][j - 1].green;
                bSum += info->pArr[i - 1][j - 1].blue;
                neighbors++;
            }

            //top pixel
            if (i > 0){
                rSum += info->pArr[i - 1][j].red;
                gSum += info->pArr[i - 1][j].green;
                bSum += info->pArr[i - 1][j].blue;
                neighbors++;
            }

            //top-right pixel
            if (i > 0 && j < width - 1){
                rSum += info->pArr[i - 1][j + 1].red;
                gSum += info->pArr[i - 1][j + 1].green;
                bSum += info->pArr[i - 1][j + 1].blue;
                neighbors++;
            }

            //right pixel
            if (j < width - 1){
                rSum += info->pArr[i][j + 1].red;
                gSum += info->pArr[i][j + 1].green;
                bSum += info->pArr[i][j + 1].blue;
                neighbors++;
            }

            //bottom-right pixel
            if (i < height - 1 && j < width - 1){
                rSum += info->pArr[i + 1][j + 1].red;
                gSum += info->pArr[i + 1][j + 1].green;
                bSum += info->pArr[i + 1][j + 1].blue;
                neighbors++;
            }

            //bottom pixel
            if (i < height - 1){
                rSum += info->pArr[i + 1][j].red;
                gSum += info->pArr[i + 1][j].green;
                bSum += info->pArr[i + 1][j].blue;
                neighbors++;
            }

            //bottom-left pixel
            if (i < height - 1 && j > 0){
                rSum += info->pArr[i + 1][j - 1].red;
                gSum += info->pArr[i + 1][j - 1].green;
                bSum += info->pArr[i + 1][j - 1].blue;
                neighbors++;
            }

            //left pixel
            if (j > 0) {
                rSum += info->pArr[i][j - 1].red;
                gSum += info->pArr[i][j - 1].green;
                bSum += info->pArr[i][j - 1].blue;
                neighbors++;
            }

            //calculate averages and assign to corresponding pixel in new_pArr
            new_pArr[i][j].red = rSum / neighbors;
            new_pArr[i][j].green = gSum / neighbors;
            new_pArr[i][j].blue = bSum / neighbors;
        }
    }

    //free memory
    for (int i = 0; i < info->height; i++){
        free(info->pArr[i]);
    }
    free(info->pArr);

    //set the threads pixel array to the new pixel array
    info->pArr = new_pArr;
    pthread_exit(0);
}

void apply_yellow(struct thread_info* info){
    for (int i = 0; i < info->height; i++){
        for (int j = 0; j < info->width; j++){
            //get current rgb values
            int red = info->pArr[i][j].red;
            int green = info->pArr[i][j].green;
            int blue = info->pArr[i][j].blue;

            //calculate new rgb values.
            //Increasing red and green, and decreasing blue results in an overall yellow color
            //using ratios ensures that the values do not exceed 255 or fall below 0
            red += (255 - red) / 2;
            green += (255 - green) / 2;
            blue /= 2;

            //assign new rgb values to thread array
            info->pArr[i][j].red = red;
            info->pArr[i][j].green = green;
            info->pArr[i][j].blue = blue;
        }
    }
}

void generateHoles(struct hole** holeCenters, struct img* img){
    int width = img->width;
    int height = img->height;
    int smallest = width > height ? height : width;
    int numHoles = (int) (smallest * 0.08);
    int radiusLowerLim = numHoles / 2;
    int radiusUpperLim = numHoles + radiusLowerLim + 1;//the +1 is because the upper limit is exclusive in rand calculations
    *holeCenters = (struct hole*) malloc(sizeof(struct hole) * numHoles);
    srand(seed);

    for (int i = 0; i < numHoles; i++){
        //get random values for x, y, and radius
        (*holeCenters)[i].x = rand() % (width + 1); //random value between 0 and width
        (*holeCenters)[i].y = rand() % (height + 1); //random value between 0 and height
        //random value between upper and lower radius limits.
        //the average value of a random number is the middle value of the range of possible values.
        //therefore, the average radius size will be equal to the smallest dimension * 0.08
        (*holeCenters)[i].radius = (rand() % (radiusUpperLim - radiusLowerLim)) + radiusLowerLim;
    }
}

int getDistance(int x1, int y1, int x2, int y2){
    return (int) sqrt(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)));
}

void drawHoles(struct thread_info* info){
    int width = info->width;
    int height = info->height;

    for (int i = 0; i < height; i++){
        for (int j = 0; j < width; j++){
            for (int k = 0; k < info->numHoles; k++){
                //calculate the distance between the current pixel and the center of each hole.
                //if the distance is smaller than the radius, turn the pixel black and break the current loop.
                //j + width * info->num maps the column index of the threads sub array to the column index of the original image.
                int distance = getDistance(j + width * info->num, i, info->holes[k].x, info->holes[k].y);
                if (distance <= info->holes[k].radius){
                    info->pArr[i][j].red = 0;
                    info->pArr[i][j].green = 0;
                    info->pArr[i][j].blue = 0;
                    break;
                }
            }
        }
    }
}

void* apply_cheese(void* data){
    struct thread_info* info = (struct thread_info*) data;
    apply_yellow(info);
    drawHoles(info);
    pthread_exit(0);
}

void apply_blur_threads(struct img* img){
    pthread_t tids[THREAD_COUNT];
    struct thread_info** infos = (struct thread_info**) malloc(sizeof(struct thread_info*) * THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++){
        //instantiate each pthread and calculate height and width of its sub array
        infos[i] = (struct thread_info*) malloc(sizeof(struct thread_info));
        infos[i]->width = img->width / THREAD_COUNT;
        infos[i]->height = img->height;

        //create sub array
        struct Pixel** pArr = (struct Pixel**) malloc(sizeof(struct Pixel*) * img->height);
        for (int j = 0; j < img->height; j++){
            //for the first thread, allocate 1 extra column to width.
            if (i == 0){
                pArr[j] = (struct Pixel*) malloc(sizeof(struct Pixel) * infos[i]->width + 1);

                //the pixels of the original image map directly to the sub array
                for (int k = 0; k < infos[i]->width + 1; k++) {
                    pArr[j][k] = img->pArr[j][k];
                }
            //for the last thread, allocate 1 extra column to width.
            } else if (i == THREAD_COUNT - 1){
                pArr[j] = (struct Pixel*) malloc(sizeof(struct Pixel) * infos[i]->width + 1);

                //each pixel corresponds to the current column + the width of the sub array * the thread number.
                //subtract 1 to offset for extra column on left.
                for (int k = 0; k < infos[i]->width + 1; k++) {
                    pArr[j][k] = img->pArr[j][k + infos[i]->width * i - 1];
                }
            //all other threads get 2 extra columns
            } else {
                pArr[j] = (struct Pixel*) malloc(sizeof(struct Pixel) * infos[i]->width + 2);

                for (int k = 0; k < infos[i]->width + 2; k++){
                    pArr[j][k] = img->pArr[j][k + infos[i]->width * i - 1];
                }
            }
        }

        infos[i]->pArr = pArr;
    }

    //create each thread
    for (int i = 0; i < THREAD_COUNT; i++){
        infos[i]->num = i;
        pthread_create(&tids[i], NULL, apply_blur, infos[i]);
    }

    //join each thread
    for (int i = 0; i < THREAD_COUNT; i++){
        pthread_join(tids[i], NULL);
    }

    //copy each sub array back into the original image
    for (int i = 0; i < img->height; i++){
        for (int j = 0; j < img->width; j++){
            int thread = j / (img->width / THREAD_COUNT); //calculate which thread owns the data for the current pixel

            //the first thread maps directly to the original image
            if (thread == 0){
                img->pArr[i][j] = infos[thread]->pArr[i][j];
            //all other threads need to offset the column index
            } else {
                img->pArr[i][j] = infos[thread]->pArr[i][(j - infos[thread]->width * thread) + 1];
            }
        }
    }

    //free memory
    for (int i = 0; i < THREAD_COUNT; i++){
        for (int j = 0; j < infos[i]->height; j++){
            free(infos[i]->pArr[j]);
        }
        free(infos[i]->pArr);
        free(infos[i]);
    }

    free(infos);
}

void getHoles(struct hole* holes, int numHoles, struct thread_info* info){
    int holeIndexes[numHoles]; //track indexes of holes to assign to this thread
    int count = 0;

    for (int i = 0; i < numHoles; i++){
        int start = info->width * info->num; //the left edge of the original image that is assigned to this thread
        int end = start + info->width; //the right edge of the original image that is assigned to this thread
        int leftBound = holes[i].x - holes[i].radius; //the furthest left point of the current hole
        int rightBound = holes[i].x + holes[i].radius; //the furthest right point of the current hole

        //if any part of the hole is in this section of the image, mark the index of the hole
        if ((leftBound >= start && leftBound < end) || (rightBound >= start && rightBound < end)){
            holeIndexes[count] = i;
            count++;
        }
    }

    struct hole* threadHoles = (struct hole*) malloc(sizeof(struct hole) * count);

    //copy each hole into a new array
    for (int i = 0; i < count; i++){
        threadHoles[i] = holes[holeIndexes[i]];
    }

    //assign the holes to this thread
    info->holes = threadHoles;
    info->numHoles = count;
}

void apply_cheese_threads(struct img* img){
    pthread_t tids[THREAD_COUNT];
    struct thread_info** infos = (struct thread_info**) malloc(sizeof(struct thread_info*) * THREAD_COUNT);

    for (int i = 0; i < THREAD_COUNT; i++){
        //instantiate each thread and calculate width and height
        infos[i] = (struct thread_info*) malloc(sizeof(struct thread_info));
        infos[i]->width = img->width / THREAD_COUNT;
        infos[i]->height = img->height;

        //create sub array
        struct Pixel** pArr = (struct Pixel**) malloc(sizeof(struct Pixel*) * img->height);
        for (int j = 0; j < img->height; j++){
            pArr[j] = (struct Pixel*) malloc(sizeof(struct Pixel) * infos[i]->width);

            //copy data from original image to sub array
            for (int k = 0; k < infos[i]->width; k++){
                pArr[j][k] = img->pArr[j][k + infos[i]->width * i];
            }
        }
        infos[i]->pArr = pArr;
    }

    struct hole* holes;
    generateHoles(&holes, img);

    int width = img->width;
    int height = img->height;
    int smallest = width > height ? height : width;
    int numHoles = (int) (smallest * 0.08);

    //determine which holes each thread is responsible for
    for (int i = 0; i < THREAD_COUNT; i++){
        infos[i]->num = i;
        getHoles(holes, numHoles, infos[i]);
    }

    //create each thread
    for (int i = 0; i < THREAD_COUNT; i++){
        pthread_create(&tids[i], NULL, apply_cheese, infos[i]);
    }

    //join threads
    for (int i = 0; i < THREAD_COUNT; i++){
        pthread_join(tids[i], NULL);
    }

    //copy data from each thread back into original image
    for (int i = 0; i < img->height; i++){
        for (int j = 0; j < img->width; j++){
            int thread = j / (img->width / THREAD_COUNT);
            if (thread == 0){
                img->pArr[i][j] = infos[thread]->pArr[i][j];
            } else {
                img->pArr[i][j] = infos[thread]->pArr[i][(j - infos[thread]->width * thread)];
            }
        }
    }

    //free memory
    for (int i = 0; i < THREAD_COUNT; i++){
        for (int j = 0; j < infos[i]->height; j++){
            free(infos[i]->pArr[j]);
        }
        free(infos[i]->pArr);
        free(infos[i]->holes);
        free(infos[i]);
    }

    free(infos);
    free(holes);
}

//use the sum of the char values of the input file name as a seed for random number generation
int generateSeed(char* str){
    int sum = 0;

    while(*str){
        sum += *str;
        str++;
    }

    return sum;
}

void main(int argc, char* argv[]) {
    char* input;
    char* output;
    char filter;
    int option;

    //parse command line args
    while ((option = getopt(argc, argv, "i:o:f:")) != -1){
        switch (option) {
            case 'i' :
                input = optarg;
                break;
            case 'o' :
                output = optarg;
                break;
            case 'f' :
                filter = optarg[0];
                break;
            default :
                printf("invalid input");
                break;
        }
    }

    struct BMP_Header BMP;
    struct DIB_Header DIB;
    struct Pixel** pArr;
    struct img img;

    //read input file
    FILE* in = fopen(input, "rb");
    readBMPHeader(in, &BMP);
    readDIBHeader(in, &DIB);

    pArr = (struct Pixel**)malloc(sizeof(struct Pixel*) * DIB.height);
    for (int p = 0; p < DIB.height; p++) {
        pArr[p] = (struct Pixel *) malloc(sizeof(struct Pixel) * DIB.width);
    }

    readPixelsBMP(in, pArr, DIB.width, DIB.height);
    fclose(in);

    img.pArr = pArr;
    img.width = DIB.width;
    img.height = DIB.height;

    //apply filter
    if (filter == 'b'){
        apply_blur_threads(&img);
    } else if (filter == 'c'){
        seed = generateSeed(input);
        apply_cheese_threads(&img);
    } else {
        printf("invalid filter");
        exit(1);
    }

    //write to output file
    FILE* out = fopen(output, "wb");
    writeBMPHeader(out, &BMP);
    writeDIBHeader(out, &DIB);
    writePixelsBMP(out, img.pArr, DIB.width, DIB.height);
    fclose(out);

    //free memory
    for (int i = 0; i < DIB.height; i++){
        free(pArr[i]);
    }
    free(pArr);
}

