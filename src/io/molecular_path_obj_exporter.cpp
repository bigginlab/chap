#include <cmath>
#include <vector>

#include <gromacs/math/vec.h>
#include <gromacs/utility/real.h> 

#include "io/molecular_path_obj_exporter.hpp"


/*
 *
 */
RegularVertexGrid::RegularVertexGrid(
        std::vector<real> s,
        std::vector<real> phi)
    : s_(s)
    , phi_(phi)
{
    
}


/*
 *
 */
void
RegularVertexGrid::addVertex(size_t i, size_t j, gmx::RVec vertex)
{
    std::pair<size_t, size_t> key(i, j);
    vertices_[key] = vertex;

/*
    std::cout<<"i = "<<i<<"  "
             <<"j = "<<j<<"  "
             <<"x = "<<vertices_[key][0]<<"  "
             <<"y = "<<vertices_[key][1]<<"  "
             <<"z = "<<vertices_[key][2]<<"  "
             <<"key.1 = "<<key.first<<"  "
             <<"key.2 = "<<key.second<<"  "
             <<std::endl;*/
}


/*
 *
 */
void
RegularVertexGrid::addVertexNormal(size_t i, size_t j, gmx::RVec normal)
{
    std::pair<size_t, size_t> key(i, j);
    normals_[key] = normal;
}


/*
 *
 */
void
RegularVertexGrid::interpolateMissing()
{
    int nMissing = 0;

    // loop over grid points:
    for(size_t i = 1; i < s_.size(); i *= 2)
    {
        for(size_t j = 1; j < i; j += 2)
        {
            int idxLen = j*(s_.size() - 1)/i;

            for(size_t k = 0; k < phi_.size(); k++)
            {
                // is value present?
                std::pair<size_t, size_t> key(idxLen, k);
                auto it = vertices_.find(key);
                if( it == vertices_.end() )
                {
                    nMissing++;

                    gmx::RVec vert = linearInterp(idxLen, k);
                    vertices_[key] = vert;
    /*                std::cout<<"vert_x = "<<vert[XX]<<"  "
                             <<"vert_y = "<<vert[YY]<<"  "
                             <<"vert_z = "<<vert[ZZ]<<"  "
                             <<"vert_x = "<<vertices_[key][XX]<<"  "
                             <<"vert_y = "<<vertices_[key][YY]<<"  "
                             <<"vert_z = "<<vertices_[key][ZZ]<<"  "
                             <<std::endl;*/
                }
            }
        }
    }

    std::cout<<"nMissing = "<<nMissing<<std::endl;
}


/*
 *
 */
gmx::RVec
RegularVertexGrid::linearInterp(size_t idxS, size_t idxPhi)
{
    // nearest neighbours and interpolated vertex:
    gmx::RVec upperVertex;
    gmx::RVec lowerVertex;
    gmx::RVec interpVertex(0, 0, 0);

    // position of neighbouring verteices along the centre line:
    real sHi;
    real sLo;

    // find nearest present neighbour at higher index:
    for(size_t i = idxS; i < s_.size(); ++i)
    {

        // check if this vertex is present:
        std::pair<size_t, size_t> key(i, idxPhi);
        auto it = vertices_.find(key);
        if( it != vertices_.end() )
        {
            upperVertex = it -> second;
            sHi = s_[i];
            break;
        }
    }

    // find nearest present neighbour at lower index:
    for(int i = idxS; i > 0 ; --i)
    {

        // check if this vertex is present:
        std::pair<size_t, size_t> key(i, idxPhi);
        auto it = vertices_.find(key);        
        if( it != vertices_.end() )
        {
            lowerVertex = it -> second;
            sLo = s_[i];
            break;
        }
    }

    // linearly interpolate between the two vectors:
    real gamma = std::fabs((s_[idxS] - sLo)/(sHi - sLo));
    svmul((1.0 - gamma), upperVertex, upperVertex);
    svmul(gamma, lowerVertex, lowerVertex);
    rvec_add(lowerVertex, upperVertex, interpVertex); 
    std::cout<<"gamma = "<<gamma<<"  "
             <<"sLo = "<<sLo<<"  "
             <<"sHi = "<<sHi<<"  "
             <<"sMi = "<<s_[idxS]<<"  "
             <<"xl = "<<lowerVertex[XX]<<"  "
             <<"yl = "<<lowerVertex[YY]<<"  "
             <<"zl = "<<lowerVertex[ZZ]<<"  "
             <<"xu = "<<upperVertex[XX]<<"  "
             <<"yu = "<<upperVertex[YY]<<"  "
             <<"zu = "<<upperVertex[ZZ]<<"  "
             <<"xi = "<<interpVertex[XX]<<"  "
             <<"yi = "<<interpVertex[YY]<<"  "
             <<"zi = "<<interpVertex[ZZ]<<"  "
             <<std::endl;

    return interpVertex;
}



