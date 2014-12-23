// Copyright Hugh Perkins 2013, 2014 hughperkins at gmail
//
// This Source Code Form is subject to the terms of the Mozilla Public License, 
// v. 2.0. If a copy of the MPL was not distributed with this file, You can 
// obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <cstdlib>
#include <cassert>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdexcept>

#include "clew/include/clew.h"

class OpenCLHelper {
public:
     cl_int error;  

    cl_uint num_platforms;
    cl_platform_id platform_id;
    cl_context context;
    cl_command_queue queue;
    cl_device_id device;
    cl_program program;

    cl_uint num_devices;

    int gpuIndex;

    class CLArrayFloat *arrayFloat(int N );
    class CLArrayInt *arrayInt(int N );
    class CLIntWrapper *intWrapper(int N, int *source );
    class CLFloatWrapper *floatWrapper(int N, float *source );

    static bool isOpenCLAvailable() {
        return 0 == clewInit();
    }

    ~OpenCLHelper() {
//        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);        
    }

    static int roundUp( int quantization, int minimum ) {
        int size = ( minimum / quantization) * quantization;
        if( size < minimum ) {
            size += quantization;
        }
        return size;
    }

    static int getPower2Upperbound( int value ) {
        int upperbound = 1;
        while( upperbound < value ) {
            upperbound <<= 1;
        }
        return upperbound;
    }

    OpenCLHelper(int gpuindex ) {
        bool clpresent = 0 == clewInit();
        if( !clpresent ) {
            throw std::runtime_error("OpenCL library not found");
        }

//        std::cout << "this: " << this << std::endl;
        this->gpuIndex = gpuindex;
        error = 0;

        // Platform
        error = clGetPlatformIDs(1, &platform_id, &num_platforms);
//        std::cout << "num platforms: " << num_platforms << std::endl;
//        assert (num_platforms == 1);
        if (error != CL_SUCCESS) {
           std::cout << "Error getting platforms ids: " << errorMessage(error) << std::endl;
           exit(error);
        }
        if( num_platforms == 0 ) {
           std::cout << "Error: no platforms available" << std::endl;
           exit(-1);
        }

        error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device, &num_devices);
        if (error != CL_SUCCESS) {
           std::cout << "Error getting device ids: " << errorMessage(error) << std::endl;
           exit(error);
        }
  //      std::cout << "num devices: " << num_devices << std::endl;
        cl_device_id *device_ids = new cl_device_id[num_devices];
        error = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, num_devices, device_ids, &num_devices);
        if (error != CL_SUCCESS) {
           std::cout << "Error getting device ids: " << errorMessage(error) << std::endl;
           exit(error);
        }
        if( gpuindex >= num_devices ) {
           std::cout << "requested gpuindex " << gpuindex << " goes beyond number of available device " << num_devices << std::endl;
           exit(-1);
        }
        device = device_ids[gpuindex];
//        std::cout << "using device: " << device << std::endl;
        delete[] device_ids;

        // Context
        context = clCreateContext(0, 1, &device, NULL, NULL, &error);
        if (error != CL_SUCCESS) {
           std::cout << "Error creating context: " << errorMessage(error) << std::endl;
           exit(error);
        }
        // Command-queue
        queue = clCreateCommandQueue(context, device, 0, &error);
        if (error != CL_SUCCESS) {
           std::cout << "Error creating command queue: " << errorMessage(error) << std::endl;
           exit(error);
        }

    }

    void finish() {
        error = clFinish( queue );
        switch( error ) {
            case CL_SUCCESS:
                break;
            case -36:
                std::cout << "Invalid command queue: often indicates out of bounds memory access within kernel" << std::endl;
                exit(-1);
            default:
                checkError(error);                
        }
    }

    class CLKernel *buildKernel( std::string kernelfilepath, std::string kernelname );

    int getComputeUnits() {
        return (int)getDeviceInfoInt(CL_DEVICE_MAX_COMPUTE_UNITS);
    }

    int getLocalMemorySize() {
        return (int)getDeviceInfoInt(CL_DEVICE_LOCAL_MEM_SIZE);
    }

    int getMaxWorkgroupSize() {
        return (int)getDeviceInfoInt(CL_DEVICE_MAX_WORK_GROUP_SIZE);
    }



static std::string errorMessage(cl_int error ) {
    return toString(error);
}

static void checkError( cl_int error ) {
    if( error != CL_SUCCESS ) {
        std::cout << "error: " << error << std::endl;
        //assert (false);
        exit(-1);
    }
}

private:


static std::string getFileContents( std::string filename ) {
    char * buffer = 0;
    long length;
    FILE * f = fopen (filename.c_str(), "rb");

    std::string returnstring = "";
    if (f)
    {
      fseek (f, 0, SEEK_END);
      length = ftell (f);
      fseek (f, 0, SEEK_SET);
      buffer = new char[length + 1];
      if (buffer) {
        int bytesread = fread (buffer, 1, length, f);
        if( bytesread != length ) {
            std::cout << "Failed to read cl source file" << std::endl;
            exit(-1);
        }
      } else {
        std::cout << "Failed to allocate memory for cl source" << std::endl;
        exit(-1);
       }
      fclose (f);
        buffer[length] = 0;
      returnstring = std::string( buffer );
      delete[] buffer;
    }
    return returnstring;
}

template<typename T>
static std::string toString(T val ) {
   std::ostringstream myostringstream;
   myostringstream << val;
   return myostringstream.str();
}

long getDeviceInfoInt( cl_device_info name ) {
    cl_ulong value = 0;
    clGetDeviceInfo(device, name, sizeof(cl_ulong), &value, 0);
    return value;
}

};

#include "CLKernel.h"
#include "CLIntWrapper.h"
#include "CLFloatWrapper.h"


