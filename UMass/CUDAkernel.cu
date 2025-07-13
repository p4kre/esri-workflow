// slope_aspect.cu  (compiled with nvcc -O3 --use_fast_math)
__global__ void slopeAspect(const float* dem, float* slope,
                            float* aspect, int W, int H, float cellSz)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;  // pixel coords
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x <= 0 || y <= 0 || x >= W-1 || y >= H-1) return;

    int idx = y * W + x;
    float dzdx = (dem[idx+1] - dem[idx-1]) / (2*cellSz);
    float dzdy = (dem[idx+W] - dem[idx-W]) / (2*cellSz);
    slope [idx] = atan(sqrtf(dzdx*dzdx + dzdy*dzdy));
    aspect[idx] = atan2f(-dzdy, dzdx);
}
cudaGraph_t g; cudaStream_t s;
cudaStreamCreate(&s);
cudaStreamBeginCapture(s, cudaStreamCaptureModeGlobal);
dim3 blk(32,32), grid((W+31)/32,(H+31)/32);
slopeAspect<<<grid, blk, 0, s>>>(dDEM, dSlope, dAspect, W, H, cell);
cudaStreamEndCapture(s, &g);
cudaGraphExec_t gExec; cudaGraphInstantiate(&gExec, g, nullptr, nullptr, 0);

for (auto roi : regions) {                   // every time user selects a ROI
    cudaMemcpyAsync(dDEM, roi.data, bytes, cudaMemcpyHostToDevice, s);
    cudaGraphLaunch(gExec, s);               // launches whole chain once
    cudaMemcpyAsync(out.data, dSlope, bytes, cudaMemcpyDeviceToHost, s);
}