/*
 *
 */
std::vector<gmx::RVec>
RegularVertexGrid::vertices()
{
    // build linearly indexed vector from dual indexed map:
    std::vector<gmx::RVec> vert;
    vert.reserve(vertices_.size());
    for(size_t i = 0; i < s_.size(); i++)
    {
        for(size_t j = 0; j < phi_.size(); j++)
        {
            std::pair<size_t, size_t> key(i, j);
            if( vertices_.find(key) != vertices_.end() )
            {
                vert.push_back(vertices_[key]); 
/*                std::cout<<"i = "<<i<<"  "
                         <<"j = "<<j<<"  "
                         <<"x = "<<vertices_[key][0]<<"  "
                         <<"y = "<<vertices_[key][1]<<"  "
                         <<"z = "<<vertices_[key][2]<<"  "
                         <<"key.1 = "<<key.first<<"  "
                         <<"key.2 = "<<key.second<<"  "
                         <<std::endl;*/
            }
            else
            {
//                throw std::logic_error("Invalid vertex reference encountered.");
            }
        }
    }

    return vert;
}


/*
 *
 */
std::vector<gmx::RVec>
RegularVertexGrid::normals()
{
    std::vector<gmx::RVec> norm;
    norm.reserve(normals_.size());
    for(size_t i = 0; i < s_.size(); i++)
    {
        for(size_t j = 0; j < phi_.size(); j++)
        {
            std::pair<size_t, size_t> key(i, j);
            if( normals_.find(key) != normals_.end() )
            {
                norm.push_back(normals_[key]); 
            }
            else
            {
//                throw std::logic_error("Invalid vertex normal reference "
//                                       "encountered.");
            }
        }
    }
    return norm;
}


/*
 *
 */
std::vector<WavefrontObjFace>
RegularVertexGrid::faces()
{
    // sanity checks:
    if( phi_.size() * s_.size() != vertices_.size() )
    {
        std::cout<<"phi.size = "<<phi_.size()<<"  "
                 <<"s.size = "<<s_.size()<<"  "
                 <<"vert.size = "<<vertices_.size()<<"  "
                 <<std::endl;
//        throw std::logic_error("Cannot generate faces on incomplete grid.");
    }

    if( !normals_.empty() && normals_.size() != vertices_.size() )
    {
        throw std::logic_error("Number of vertex normals does not equal "
                               "number of vertices.");
    }

    // preallocate face vector:
    std::vector<WavefrontObjFace> faces;
    faces.reserve(phi_.size()*s_.size());

    // loop over grid:
    for(size_t i = 0; i < s_.size() - 1; i++)
    {
        for(size_t j = 0; j < phi_.size() - 1; j++)
        {
            // calculate linear indices:
            int kbl = i*phi_.size() + j; 
            int kbr = kbl + 1;
            int ktl = kbl + phi_.size();
            int ktr = kbr + phi_.size();

            // two faces per square:
            if( normals_.empty() )
            {
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, ktr + 1, ktl + 1}) );
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, kbr + 1, ktr + 1}) );
            }
            else
            {
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, ktr + 1, ktl + 1},
                        {kbl + 1, ktr + 1, ktl + 1}) );
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, kbr + 1, ktr + 1},
                        {kbl + 1, kbr + 1, ktr + 1}) );
            }
        }
    }

    // wrap around:
    for(size_t i = 0; i < s_.size() - 1; i++)
    {
        // calculate linear indices:
        int kbl = i*phi_.size() + phi_.size() - 1; 
        int kbr = i*phi_.size();
        int ktl = kbl + phi_.size();
        int ktr = kbr + phi_.size();

        // two faces per square:
        if( normals_.empty() )
        {
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, ktr + 1, ktl + 1}) );
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, kbr + 1, ktr + 1}) );
        }
        else
        {
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, ktr + 1, ktl + 1},
                    {kbl + 1, ktr + 1, ktl + 1}) );
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, kbr + 1, ktr + 1},
                    {kbl + 1, kbr + 1, ktr + 1}) );
        }
    }

    // return face vector:
    return faces;
}



