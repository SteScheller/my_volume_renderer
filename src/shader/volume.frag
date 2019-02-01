#version 330 core
layout(location = 0) out vec4 fragColor;
layout(location = 1) out uvec4 stateOut;

in vec3 vTexCoord;          //!< texture coordinates
in vec3 vWorldCoord;        //!< texture coordinates

uniform sampler3D volumeTex;            //!< 3D texture handle
uniform sampler2D transferfunctionTex;  //!< 3D texture handle

uniform bool useSeed;           //!< flag if the seed texture shall be used
uniform usampler2D seed;        //!< seed texture for random number generator
uniform usampler2D stateIn;     //!< state of random number generator

uniform vec3 eyePos;            //!< camera / eye position in world coordinates
uniform vec3 bbMin;             //!< axes aligned bounding box min. corner
uniform vec3 bbMax;             //!< axes aligned bounding box max. corner

uniform int winWidth;           //!< width of the window in pixels
uniform int winHeight;          //!< height of the window in pixels

// rendering method:
//   0: line-of-sight
//   1: mip
//   2: isosurface
//   3: volume with transfer function
uniform int mode;

uniform int gradMethod;     //!< switch to select gradient calculation method

uniform float stepSize;         //!< distance between sample points between
                                //!< sample points in world coordinates
uniform float stepSizeVoxel;    //!< distance between sample points in voxels
uniform float brightness;       //!< color coefficient

uniform bool ambientOcclusion;  //!< switch for activating ambient occlusion
uniform float aoProportion;     //!< weight of ambient occlusion on final color
uniform int aoSamples;          //!< number of samples involved in ambient
                                //!< occlusion calculation
uniform float aoRadius;         //!< radius of the sampled halfdome

uniform float isovalue;         //!< value for isosurface
uniform bool isoDenoise;        //!< switch for smoothing of the isosurface
uniform float isoDenoiseR;      //!< radius for denoising

uniform vec3 lightDir;      //!< light direction

uniform vec3 ambient;       //!< ambient color
uniform vec3 diffuse;       //!< diffuse color
uniform vec3 specular;      //!< specular color

uniform float kAmb;        //!< ambient factor
uniform float kDiff;       //!< diffuse factor
uniform float kSpec;       //!< specular factor
uniform float kExp;        //!< specular exponent

uniform bool invertColors; //!< switch for inverting the color output
uniform bool invertAlpha;  //!< switch for inverting the alpha output

uniform bool sliceVolume;          //!< switch for slicing plane
uniform vec3 slicePlaneNormal;    //!< normal of slicing plane
uniform vec3 slicePlaneBase;      //!< base point of slicing plane

#define M_PIH   1.570796
#define M_PI    3.141592
#define M_2PI   6.283185
#define EPS     0.000001

// ----------------------------------------------------------------------------
// random number generator
//
// Generates uniform random numbers in the interval [0,1]
//
// - needs a 4 channel uint texture as in and output for managing its state
// - this state texture must be initialized externally with four random numbers
//   >128  + four values that are unique for that pixel
// - before use the state of the rng has to be initialized by calling initRNG()
// - random numbers can be retrieved by calling uniformRandom()
// - implemented according to:
//
// https://math.stackexchange.com/questions/337782/
//  pseudo-random-number-generation-on-the-gpu
//
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch37.html
// ----------------------------------------------------------------------------

uint z1, z2, z3, z4;

void initRNG()
{
    uvec4 stateVec = uvec4(0U);

    if (useSeed)
    {
        stateVec = texture(
            seed,
            vec2(gl_FragCoord.x / winWidth, gl_FragCoord.y / winHeight));
    }
    else
    {
        stateVec = texture(
            stateIn,
            vec2(gl_FragCoord.x / winWidth, gl_FragCoord.y / winHeight));
    }

    z1 = stateVec.x;
    z2 = stateVec.y;
    z3 = stateVec.z;
    z4 = stateVec.w;

    stateOut = stateVec;
}

