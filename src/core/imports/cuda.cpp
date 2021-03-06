#include "El-lite.hpp"
#include "El/core/imports/cuda.hpp"

#include <cuda.h>
#include <cuda_runtime.h>
#include <cublas_v2.h>

#include <cstdlib>// getenv

namespace El
{

// Global static pointer used to ensure a single instance of the
// GPUManager class.
std::unique_ptr<GPUManager> GPUManager::instance_ = nullptr;

void InitializeCUDA(int argc, char* argv[])
{

    int numDevices = 0;
    int device = -1;
    cudaDeviceProp deviceProp;

    EL_FORCE_CHECK_CUDA(
        cudaGetDeviceCount(&numDevices));

    // Choose device by parsing command-line arguments
    // if (argc > 0) { device = std::atoi(argv[0]); }

    // Choose device by parsing environment variables
    if (device < 0)
    {
        char* env = nullptr;
        if (!env)
            env = std::getenv("SLURM_LOCALID");
        if (!env)
            env = std::getenv("MV2_COMM_WORLD_LOCAL_RANK");
        if (!env)
            env = std::getenv("OMPI_COMM_WORLD_LOCAL_RANK");

        if (env)
        {
            // Allocate devices amongst ranks in round-robin fashion
            int const localRank = std::atoi(env);
            device = localRank % numDevices;

            // If device is shared amongst MPI ranks, check its
            // compute mode
            if (localRank >= numDevices)
            {
                EL_FORCE_CHECK_CUDA(
                    cudaGetDeviceProperties(&deviceProp, device));
                switch(deviceProp.computeMode)
                {
                case cudaComputeModeExclusive:
                    cudaDeviceReset();
                    RuntimeError("Attempted to share CUDA device ",device,
                                 " amongst multiple MPI ranks, ",
                                 "but it is set to cudaComputeModeExclusive");
                    break;
                case cudaComputeModeExclusiveProcess:
                    cudaDeviceReset();
                    RuntimeError("Attempted to share CUDA device ",device,
                                 " amongst multiple MPI ranks, "
                                 "but it is set to "
                                 "cudaComputeModeExclusiveProcess");
                    break;
                }
            }
        }
    }

    // Check device compute mode
    if (device < 0)
        device = 0;

    EL_FORCE_CHECK_CUDA(
        cudaGetDeviceProperties(&deviceProp, device));

    switch(deviceProp.computeMode)
    {
    case cudaComputeModeExclusive:
    case cudaComputeModeExclusiveProcess:
        // Let CUDA handle GPU assignments
        EL_FORCE_CHECK_CUDA(cudaGetDevice(&device));
        break;
    case cudaComputeModeProhibited:
        cudaDeviceReset();
        RuntimeError("CUDA Device ",device,
                     " is set with ComputeModeProhibited");
        break;
    }

    // Instantiate CUDA manager
    GPUManager::Create(device);
}

void FinalizeCUDA()
{
    GPUManager::Destroy();
}

GPUManager::GPUManager(int device)
    : numDevices_{0}, device_{device}, stream_{nullptr}, cublasHandle_{nullptr}
{
    // Check if device is valid
    EL_FORCE_CHECK_CUDA(
        cudaGetDeviceCount(&numDevices_));

    if (device_ < 0 || device_ >= numDevices_)
    {
        RuntimeError("Attempted to set invalid CUDA device ",
                     "(requested device ",device_,", ",
                     "but there are ",numDevices_," available devices)");
    }

    // Initialize CUDA and cuBLAS objects
    EL_FORCE_CHECK_CUDA(cudaSetDevice(device_));
    EL_FORCE_CHECK_CUDA(cudaStreamCreate(&stream_));
    EL_FORCE_CHECK_CUBLAS(cublasCreate(&cublasHandle_));
    EL_FORCE_CHECK_CUBLAS(cublasSetStream(cublasHandle_, stream_));
    EL_FORCE_CHECK_CUBLAS(cublasSetPointerMode(cublasHandle_,
                                               CUBLAS_POINTER_MODE_HOST));
}


GPUManager::~GPUManager()
{
    try
    {
        EL_FORCE_CHECK_CUDA(cudaSetDevice(device_));
        if (cublasHandle_ != nullptr)
            EL_FORCE_CHECK_CUBLAS(cublasDestroy(cublasHandle_));

        if (stream_ != nullptr)
            EL_FORCE_CHECK_CUDA(cudaStreamDestroy(stream_));
    }
    catch (std::exception const& e)
    {
        std::cerr << "cudaFree error detected:\n\n"
                  << e.what() << std::endl
                  << "std::terminate() will be called."
                  << std::endl;
        std::terminate();
    }
}

void GPUManager::Create(int device)
{
    instance_.reset(new GPUManager(device));
}

void GPUManager::Destroy()
{
    instance_.reset();
}

GPUManager* GPUManager::Instance()
{
    if (!instance_)
        Create();

    EL_CHECK_CUDA(
        cudaSetDevice(instance_->device_));
    return instance_.get();
}

int GPUManager::NumDevices()
{
    return Instance()->numDevices_;
}

int GPUManager::Device()
{
    return Instance()->device_;
}

void GPUManager::SetDevice(int device)
{
    if (instance_ && instance_->device_ != device)
        Destroy();
    if (!instance_)
        Create(device);
}

cudaStream_t GPUManager::Stream()
{
    return Instance()->stream_;
}

cublasHandle_t GPUManager::cuBLASHandle()
{
    auto* instance = Instance();
    auto handle = instance->cublasHandle_;
    EL_CHECK_CUBLAS(cublasSetStream(handle, instance->stream_));
    EL_CHECK_CUBLAS(cublasSetPointerMode(handle,
                                         CUBLAS_POINTER_MODE_HOST));
    return handle;
}

} // namespace El