/*
 * Constructor.
 */
MolecularPathObjExporter::MolecularPathObjExporter()
{
    
}


void
MolecularPathObjExporter::operator()(std::string fileName,
                                     MolecularPath &molPath)
{
    

    bool useNormals = true;
    real extrapDist = 0.0;

    int numPhi = 30;
    int numLen = std::pow(2, 6) + 1;

    std::cout<<"gen grid = "<<numLen<<std::endl;
    // generate a grid of vertices:
    RegularVertexGrid grid = generateGrid(molPath, numLen, numPhi, extrapDist);

 

    // obtain vertices, normals, and faces from grid:
    auto vertices = grid.vertices();
    auto vertexNormals = grid.normals();
    auto faces = grid.faces();

    std::cout<<"vertices.size = "<<vertices.size()<<"  "
             <<"faces.size = "<<faces.size()<<"  "
             <<std::endl;

    

    // add faces to surface:
    WavefrontObjGroup surface("pore_surface");
    for(auto face : faces)
    {
/*        std::cout<<"face = "<<"  "
                 <<face.vertexIdx_[0]<<"  "
                 <<face.vertexIdx_[1]<<"  "
                 <<face.vertexIdx_[2]<<"  "
                 <<"hasNormals = "<<face.hasNormals()<<"  "
                 <<std::endl;*/
        surface.addFace(face);
    }
   
    std::cout<<"nVert = "<<vertices.size()<<"  "
             <<"nNorm = "<<vertexNormals.size()<<"  "
             <<std::endl;

    // assemble OBJ object:
    WavefrontObjObject obj("pore");
    obj.addVertices(vertices);
    obj.addVertexNormals(vertexNormals);
    obj.addGroup(surface);

    // scale object by factor of 10 to convert nm to Ang:
    obj.scale(10.0);
    obj.calculateCog();

    // create OBJ exporter and write to file:
    WavefrontObjExporter objExp;
    objExp.write(fileName, obj);

    
}


/*
 *
 */
std::vector<gmx::RVec>
MolecularPathObjExporter::generateNormals(
        const std::vector<gmx::RVec> &tangents)
{
    // preallocate container for normal vectors:
    std::vector<gmx::RVec> normals;
    normals.reserve(tangents.size());

    // generate initial normal:
    gmx::RVec normal = orthogonalVector(tangents[0]);
    unitv(normal, normal);
    normals.push_back(normal);
 
    // loop over tangent vectors and update normals:
    for(unsigned int i = 1; i < tangents.size(); i++)
    {
        // find axis of tangent rotation:
        gmx::RVec tangentRotAxis;
        cprod(tangents[i - 1], tangents[i], tangentRotAxis);

        // find tangent angle of rotation:
        real tangentRotCos = iprod(tangents[i], tangents[i - 1]);
        real tangentRotAxisLen = norm(tangentRotAxis);
        real tangentRotAngle = std::atan2(tangentRotAxisLen, tangentRotCos);

        // update normal by rotating it like the tangent:
        normal = rotateAboutAxis(normal, tangentRotAxis, tangentRotAngle);
        unitv(normal, normal);
        normals.push_back(normal);
    }

    // return normal vectors:
    return normals;
}