uint tausStep(uint z, int S1, int S2, int S3, uint M)
{
    uint b = (((z << S1) ^ z) >> S2);

    return (((z & M) << S3) ^ b);
}

uint lcgStep(uint z, uint A, uint C)
{
    return (A * z + C);
}

float uniformRandom()
{
    z1 = tausStep(z1, 13, 19, 12, 4294967294U);
    z2 = tausStep(z2, 2, 25, 4, 4294967288U);
    z3 = tausStep(z3, 3, 11, 17, 4294967280U);
    z4 = lcgStep(z4, 1664525U, 1013904223U);

    stateOut = uvec4(z1, z2, z3, z4);

    return 2.3283064365387e-10 * float(z1 ^ z2 ^ z3 ^ z4);
}
// ----------------------------------------------------------------------------

/*!
 *  \brief samples uniform random direction vectors around in a unit half dome
 *
 *  \param n normalized vector which points to the upper pole of the half dome
 *
 *  \return normalized sampled direction vector
 *
 *  implementation is based on:
 *  https://math.stackexchange.com/questions/180418/
 *  calculate-rotation-matrix-to-align-vector-a-to-vector-b-in-3d/476311#476311
 *
 *  The coordinates are sampled using an area (and therefore uniformity)
 *  conserving mapping from spherical coordinates with height (z) and
 *  azimut angle (phi) parameters.
 */
vec3 sampleHalfdomeDirectionUpper(vec3 n)
{
    vec3 dir = vec3(0.f);
    float z = 0.f, phi = 0.f, temp = 0.f, s = 0.f, c = 0.f;
    vec3 v = vec3(0.f), a = vec3(0.f);
    mat3 V = mat3(0.f), R = mat3(0.f);

    // Sample a direction in haldome define by the y axis
    a = vec3(0, 1.f, 0.f);
    z = 2.f * uniformRandom() - 1.f;
    phi = uniformRandom() * M_2PI;
    temp = sqrt(1.f - pow(z,2));
    dir = normalize(vec3(temp * sin(phi), temp * cos(phi), z));

    // Find a rotation matrix that transforms the given normal into the sample
    // direction
    v = cross(a, dir);
    s = length(v);
    c = dot(a, dir);

    V = mat3(
            0.f,    v.z,    -v.y,   // first column
            -v.z,   0.f,    v.x,    // second column
            v.y,    -v.x,   0.f);   // third column
    R = mat3(1.f) + V + (V * V) * (1.f / (1.f + c));

    return R * n;
}

/*!
 *  \brief calculates the fragment color with the Blinn-Phong model
 *
 *  \param n normal vector pointing away from the surface
 *  \param l light vector pointing towards the light source
 *  \param v view vector pointing towards the eye
 *  \return color with ambient, diffuse and specular component
 */
vec3 blinnPhong(vec3 n, vec3 l, vec3 v)
{
    vec3 color = vec3(0.0);     // accumulated RGB color of the fragment
    vec3 h = normalize(v + l);  // halfway vector

    color = kAmb * ambient;
    color += kDiff * diffuse * max(0.f, dot(n, l));
    color += kSpec * specular * ((kExp + 2.f) / M_2PI) *
        pow(max(0.f, dot(h,n)), kExp);

    return color;
}
/*!
 *  \brief calculates and scalar ambient occlusion factor
 *
 *  \param volume sampler that contains the volume data
 *  \param pos position in the volume where the occlusion factor shall be
 *             calculated
 *  \param n normal that points towards the pole of the sampled halfdome
 *  \param r radius of the sampled halfdome
 *  \param samples number of samples that shall be taken for the calculation
 *                 of the occlusion factor
 *  \param threshold values above this will be considered as occluding
 *
 *  Samples halfdome defined by a position and a normal with a given radius
 *  and returns the ratio of samples that have a value > threshold and the
 *  amount of taken samples.
 */
