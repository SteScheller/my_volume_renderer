#version 330 core
layout(location = 0) out vec4 frag_color;

in vec3 vTexCoord;      //!< texture coordinates
in vec3 vWorldCoord;    //!< position of the fragment in world coordinates

uniform sampler3D volumeTex;    //!< 3D texture handle

uniform mat4 viewMX;            //!< view matrix
uniform mat4 modelMX;           //!< model matrix
uniform mat4 invModelViewMX;    //!< inverse model-view matrix
uniform vec3 camPos;            //!< camera position in world coordinates

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

#define M_PI  3.14159265
#define M_2PI  6.2831853
#define EPS 0.000001

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
 *  \brief Intersects a ray with the bounding box and returns the intersection points
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

// ----------------------------------------------------------------------------
//   main
// ----------------------------------------------------------------------------
void main()
{
    vec4 color = vec4(0.f); //!< RGBA color of the fragment
    float value = 0.f;      //!< value sampled from the volume

    vec3 pos = vec3(0.f);   //!< current position on the ray in world
                            //!< coordinates
    float x = 0.f;          //!< distance from origin to current position
    float dx = step_size;   //!< distance between sample points along the ray
    float t_near, t_far;    //!< near and far distances where the shot ray
                            //!< intersects the bounding box of the volume data

    vec3 rayOrig = camPos;                          //!< origin of the ray
    vec3 rayDir = normalize(vWorldCoord - rayOrig); //!< direction of the ray

    // maximum intensity projection
    float maxValue = 0.0;
    
    // iso-surface 
    float sampleValueLast = 0.0;	//!< temporary variable for storing the
                                    //!< last sample value
    vec3 posLast = rayOrig;			//!< temporary variable for storing the 
                                    //!< last sampled position on the ray 
    
    // Blinn-Phong shading
    vec3 p = vec3(0.0);				//!< position on the iso-surface in world
                                    //!< coordinates
    vec3 n = vec3(0.0);				//!< surface normal pointing away from the
                                    //!< surface
    vec3 l = lightDir;				//!< direction of the distant light source
                                    //!< in world coordinates
    vec3 e = -rayDir;				//!< direction of the (virtual) eye in 
                                    //!< world coordinates

    for (x = tNear; x <= tFar; x += dx)
    {
        pos = rayOrig + x * rayDir;
        value = texture(volumeTex, pos - bbMin).r;

        switch (mode)
        {
            // line-of-sight integration
            case 0: 
                color.rgb += vec3(value * brightness * dx);
                color.a = 1.f;
                break;

            // maximum-intensity projection
            case 1:
                if (value > maxValue)
                {
                    maxValue = value;
                    color = vec3(// TODO: continue
                for (int i = 0; i <= maxSteps; i++)
                {
                    pos = pHitObj + i * step * rayDirObj;
                    curVal = texture(volume, pos).r;
                    if (curVal > maxVal) maxVal = curVal;
                    color += texture(volume, pos).r * vec3(1.f);
                }
                color = scale * maxVal * vec3(1.f);
                frag_color = vec4(color, 1.0f);
                break;

            // isosurface
            // TODO: Continue here
            case 2:
                float phiP1 = 0.f, phiP2 = 0.f;
                for (int i = 0; i <= maxSteps; i++)
                {
                    pos = pHitObj + i * step * rayDirObj;
                    phiP1 =  texture(volume, pos).r;
                    phiP2 =  texture(volume, pos + step * rayDirObj).r;
                    // check if we exceed the isosurface threshold value
                    if (0.f > ((phiP1 - isovalue) * (phiP2 - isovalue)))
                    {
                        // iso-surface exceeded -> do Blinn-Phong shading to determine the color
                        // calculate interpolated position
                        float delta = (isovalue - phiP1) / (phiP2 - phiP1);
                        vec3 p =  pos + delta * (step * rayDirObj);

                        // calculate normal
                        vec3 n = vec3(
                            textureOffset(volume, p, ivec3(-1, 0, 0)).r - isovalue,
                            textureOffset(volume, p, ivec3(0, -1, 0)).r - isovalue,
                                        textureOffset(volume, p, ivec3(0, 0, -1)).r - isovalue);
                        n = normalize(n);

                        // actual shading
                        color = blinnPhong(n,normalize(-1.f * lightDir), normalize(pCamObj - p));
                        break;
                    }
                }
                break;
        }
    }
    frag_color = color;
}