/*
 *
 */
RegularVertexGrid
MolecularPathObjExporter::generateGrid(
        MolecularPath &molPath,
        size_t numLen,
        size_t numPhi,
        real extrapDist)
{    
    // check if number of intervals is power of two:
    int numInt = numLen - 1;
    if( (numInt & (numInt)) == 0 && numInt > 0 )
    {
        throw std::logic_error("Number of steps along pore must be power of "
                               "two.");
    }

    // generate grid coordinates:
    auto s = molPath.sampleArcLength(numLen, extrapDist);
    std::vector<real> phi;
    phi.reserve(numPhi);
    for(size_t i = 0; i < numPhi; i++)
    {
        phi.push_back(i*2.0*M_PI/numPhi);
    }

    // generate grid from coordinates:
    RegularVertexGrid grid(s, phi);

    // sample centre points radii, and tangents along molecular path:
    auto centres = molPath.samplePoints(s);
    auto radii  = molPath.sampleRadii(s);
    auto tangents = molPath.sampleNormTangents(s);

    // sample normals along molecular path:
    auto normals = generateNormals(tangents);
    
    // first and last vertex ring:
    int idxLen = 0;
    for(size_t k = 0; k < phi.size(); k++)
    {
        // rotate normal vector:
        gmx::RVec rotNormal = rotateAboutAxis(
                normals[idxLen], 
                tangents[idxLen],
                phi[k]);

        // generate vertex:
        gmx::RVec vertex = centres[idxLen];
        vertex[XX] += radii[idxLen]*rotNormal[XX];
        vertex[YY] += radii[idxLen]*rotNormal[YY];
        vertex[ZZ] += radii[idxLen]*rotNormal[ZZ];

        // add to grid:
        grid.addVertex(idxLen, k, vertex);
        grid.addVertexNormal(idxLen, k, rotNormal);
    }
    idxLen = s.size() - 1;
    for(size_t k = 0; k < phi.size(); k++)
    {
        // rotate normal vector:
        gmx::RVec rotNormal = rotateAboutAxis(
                normals[idxLen], 
                tangents[idxLen],
                phi[k]);

        // generate vertex:
        gmx::RVec vertex = centres[idxLen];
        vertex[XX] += radii[idxLen]*rotNormal[XX];
        vertex[YY] += radii[idxLen]*rotNormal[YY];
        vertex[ZZ] += radii[idxLen]*rotNormal[ZZ];

        // add to grid:
        grid.addVertex(idxLen, k, vertex);
        grid.addVertexNormal(idxLen, k, rotNormal);
    }

    // build intermediate vertex rings:
    for(int i = 1; i <= numLen; i *= 2)
    {
        for(int j = 1; j < i; j += 2)
        {
            idxLen = j*(numLen - 1)/i;

            // create ring of vertices:
            std::vector<gmx::RVec> vertRing;
            std::vector<gmx::RVec> vertNormRing;
            for(size_t k = 0; k < phi.size(); k += 1)
            {
                // rotate normal vector:
                gmx::RVec rotNormal = rotateAboutAxis(
                        normals[idxLen], 
                        tangents[idxLen],
                        phi[k]);

                // generate vertex:
                gmx::RVec vertex = centres[idxLen];
                vertex[XX] += radii[idxLen]*rotNormal[XX];
                vertex[YY] += radii[idxLen]*rotNormal[YY];
                vertex[ZZ] += radii[idxLen]*rotNormal[ZZ];

                vertRing.push_back(vertex);
                vertNormRing.push_back(rotNormal);
            }
 

            // check for clashes:
            bool clash = false;
            real angleThreshold = 0.0;
            for(size_t k = 0; k < vertRing.size(); k++)
            {
                gmx::RVec vecA;
                gmx::RVec vecB;
                rvec_sub(vertRing[k], centres[idxLen - 1], vecA);
                rvec_sub(vertRing[k], centres[idxLen + 1], vecB);

                real cosA = iprod(tangents[idxLen - 1], vecA);
                real cosB = iprod(tangents[idxLen + 1], vecB);

                if( cosA < angleThreshold || cosB > -angleThreshold )
                {
//                    std::cout<<"clash!"<<std::endl;
                    
                    clash = true;
                    break;
                } 
            }

            if( clash == true )
            {
                // adjust tangent and normals:
              rvec_add(tangents[idxLen - 1], tangents[idxLen + 1], tangents[idxLen]);
              svmul(0.5, tangents[idxLen], tangents[idxLen]);
              normals = generateNormals(tangents); // TODO: more efficient jsut recomp one

                // adjust radius:
//                radii[idxLen] = 0.5*(radii[idxLen - 1] + radii[idxLen + 1]);
//                radii[idxLen] = std::max(radii[idxLen - 1], radii[idxLen + 1]);
            }

            // recompute ring of vertices:            
            vertRing.clear();
            vertNormRing.clear();
            for(size_t k = 0; k < phi.size(); k += 1)
            {
                // rotate normal vector:
                gmx::RVec rotNormal = rotateAboutAxis(
                        normals[idxLen], 
                        tangents[idxLen],
                        phi[k]);

                // generate vertex:
                gmx::RVec vertex = centres[idxLen];
                vertex[XX] += radii[idxLen]*rotNormal[XX];
                vertex[YY] += radii[idxLen]*rotNormal[YY];
                vertex[ZZ] += radii[idxLen]*rotNormal[ZZ];

                vertRing.push_back(vertex);
                vertNormRing.push_back(rotNormal);
            }
            
            // check for clashes AGAIN:
            clash = false;
            angleThreshold = 0.0;
            for(size_t k = 0; k < vertRing.size(); k++)
            {
                gmx::RVec vecA;
                gmx::RVec vecB;
                rvec_sub(vertRing[k], centres[idxLen - 1], vecA);
                rvec_sub(vertRing[k], centres[idxLen + 1], vecB);

                real cosA = iprod(tangents[idxLen - 1], vecA);
                real cosB = iprod(tangents[idxLen + 1], vecB);


                if( cosA < angleThreshold || cosB > -angleThreshold )
                {
//                    std::cout<<"REPEATED clash!"<<std::endl;
                    
                    clash = true;
                    break;
                } 
            }


            if( clash == false )
            {            
                // add vertices to grid:
                for(size_t k = 0; k < vertRing.size(); k++)
                {
                    grid.addVertex(idxLen, k, vertRing[k]);
                }
     
                // add vertex normals to grid:
                for(size_t k = 0; k < vertNormRing.size(); k++)
                {
                    grid.addVertexNormal(idxLen, k, vertNormRing[k]);
                }
            }

            std::cout<<"i = "<<i<<"  "
                     <<"j = "<<j<<"  "
                     <<"numLen = "<<numLen<<"  "
                     <<"crnt = "<<idxLen<<"  "
                     <<"s.size = "<<s.size()<<"  "
                     <<"lo = "<<(j-1)*(numLen - 1)/i<<"  "
                     <<"hi = "<<(j+1)*(numLen - 1)/i<<"  "
                     <<std::endl;
        }
    }

    // correct mesh:
    
    // FIXME I think the porblem is that interpolation in XYZ does also shift
    // the vertex in S direction. Might be better to interpolate R if a con-
    // flict is found...
//    grid.interpolateMissing();

    // return grid:
    return grid;
}