float calcAmbientOcclussionFactor(
        sampler3D volume,
        vec3 pos,
        vec3 n,
        float r,
        int samples,
        float threshold)
{
    int i = 0, count = 0;
    vec3 sampleDir = vec3(0.f), sampleCoord = vec3(0.f);
    float value = 0.f, ratio = 0.f;

    for (i = 0; i < samples; ++i)
    {
        sampleDir = sampleHalfdomeDirectionUpper(n);
        sampleCoord = pos + r * sampleDir;
        value = texture(volume, sampleCoord).r;
        if (value > threshold)
            ++count;
    }

    ratio = float(count) / float(samples);

    return ratio;
}

/*!
 *  \brief computes the output color with front to back compositing
 *
 *  \param input input color as RGBA vector
 *  \param c color of the blended element
 *  \param a alpha of the blended element
 *  \param tStep current step size over basis step size used to adjust the
 *               opacity contribution
 *
 *  \return output color as RGBA vector
 */
vec4 frontToBack(vec4 inRGBA, vec3 c, float a, float tStep)
{
    vec4 outRGBA;
    float adjustedAlpha = 0.f;

    adjustedAlpha = (1.f - pow(1.f - a, tStep));

    outRGBA.rgb = inRGBA.rgb + (1.f - inRGBA.a) * c * adjustedAlpha;
    outRGBA.a = inRGBA.a + (1.f - inRGBA.a) * adjustedAlpha;

    return outRGBA;
}

/*!
 *  \brief Intersects a ray with an AABB and returns the intersection points
 *
 *  Intersection test with axes-aligned bounding box according to the slab
 *  method; It relies on IEEE 754 floating-point properties (see elementwise
 *  inverse of rayDir vector).
 *
 *  \param rayOrig The origin of the ray
 *  \param rayDir The direction of the ray
 *  \param tNear Out: distance from ray origin to first intersection point
 *  \param tFar Out: distance from ray origin to second intersection point
 *  \return True if the ray intersects the bounding box
 */
bool intersectBoundingBox(
    vec3 rayOrig, vec3 rayDir, out float tNear, out float tFar)
{
    vec3 invR = vec3(1.0) / rayDir;
    vec3 tbot = invR * (bbMin - rayOrig);
    vec3 ttop = invR * (bbMax - rayOrig);

    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);

    float largestTMin = max(max(tmin.x, tmin.y), max(tmin.x, tmin.z));
    float smallestTMax = min(min(tmax.x, tmax.y), min(tmax.x, tmax.z));

    tNear = largestTMin;
    tFar = smallestTMax;

    return (smallestTMax > largestTMin);
}

/*!
 *  \brief Intersects a ray with an infinite plane
 *
 *  Intersection test of ray with plane based on:
 *  stackoverflow.com/questions/23975555/how-to-do-ray-plane-intersection
 *
 *  \param rayOrig The origin of the ray
 *  \param rayDir The direction of the ray
 *  \param planeBase The base point of the plane
 *  \param planeNormal The normal of the plane
 *  \param t Out: distance from ray origin to the intersection point
 *  \return True if the ray intersects the plane
 */
bool intersectPlane(
        vec3 rayOrig, vec3 rayDir, vec3 planeBase, vec3 planeNormal, out float t)
{
    bool intersect = false;
    t = 0.f;

    float denom = dot(rayDir, planeNormal);
    if (abs(denom) > EPS)
    {
        t = dot(planeBase - rayOrig, planeNormal) / denom;
        if (t >= EPS) intersect = true;
    }

    return intersect;
}

/**
 *  \brief calculates the gradient with central differences
 *
 *  \param volume handle to the 3D texture
 *  \param pos The position from which the gradient should be determined
 *  \param h distance for finite differences
 *  \return The gradient at pos
 *
 *  Calculates the gradient at a given position in a 3D volume using central
 *  differences.
 */
vec3 gradientCentral(sampler3D volume, vec3 pos, float h)
{
    vec3 grad = vec3(0.0);      // gradient vector

    grad.x = texture(volume, pos + vec3(h, 0.f, 0.f)).r -
        texture(volume, pos - vec3(h, 0.f, 0.f)).r;
    grad.y = texture(volume, pos + vec3(0.f, h, 0.f)).r -
        texture(volume, pos - vec3(0.f, h, 0.f)).r;
    grad.z = texture(volume, pos + vec3(0.f, 0.f, h)).r -
        texture(volume, pos - vec3(0.f, 0.f, h)).r;
    grad /= 2.0 * h;

    return normalize(grad);
}

