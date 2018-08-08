#version 330 core
layout(location = 0) out vec4 frag_color;

in vec3 vTexCoord;      //!< texture coordinates
in vec3 vWorldCoord;      //!< texture coordinates

uniform sampler3D volumeTex;    //!< 3D texture handle

uniform vec3 eyePos;            //!< camera / eye position in world coordinates
uniform vec3 bbMin;             //!< axes aligned bounding box min. corner
uniform vec3 bbMax;             //!< axes aligned bounding box max. corner

// rendering method:
//   0: line-of-sight
//   1: mip
//   2: isosurface
//   3: volume with transfer function
uniform int mode;

uniform float step_size;    //!< distance between sample points along the ray
uniform float brightness;   //!< color coefficient

uniform float isovalue;     //!< value for isosurface

uniform vec3 lightDir;      //!< light direction

uniform vec3 ambient;       //!< ambient color
uniform vec3 diffuse;       //!< diffuse color
uniform vec3 specular;      //!< specular color

uniform float k_amb;        //!< ambient factor
uniform float k_diff;       //!< diffuse factor
uniform float k_spec;       //!< specular factor
uniform float k_exp;        //!< specular exponent

#define M_PIH   1.570796
#define M_PI    3.141592
#define M_2PI   6.283185
#define EPS     0.000001

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

    color = k_amb * ambient;
    color += k_diff * diffuse * max(0.f, dot(n, l));
    color += k_spec * specular * ((k_exp + 2.f) / M_2PI) *
        pow(max(0.f, dot(h,n)), k_exp);

    return color;
}

/*!
 *  \brief computes the output color with front to back compositing
 *
 *  \param input input color as RGBA vector
 *  \param c color of the blended element
 *  \param a alpha of the blended element
 *  \return output color as RGBA vector
 */
vec4 frontToBack(vec4 inRGBA, vec3 c, float a)
{
    vec4 outRGBA;

    outRGBA.rgb = inRGBA.rgb + (1 - inRGBA.a) * c * a;
    outRGBA.a = inRGBA.a + (1 - inRGBA.a) * a;

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

/**
 *	\brief calculates the gradient with central differences
 *
 *	\param volume handle to the 3D texture
 *	\param pos The position from which the gradient should be determined
 *	\param h distance for finite differences
 *	\return The gradient at pos
 *
 *	Calculates the gradient at a given position in a 3D volume using central
 *	differences.
 */
vec3 gradientCentral(sampler3D volume, vec3 pos, float h)
{
	vec3 grad = vec3(0.0);		// gradient vector

    grad.x = texture(volume, pos + vec3(h, 0.f, 0.f)).r -
        texture(volume, pos - vec3(h, 0.f, 0.f)).r;
    grad.y = texture(volume, pos + vec3(0.f, h, 0.f)).r -
        texture(volume, pos - vec3(0.f, h, 0.f)).r;
    grad.z = texture(volume, pos + vec3(0.f, 0.f, h)).r -
        texture(volume, pos - vec3(0.f, 0.f, h)).r;
    grad /= 2.0 * h;

    return normalize(grad);
}
// ----------------------------------------------------------------------------
//   main
// ----------------------------------------------------------------------------
void main()
{
    vec4 color = vec4(0.f); //!< RGBA color of the fragment
    float value = 0.f;      //!< value sampled from the volume

    vec3 volCoord = vec3(0.f);      //!< coordinates for texture access

    bool terminateEarly = false;    //!< early ray termination

    vec3 pos = vec3(0.f);   //!< current position on the ray in world
                            //!< coordinates
    float x = 0.f;          //!< distance from origin to current position
    float dx = step_size;   //!< distance between sample points along the ray
    float tNear, tFar;      //!< near and far distances where the shot ray
                            //!< intersects the bounding box of the volume data

    vec3 rayOrig = eyePos;                          //!< origin of the ray
    vec3 rayDir = normalize(vWorldCoord - rayOrig); //!< direction of the ray

    // maximum intensity projection
    float maxValue = 0.f;

    // iso-surface
    float valueLast = 0.f;	        //!< temporary variable for storing the
                                    //!< last sample value
    vec3 posLast = rayOrig;			//!< temporary variable for storing the
                                    //!< last sampled position on the ray

    // Blinn-Phong shading
    vec3 p = vec3(0.f);				//!< position on the iso-surface in world
                                    //!< coordinates
    vec3 n = vec3(0.f);				//!< surface normal pointing away from the
                                    //!< surface
    vec3 l = lightDir;				//!< direction of the distant light source
                                    //!< in world coordinates
    vec3 e = -rayDir;				//!< direction of the (virtual) eye in
                                    //!< world coordinates

    // intersect with bounding box and handle special case when we are inside
    // the box. In this case the ray marching starts directly at the origin.
    if (!intersectBoundingBox(rayOrig, rayDir, tNear, tFar))
        tNear = 0.f;

    for (x = tNear; x <= tFar; x += dx)
    {
        pos = rayOrig + x * rayDir;
        volCoord = (pos - bbMin) / (bbMax - bbMin);
        value = texture(volumeTex, volCoord).r;

        switch (mode)
        {
            // line-of-sight integration
            case 0:
                color.rgb += vec3(value * dx);
                color.a = 1.f;
                break;

            // maximum-intensity projection
            case 1:
                if (value > maxValue)
                {
                    maxValue = value;
                    color.rgb = vec3(value);
                    color.a = 1.f;
                }
                break;

            // isosurface
            case 2:
                if (0.f > ((value - isovalue) * (valueLast - isovalue)))
                {
                    p = mix(
                        posLast,
                        pos,
                        (isovalue - valueLast) / (value - valueLast));

                    n = -gradientCentral(
                        volumeTex,
                        (p - bbMin) / (bbMax - bbMin),
                        dx);

                    color.rgb = blinnPhong(n, l, e);
                    color.a = 1.f;
                    terminateEarly = true;
                }
                valueLast = value;
                posLast = pos;
                break;

            // transfer function
            case 3:
                //TODO
                color = frontToBack(color, vTexCoord, 1.f);
                if (color.a > 0.99f)
                    terminateEarly = true;
                break;
        }

        if (terminateEarly)
            break;
    }

    frag_color = vec4(brightness * color.rgb, color.a);
}

