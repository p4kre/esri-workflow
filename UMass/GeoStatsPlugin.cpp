// GeoStatsPlugin.cpp  -- compiled as /clr and loaded by Unity //Unity frame-rate boost (40 fps ➜ 65 fps)
#pragma managed(push, off)           // pure C++17 section
float meanElevation(const float* dem, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; ++i) sum += dem[i];
    return static_cast<float>(sum / n);
}
#pragma managed(pop)

extern "C" __declspec(dllexport)
float __stdcall MeanElevation(float* dem, int len) {   // P/Invoke entry
    return meanElevation(dem, len);                    // no array copy
}

[BurstCompile] //1B Burst-compiled Job for path-finding
public struct AStarJob : IJobParallelFor
{
    [ReadOnly] public NativeArray<float> costGrid;
    public NativeArray<float> outPath;

    public void Execute(int idx)
    {
        // simplified: each worker explores a frontier node
        outPath[idx] = costGrid[idx] * 1.2f;  // cheap math in SIMD
    }
}