/**
 *  \brief calculates the gradient with central differences
 *
 *  \param volume handle to the 3d texture
 *  \param pos the position from which the gradient should be determined
 *  \param h distance for finite differences
 *  \return the gradient at pos
 *
 *  calculates the gradient at a given position in a 3d volume using sobel
 *  operators.
 */
 vec3 gradientSobel(sampler3D volume, vec3 pos, float h)
{
    vec3 grad;
    mat3 smoothing = mat3(
            vec3(1.f, 2.f, 1.f), vec3(2.f, 4.f, 2.f), vec3(1.f, 2.f, 1.f));

    /* surrounding value name scheme:
     * v_m(inus)    v_z(ero)    v_p(lus)
     *     |            |           |
     *   pos-h         pos        pos+h
     *
     * Multiple suffixes used to indicate offset in the different directions!
     */
    float v_mmm = texture(volume, pos + vec3(-h,    -h,     -h)).r;
    float v_mmz = texture(volume, pos + vec3(-h,    -h,     0.f)).r;
    float v_mmp = texture(volume, pos + vec3(-h,    -h,     h)).r;
    float v_mzm = texture(volume, pos + vec3(-h,    0.f,    -h)).r;
    float v_mzz = texture(volume, pos + vec3(-h,    0.f,    0.f)).r;
    float v_mzp = texture(volume, pos + vec3(-h,    0.f,    h)).r;
    float v_mpm = texture(volume, pos + vec3(-h,    h,      -h)).r;
    float v_mpz = texture(volume, pos + vec3(-h,    h,      0.f)).r;
    float v_mpp = texture(volume, pos + vec3(-h,    h,      h)).r;

    float v_zmm = texture(volume, pos + vec3(0.f,   -h,     -h)).r;
    float v_zmz = texture(volume, pos + vec3(0.f,   -h,     0.f)).r;
    float v_zmp = texture(volume, pos + vec3(0.f,   -h,     h)).r;
    float v_zzm = texture(volume, pos + vec3(0.f,   0.f,    -h)).r;
    float v_zzp = texture(volume, pos + vec3(0.f,   0.f,    h)).r;
    float v_zpm = texture(volume, pos + vec3(0.f,   h,      -h)).r;
    float v_zpz = texture(volume, pos + vec3(0.f,   h,      0.f)).r;
    float v_zpp = texture(volume, pos + vec3(0.f,   h,      h)).r;

    float v_pmm = texture(volume, pos + vec3(h,     -h,     -h)).r;
    float v_pmz = texture(volume, pos + vec3(h,     -h,     0.f)).r;
    float v_pmp = texture(volume, pos + vec3(h,     -h,     h)).r;
    float v_pzm = texture(volume, pos + vec3(h,     0.f,    -h)).r;
    float v_pzz = texture(volume, pos + vec3(h,     0.f,    0.f)).r;
    float v_pzp = texture(volume, pos + vec3(h,     0.f,    h)).r;
    float v_ppm = texture(volume, pos + vec3(h,     h,      -h)).r;
    float v_ppz = texture(volume, pos + vec3(h,     h,      0.f)).r;
    float v_ppp = texture(volume, pos + vec3(h,     h,      h)).r;


    grad.x =
        (dot(smoothing[0], vec3(v_pmm, v_pmz, v_pmp)) +
         dot(smoothing[1], vec3(v_pzm, v_pzz, v_pzp)) +
         dot(smoothing[2], vec3(v_ppm, v_ppz, v_ppp))) -
        (dot(smoothing[0], vec3(v_mmm, v_mmz, v_mmp)) +
         dot(smoothing[1], vec3(v_mzm, v_mzz, v_mzp)) +
         dot(smoothing[2], vec3(v_mpm, v_mpz, v_mpp)));

    grad.y =
        (dot(smoothing[0], vec3(v_mpm, v_mpz, v_mpp)) +
         dot(smoothing[1], vec3(v_zpm, v_zpz, v_zpp)) +
         dot(smoothing[2], vec3(v_ppm, v_ppz, v_ppp))) -
        (dot(smoothing[0], vec3(v_mmm, v_mmz, v_mmp)) +
         dot(smoothing[1], vec3(v_zmm, v_zmz, v_zmp)) +
         dot(smoothing[2], vec3(v_pmm, v_pmz, v_pmp)));

    grad.z =
        (dot(smoothing[0], vec3(v_mmp, v_mzp, v_mpp)) +
         dot(smoothing[1], vec3(v_zmp, v_zzp, v_zpp)) +
         dot(smoothing[2], vec3(v_pmp, v_pzp, v_ppp))) -
        (dot(smoothing[0], vec3(v_mmm, v_mzm, v_mpm)) +
         dot(smoothing[1], vec3(v_zmm, v_zzm, v_zpm)) +
         dot(smoothing[2], vec3(v_pmm, v_pzm, v_ppm)));

    return normalize(grad);
}