/*
 *
 */
int
MolecularPathObjExporter::numPlanarVertices(real &d, real &r)
{
    return std::max(static_cast<int>(std::ceil(PI_/(2.0*std::acos(1.0 - d*d/(2.0*r*r))))), 4);
}


/*
 *
 */
std::pair<std::vector<gmx::RVec>, std::vector<gmx::RVec>>
MolecularPathObjExporter::vertexRing(gmx::RVec base,
                                     gmx::RVec tangent,
                                     gmx::RVec normal,
                                     real radius,
                                     real angleIncrement,
                                     size_t nIncrements)
{
    // make sure input normal vector is normalised:
    unitv(normal, normal);

    // preallocate vertex and normal vectors:
    std::vector<gmx::RVec> vertices;
    vertices.reserve(nIncrements);
    std::vector<gmx::RVec> normals;
    vertices.reserve(nIncrements);

    // sample vertices in a ring around the base point: 
    for(size_t j = 0; j < nIncrements; j++)
    {
        // rotate normal vector:
        gmx::RVec rotNormal = rotateAboutAxis(normal, tangent, j*angleIncrement);
        normals.push_back(rotNormal);

        // generate vertex:
        gmx::RVec vertex = base;
        vertex[XX] += radius*rotNormal[XX];
        vertex[YY] += radius*rotNormal[YY];
        vertex[ZZ] += radius*rotNormal[ZZ];
        vertices.push_back(vertex);
    }

    // return ring of vertices:
    std::pair<std::vector<gmx::RVec>, std::vector<gmx::RVec>> ret;
    ret.first = vertices;
    ret.second = normals;
    return ret;
}