/**
 *  \brief wrapper that calculated the gradient with different methods
 *
 *  \param volume handle to the 3d texture
 *  \param pos the position from which the gradient should be determined
 *  \param h distance for finite differences
 *  \param h distance for finite differences
 *  \return the gradient at pos
 *
 *  calculates the gradient at a given position using one of the following
 *  methods:
 *      1 = sobel operators
 *      0 = central differences
 */
vec3 gradient(sampler3D volume, vec3 pos, float h, int method)
{
    vec3 grad = vec3(0.0);      // gradient vector

    switch(method)
    {
        case 1:
            grad = gradientSobel(volume, pos, h);
            break;

        default:
            grad = gradientCentral(volume, pos, h);
            break;
    }

    return grad;
}

/**
 *  \brief returns an average value of the volume around pos
 *
 *  \param volume handle to the 3d texture
 *  \param pos central volume postion where the value shall be calculated
 *  \param r radius of the averaged sphere
 *  \return averaged value
 *
 *  Calculates the average value of the volume around a given position by
 *  sampling a sphere with paramatrizable radius at fixed positions.
 */
float denoiseSphereAvg(sampler3D volume, vec3 pos, float r)
{
    float avg = 4.f * texture(volume, pos).r;
    float sqrt_rr_by_2 = sqrt(r * r / 2.f);
    float r_by_2 = r / 2.f;

    avg += 2.f * texture(volume, pos + vec3(-r, 0.f, 0.f)).r;
    avg += 2.f * texture(volume, pos + vec3(r, 0.f, 0.f)).r;
    avg += 2.f * texture(volume, pos + vec3(0.f, -r, 0.f)).r;
    avg += 2.f * texture(volume, pos + vec3(0.f, r, 0.f)).r;
    avg += 2.f * texture(volume, pos + vec3(0.f, 0.f, -r)).r;
    avg += 2.f * texture(volume, pos + vec3(0.f, 0.f, r)).r;

    avg += texture(volume, pos + vec3(r_by_2,   sqrt_rr_by_2,   r_by_2)).r;
    avg += texture(volume, pos + vec3(-r_by_2,  sqrt_rr_by_2,   r_by_2)).r;
    avg += texture(volume, pos + vec3(r_by_2,   sqrt_rr_by_2,   -r_by_2)).r;
    avg += texture(volume, pos + vec3(-r_by_2,  sqrt_rr_by_2,   -r_by_2)).r;
    avg += texture(volume, pos + vec3(r_by_2,   -sqrt_rr_by_2,  r_by_2)).r;
    avg += texture(volume, pos + vec3(-r_by_2,  -sqrt_rr_by_2,  r_by_2)).r;
    avg += texture(volume, pos + vec3(r_by_2,   -sqrt_rr_by_2,  -r_by_2)).r;
    avg += texture(volume, pos + vec3(-r_by_2,  -sqrt_rr_by_2,  -r_by_2)).r;

    return (avg / 24.f);
}

// ----------------------------------------------------------------------------
//   main
// ----------------------------------------------------------------------------
void main()
{
    vec4 color = vec4(0.f); //!< RGBA color of the fragment
    float value = 0.f;      //!< value sampled from the volume
    float denoisedValue = 0.f;      //!< denoised value from the volume

    bool intersect = false; //!< flag if we did hit the bounding box
    float temp = 0.f;       //!< temporary variable
    vec3 volCoord = vec3(0.f);      //!< coordinates for texture access

    bool terminateEarly = false;    //!< early ray termination

    vec3 pos = vec3(0.f);   //!< current position on the ray in world
                            //!< coordinates
    float x = 0.f;          //!< distance from origin to current position
    float dx = stepSize;    //!< distance between sample points along the ray
    float tNear, tFar;      //!< near and far distances where the shot ray
                            //!< intersects the bounding box of the volume data

    vec3 rayOrig = eyePos;                          //!< origin of the ray
    vec3 rayDir = normalize(vWorldCoord - rayOrig); //!< direction of the ray

    float aoFactor = 0.f;   //! multiplicative factor for ambient occlusion

    // maximum intensity projection
    float maxValue = 0.f;

    // isosurface
    float valueLast = 0.f;          //!< temporary variable for storing the
                                    //!< last sample value
    vec3 posLast = rayOrig;         //!< temporary variable for storing the
                                    //!< last sampled position on the ray

    // Blinn-Phong shading
    vec3 p = vec3(0.f);             //!< position on the isosurface in world
                                    //!< coordinates
    vec3 pTexCoord = vec3(0.f);     //!< position on the isosurface in volume
                                    //!< texture coordinates
    vec3 n = vec3(0.f);             //!< surface normal pointing away from the
                                    //!< surface
    vec3 l = lightDir;              //!< direction of the distant light source
                                    //!< in world coordinates
    vec3 e = -rayDir;               //!< direction of the (virtual) eye in
                                    //!< world coordinates

    // transfer function
    vec4 tfColor = vec4(0.f);       //!< color value from the transferfunction

    // initialize random number generator
    initRNG();

    // intersect with bounding box and handle special case when we are inside
    // the box. In this case the ray marching starts directly at the origin.
    volCoord = rayOrig - bbMin;
    if ((volCoord.x < bbMax.x) &&
        (volCoord.y < bbMax.y) &&
        (volCoord.z < bbMax.z))
    {
        // we are within the volume -> nevertheless make an intersection test
        // to get the exit point
        intersect = true;
        intersectBoundingBox(rayOrig, rayDir, temp, tFar);
        tNear = 0.f;
    }
    else
    {
        intersect = intersectBoundingBox(rayOrig, rayDir, tNear, tFar);
    }

    if (!intersect)
    {
        // we do not hit the volume
        fragColor = vec4(0.f);
        return;
    }
    else
    {
        // we do hit the volume -> check if it shall be sliced and update
        // the starting position accordingly
        if (sliceVolume)
        {
            if (intersectPlane(
                        rayOrig,
                        rayDir,
                        slicePlaneBase,
                        slicePlaneNormal,
                        temp))
            {
                // we intersect the slicing plane update the starting point
                // to show everything behind the plane
                tNear = temp;
            }
            else
                tNear = tFar + dx;
        }
    }

    for (x = tNear; x <= tFar; x += dx)
    {
        pos = rayOrig + x * rayDir;
        volCoord = (pos - bbMin) / (bbMax - bbMin);

        // Check if we are in the volume (precision issue)
        int continueCount = 0;
        const int continueTolerance = 1;
        if ((volCoord.x < 0.f) ||
            (volCoord.y < 0.f) ||
            (volCoord.z < 0.f) ||
            (volCoord.x > 1.f) ||
            (volCoord.y > 1.f) ||
            (volCoord.z > 1.f))

        {
            if (continueCount < continueTolerance)
                continue;
            ++continueCount;
            fragColor = vec4(1.f, 0.f, 1.f, 1.f);
            return;
        }

        value = texture(volumeTex, volCoord).r;

        switch (mode)
        {
            // line-of-sight integration
            case 0:
                color.rgb += vec3(value * dx);
                color.a = 1.f;
                if (color.r > 0.99f)
                {
                    if(ambientOcclusion)
                    {
                        pTexCoord = (pos - bbMin) / (bbMax - bbMin);
                        n = -gradient(volumeTex, pTexCoord, dx, gradMethod);
                        aoFactor = calcAmbientOcclussionFactor(
                            volumeTex,
                            pTexCoord,
                            n,
                            aoRadius,
                            aoSamples,
                            value);
                        color.rgb = mix(
                            color.rgb, aoFactor * color.rgb, aoProportion);
                    }
                    terminateEarly = true;
                }
                break;

            // maximum-intensity projection
            case 1:
                if (value > maxValue)
                {
                    maxValue = value;
                    color.rgb = vec3(value);
                    color.a = 1.f;
                    if (color.r > 0.99f)
                    {
                        if(ambientOcclusion)
                        {
                            pTexCoord = (pos - bbMin) / (bbMax - bbMin);
                            n = -gradient(volumeTex, pTexCoord, dx, gradMethod);
                            aoFactor = calcAmbientOcclussionFactor(
                                volumeTex,
                                pTexCoord,
                                n,
                                aoRadius,
                                aoSamples,
                                value);
                            color.rgb = mix(
                                color.rgb, aoFactor * color.rgb, aoProportion);
                        }
                        terminateEarly = true;
                    }
                }
                break;

            // isosurface
            case 2:
                if (0.f > ((value - isovalue) * (valueLast - isovalue)))
                {
                    if(isoDenoise)
                    {
                        // check if we still cross the isovalue after denoising
                        denoisedValue = denoiseSphereAvg(
                                volumeTex, volCoord, isoDenoiseR);
                        if (!(0.f > (
                                (denoisedValue - isovalue) *
                                (valueLast - isovalue))))
                        {
                            // after denoising we do not cross the isovalue
                            valueLast = denoisedValue;
                            posLast = pos;
                            break;
                        }
                    }

                    p = mix(
                        posLast,
                        pos,
                        (isovalue - valueLast) / (value - valueLast));
                    pTexCoord = (p - bbMin) / (bbMax - bbMin);
                    n = -gradient(volumeTex, pTexCoord, dx, gradMethod);

                    color.rgb = blinnPhong(n, l, e);
                    color.a = 1.f;

                    if(ambientOcclusion)
                    {
                        aoFactor = calcAmbientOcclussionFactor(
                            volumeTex,
                            pTexCoord,
                            n,
                            aoRadius,
                            aoSamples,
                            isovalue);
                        color.rgb = mix(
                            color.rgb, aoFactor * color.rgb, aoProportion);
                    }
                    terminateEarly = true;
                }
                valueLast = value;
                posLast = pos;
                break;

            // transfer function
            case 3:
                tfColor = texture(
                    transferfunctionTex,
                    vec2(value, 0.5f));
                color = frontToBack(
                    color,
                    tfColor.rgb, tfColor.a, stepSizeVoxel);
                if (color.a > 0.99f)
                {
                    if(ambientOcclusion)
                    {
                        pTexCoord = (pos - bbMin) / (bbMax - bbMin);
                        n = -gradient(volumeTex, pTexCoord, dx, gradMethod);
                        aoFactor = calcAmbientOcclussionFactor(
                            volumeTex,
                            pTexCoord,
                            n,
                            aoRadius,
                            aoSamples,
                            value);
                        color.rgb = mix(
                            color.rgb, aoFactor * color.rgb, aoProportion);
                    }
                    terminateEarly = true;
                }
                break;
        }

        if (terminateEarly)
            break;
    }

    if (invertColors)
        color.rgb = vec3(1.f) - color.rgb;

    if (invertAlpha)
        color.a = 1.f - color.a;

    fragColor = vec4(brightness * color.rgb, color.a);
}