/*
 *
 */
gmx::RVec
MolecularPathObjExporter::orthogonalVector(gmx::RVec vec)
{
    // find first nonzero element in vector:
    int idxNonZero = -1;
    for(int i = 0; i < 3; i++)
    {
        if( std::abs(vec[i]) > std::numeric_limits<real>::epsilon() )
        {
            idxNonZero = i;
            break;
        }
    }

    // sanity check:
    if( idxNonZero == -1 )
    {
        std::cerr<<"ERROR: Can not find orthogonal to null vector!"<<std::endl;
        std::cerr<<"vec = "<<vec[0]<<" "<<vec[1]<<" "<<vec[2]<<std::endl;
        std::abort();
    }

    // find index for switching:
    int idxSwitch = (idxNonZero + 1) % 3;

    // construct non-colinear vector by element switching:
    gmx::RVec otherVec = vec;
    otherVec[idxNonZero] = vec[idxSwitch];
    otherVec[idxSwitch] = -vec[idxNonZero];

    // construct orthogonal vector via cross product:
    gmx::RVec orthVec;
    cprod(vec, otherVec, orthVec);

    // return a vector orthogonal to input vector:
    return orthVec;
}


/*
 *
 */
gmx::RVec
MolecularPathObjExporter::rotateAboutAxis(gmx::RVec vec, 
                                          gmx::RVec axis,
                                          real angle)
{
    // evaluate trigonometrix functions of rotation angle:
    const real COS = std::cos(angle);
    const real SIN = std::sin(angle);

    // construct rotation matrix:
    matrix rotMat;
    rotMat[XX][XX] = COS + axis[XX]*axis[XX]*(1.0 - COS);
    rotMat[XX][YY] = axis[XX]*axis[YY]*(1.0 - COS) - axis[ZZ]*SIN;
    rotMat[XX][ZZ] = axis[XX]*axis[ZZ]*(1.0 - COS) + axis[YY]*SIN;
    rotMat[YY][XX] = axis[YY]*axis[XX]*(1.0 - COS) + axis[ZZ]*SIN;
    rotMat[YY][YY] = COS + axis[YY]*axis[YY]*(1 - COS);
    rotMat[YY][ZZ] = axis[YY]*axis[ZZ]*(1.0 - COS) - axis[XX]*SIN;
    rotMat[ZZ][XX] = axis[ZZ]*axis[XX]*(1.0 - COS) - axis[YY]*SIN;
    rotMat[ZZ][YY] = axis[ZZ]*axis[YY]*(1.0 - COS) + axis[XX]*SIN;
    rotMat[ZZ][ZZ] = COS + axis[ZZ]*axis[ZZ]*(1.0 - COS);

    // rotate vector:
    gmx::RVec rotVec;
    mvmul(rotMat, vec, rotVec);

    // return rotated vector:
    return rotVec;
}

