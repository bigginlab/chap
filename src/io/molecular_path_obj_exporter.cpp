#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

#include <gromacs/math/vec.h>
#include <gromacs/utility/real.h> 

#include "io/molecular_path_obj_exporter.hpp"

#include "geometry/cubic_spline_interp_3D.hpp"


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
RegularVertexGrid::addVertex(
        size_t i, 
        size_t j,
        std::string p,
        gmx::RVec vertex, 
        real weight)
{
    if( std::isnan(weight) )
    {
        std::cout<<"i = "<<i<<"  "
                 <<"j = "<<j<<"  "
                 <<"p = "<<p<<"  "
                 <<std::endl;
        throw std::logic_error("weight is nan");
    }

    p_.insert(p);
    std::tuple<size_t, size_t, std::string> key(i, j, p);
    vertices_[key] = vertex;
    weights_[key] = weight;
}


/*
 *
 */
void
RegularVertexGrid::addVertexNormal(
        size_t i, 
        size_t j,
        std::string p,
        gmx::RVec normal)
{
    p_.insert(p);
    std::tuple<size_t, size_t, std::string> key(i, j, p);
    normals_[key] = normal;
}


/*
 *
 */
gmx::RVec
RegularVertexGrid::getVertex(size_t i, size_t j, std::string p) const
{
    std::tuple<size_t, size_t, std::string> key(i, j, p);
    return vertices_.at(key);
}


/*
 *
 */
std::vector<gmx::RVec>
RegularVertexGrid::vertices(
        std::string p)
{
    // build linearly indexed vector from dual indexed map:
    std::vector<gmx::RVec> vert;
    vert.reserve(vertices_.size());
    for(size_t i = 0; i < s_.size(); i++)
    {
        for(size_t j = 0; j < phi_.size(); j++)
        {
            std::tuple<size_t, size_t, std::string> key(i, j, p);
            if( vertices_.find(key) != vertices_.end() )
            {
                vert.push_back(vertices_[key]); 
            }
            else
            {
                throw std::logic_error("Invalid vertex reference encountered.");
            }
        }
    }

    return vert;
}


/*
 *
 */
std::vector<std::pair<gmx::RVec, real>>
RegularVertexGrid::weightedVertices(
        std::string p)
{
    // build linearly indexed vector from dual indexed map:
    std::vector<std::pair<gmx::RVec, real>> vert;
    vert.reserve(vertices_.size());
    for(size_t i = 0; i < s_.size(); i++)
    {
        for(size_t j = 0; j < phi_.size(); j++)
        {
            std::tuple<size_t, size_t, std::string> key(i, j, p);
            if( vertices_.find(key) != vertices_.end() && 
                weights_.find(key) != weights_.end() )
            {
                std::pair<gmx::RVec, real> vertex(vertices_[key], weights_[key]);
                vert.push_back(vertex); 
            }
            else
            {
                throw std::logic_error("Invalid vertex reference encountered.");
            }
        }
    }

    return vert;
}


/*
 *
 */
void
RegularVertexGrid::normalsFromFaces()
{
    // array sizes for index wrap:
    int mI = s_.size();
    int mJ = phi_.size();

    for(auto p : p_)
    {
        for(int i = 0; i < s_.size(); i++)
        {
            for(int j = 0; j < phi_.size(); j++)
            {
                // index pairs for neighbouring vertices:
                // TODO: not circular in i, cant wrap around
                std::tuple<size_t, size_t, std::string> crntKey(
                        i, 
                        j,
                        p);
                std::tuple<size_t, size_t, std::string> leftKey(
                        i, 
                        (j - 1 + mJ) % mJ,
                        p);
                std::tuple<size_t, size_t, std::string> rghtKey(
                        i, 
                        (j + 1 + mJ) % mJ,
                        p);
                std::tuple<size_t, size_t, std::string> upprKey(
                        (i + 1 + mI) % mI, 
                        j,
                        p);
                std::tuple<size_t, size_t, std::string> lowrKey((
                        i - 1 + mI) % mI, 
                        j,
                        p);
                std::tuple<size_t, size_t, std::string> dglrKey(
                        (i - 1 + mI) % mI, 
                        (j + 1 + mJ) % mJ,
                        p);
                std::tuple<size_t, size_t, std::string> dgulKey(
                        (i + 1 + mI) % mI, 
                        (j - 1 + mJ) % mJ,
                        p);

                // handle endpoints in direction along spline:
                if( i == 0 )
                {
                    lowrKey = crntKey; 
                    dglrKey = crntKey;
                }
                if( i == s_.size() - 1 )
                {
                    upprKey = crntKey;
                    dgulKey = crntKey;
                }

                // neighbouring vertices:
                gmx::RVec crntVert = vertices_.at(crntKey);
                gmx::RVec leftVert = vertices_.at(leftKey);
                gmx::RVec rghtVert = vertices_.at(rghtKey);
                gmx::RVec upprVert = vertices_.at(upprKey);
                gmx::RVec lowrVert = vertices_.at(lowrKey);
                gmx::RVec dglrVert = vertices_.at(dglrKey);
                gmx::RVec dgulVert = vertices_.at(dgulKey);

                // initialise normal as null vector:
                gmx::RVec norm(0.0, 0.0, 0.0);
                gmx::RVec sideA;
                gmx::RVec sideB;

                // North-East triangle:
                rvec_sub(rghtVert, crntVert, sideA);
                rvec_sub(upprVert, crntVert, sideB);
                addTriangleNorm(sideA, sideB, norm);
                
                // North-North-West triangle:
                rvec_sub(upprVert, crntVert, sideA);
                rvec_sub(dgulVert, crntVert, sideB);
                addTriangleNorm(sideA, sideB, norm);

                // West-North-West triangle:
                rvec_sub(dgulVert, crntVert, sideA);
                rvec_sub(leftVert, crntVert, sideB);
                addTriangleNorm(sideA, sideB, norm);

                // South-West triangle:
                rvec_sub(leftVert, crntVert, sideA);
                rvec_sub(lowrVert, crntVert, sideB);
                addTriangleNorm(sideA, sideB, norm);

                // South-South-East triangle:
                rvec_sub(lowrVert, crntVert, sideA);
                rvec_sub(dglrVert, crntVert, sideB);
                addTriangleNorm(sideA, sideB, norm);
                
                // East-South-East triangle:
                rvec_sub(dglrVert, crntVert, sideA);
                rvec_sub(rghtVert, crntVert, sideB);
                addTriangleNorm(sideA, sideB, norm);

                // normalise normal:
                unitv(norm, norm);
                
                // add to container of normals:
                normals_[crntKey] = norm;
            }
        }
    }
}


/*
 *
 */
ColourScale
RegularVertexGrid::colourScale(std::string p)
{
    return colourScales_.at(p);
}



/*
 *
 */
void
RegularVertexGrid::addTriangleNorm(
        const gmx::RVec &sideA,
        const gmx::RVec &sideB,
        gmx::RVec &norm)
{
    gmx::RVec tmp;
    cprod(sideA, sideB, tmp);
    rvec_add(norm, tmp, norm);
}


/*
 *
 */
std::vector<gmx::RVec>
RegularVertexGrid::normals(
        std::string p)
{
    std::vector<gmx::RVec> norm;
    norm.reserve(normals_.size());
    for(size_t i = 0; i < s_.size(); i++)
    {
        for(size_t j = 0; j < phi_.size(); j++)
        {
            std::tuple<size_t, size_t, std::string> key(i, j, p);
            if( normals_.find(key) != normals_.end() )
            {
                norm.push_back(normals_[key]); 
            }
            else
            {
                throw std::logic_error("Invalid vertex normal reference "
                                       "encountered.");
            }
        }
    }

    return norm;
}


/*
 *
 */
std::vector<WavefrontObjFace>
RegularVertexGrid::faces(
        std::string p)
{
    // sanity checks:
    if( phi_.size() * s_.size() * p_.size() != vertices_.size() )
    {
        std::cout<<"MESH ERROR  "
                 <<"phi.size = "<<phi_.size()<<"  "
                 <<"s.size = "<<s_.size()<<"  "
                 <<"vert.size = "<<vertices_.size()<<"  "
                 <<std::endl;
                 // TODO keep this?
//        throw std::logic_error("Cannot generate faces on incomplete grid.");
    }

    if( !normals_.empty() && normals_.size() != vertices_.size() )
    {
        throw std::logic_error("Number of vertex normals does not equal "
                               "number of vertices.");
    }
    
    // find scalar property data range:
    real minRange = std::numeric_limits<real>::max();
    real maxRange = std::numeric_limits<real>::min();
    for(auto it = weights_.begin(); it != weights_.end(); it++)
    {
        if( it -> second < minRange )
        {
            minRange = it -> second;
        }
        if( it -> second > maxRange )
        {
            maxRange = it -> second;
        }
    }

    // prepare colour scale:
    ColourScale colScale(p);
    colScale.setRange(minRange, maxRange);
    colScale.setResolution(100);
    colourScales_.insert(std::pair<std::string, ColourScale>(p, colScale));

    // number of vertices per property grid:
    size_t propIdx = std::distance(p_.begin(), find(p_.begin(), p_.end(), p));
    size_t vertOffset = s_.size() * phi_.size() * propIdx;

    // preallocate face vector:
    std::vector<WavefrontObjFace> faces;
    faces.reserve(phi_.size()*s_.size());

    // loop over grid:
    for(size_t i = 0; i < s_.size() - 1; i++)
    {
        for(size_t j = 0; j < phi_.size() - 1; j++)
        {
            // calculate linear indices:
            int kbl = vertOffset + i * phi_.size() + j; 
            int kbr = kbl + 1;
            int ktl = kbl + phi_.size();
            int ktr = kbr + phi_.size();

            // vertex keys:
            std::tuple<size_t, size_t, std::string> keyBl(i,     j,     p);
            std::tuple<size_t, size_t, std::string> keyBr(i,     j + 1, p);
            std::tuple<size_t, size_t, std::string> keyTl(i + 1, j,     p);
            std::tuple<size_t, size_t, std::string> keyTr(i + 1, j + 1, p);

            // face weight is average of vertex weights:
            real scalarA = weights_.at(keyBl) + weights_.at(keyTr) + weights_.at(keyTl);
            real scalarB = weights_.at(keyBl) + weights_.at(keyBr) + weights_.at(keyTr);
            scalarA /= 3.0;
            scalarB /= 3.0;

            // name of material from colour scale:
            std::string mtlNameA = colScale.scalarToColourName(scalarA); 
            std::string mtlNameB = colScale.scalarToColourName(scalarB); 

            // two faces per square:
            if( normals_.empty() )
            {
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, ktr + 1, ktl + 1},
                        mtlNameA) );
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, kbr + 1, ktr + 1},
                        mtlNameB) );
            }
            else
            {
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, ktr + 1, ktl + 1},
                        {kbl + 1, ktr + 1, ktl + 1},
                        mtlNameA) );
                faces.push_back( WavefrontObjFace(
                        {kbl + 1, kbr + 1, ktr + 1},
                        {kbl + 1, kbr + 1, ktr + 1},
                        mtlNameB) );
            }
        }
    }

    // wrap around:
    for(size_t i = 0; i < s_.size() - 1; i++)
    {
        // calculate linear indices:
        int kbl = vertOffset + i*phi_.size() + phi_.size() - 1; 
        int kbr = vertOffset + i*phi_.size();
        int ktl = kbl + phi_.size();
        int ktr = kbr + phi_.size();

        // vertex keys:
        std::tuple<size_t, size_t, std::string> keyBl(i, phi_.size() - 1, p);
        std::tuple<size_t, size_t, std::string> keyBr(i, 0, p);
        std::tuple<size_t, size_t, std::string> keyTl(i + 1, phi_.size() - 1, p);
        std::tuple<size_t, size_t, std::string> keyTr(i + 1, 0, p);

        // face weight is average of vertex weights:
        real scalarA = weights_[keyBl] + weights_[keyTr] + weights_[keyTl];
        real scalarB = weights_[keyBl] + weights_[keyBr] + weights_[keyTr];
        scalarA /= 3.0;
        scalarB /= 3.0;

        // name of material from colour scale:
        std::string mtlNameA = colScale.scalarToColourName(scalarA); 
        std::string mtlNameB = colScale.scalarToColourName(scalarB); 

        // two faces per square:
        if( normals_.empty() )
        {
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, ktr + 1, ktl + 1},
                    mtlNameA) );
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, kbr + 1, ktr + 1},
                    mtlNameB) );
        }
        else
        {

            faces.push_back( WavefrontObjFace(
                    {kbl + 1, ktr + 1, ktl + 1},
                    {kbl + 1, ktr + 1, ktl + 1},
                    mtlNameA) );
            faces.push_back( WavefrontObjFace(
                    {kbl + 1, kbr + 1, ktr + 1},
                    {kbl + 1, kbr + 1, ktr + 1},
                    mtlNameB) );
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


/*
 *
 */
void
MolecularPathObjExporter::operator()(
        std::string fileName,
        std::string objectName,
        MolecularPath &molPath,
        std::map<std::string, ColourPalette> palettes)
{
    std::cout<<"EXPORT"<<std::endl;

    // define evaluation range:   
    real extrapDist = 0.0;
    std::pair<real, real> range(molPath.sLo() - extrapDist,
                                molPath.sHi() + extrapDist);

    // define resolution:
    int numPhi = 50;
    int numLen = std::pow(2, 8) + 1;
    std::pair<size_t, size_t> resolution(numLen, numPhi);
    
    // pathway geometry:
    auto centreLine = molPath.centreLine();
    auto pathRadius = molPath.pathRadius();

    // pathway properties:
    // (radius is added here to ensure that there is always one property)
    molPath.addScalarProperty("radius", pathRadius, false);
    auto properties = molPath.scalarProperties();   


    // Build OBJ & MTL Objects of Coloured Pore Surface
    //-------------------------------------------------------------------------

    // prepare objects:
    WavefrontObjObject obj(objectName);
    WavefrontMtlObject mtl;
  
    // generate the vertex grid:
    RegularVertexGrid grid = generateGrid(
            centreLine,
            pathRadius,
            properties,
            resolution,
            range);

    std::cout<<"GRID COMPLETE!"<<std::endl;

    // loop over properties:
    for(auto prop : properties)
    {
        // obtain vertices, normals, and faces from grid:
        grid.normalsFromFaces();
        std::cout<<"normals calculated!"<<std::endl;
        auto vertices = grid.weightedVertices(prop.first);
        std::cout<<"vertices obtained!"<<std::endl;
        auto vertexNormals = grid.normals(prop.first);
        std::cout<<"normals obtained!"<<std::endl;
        auto faces = grid.faces(prop.first);

        std::cout<<"faces obtained!"<<std::endl;

        // add faces to surface:
        WavefrontObjGroup group("pathway_" + prop.first);
        for(auto face : faces)
        {
            group.addFace(face);
        }

        // add to the overall OBJ object:
        obj.addVertices(vertices);
        obj.addVertexNormals(vertexNormals);
        obj.addGroup(group);

        // obtain colour scale for this property:
        auto colScale = grid.colourScale(prop.first);

        // is there a colour palatte for this property?
        if( palettes.find(prop.first) != palettes.end() )
        {
            colScale.setPalette(palettes.at(prop.first));
        }
        else if( palettes.find("default") != palettes.end() )
        {
            colScale.setPalette(palettes.at("default"));
        }
        else
        {
            throw std::runtime_error("Could not find colour palette for "
                                     "property " + prop.first + " and no "
                                     "default colour palette is available.");
        }

        // retrieve RGB colours from scale:
        auto colours = colScale.getColours();

        // create material for each colour in this scale:
        for(auto col : colours)
        {
            // create material corresponding to colour:
            WavefrontMtlMaterial material(col.first);
            material.setAmbientColour(col.second);
            material.setDiffuseColour(col.second);
            material.setSpecularColour(col.second);

            // add to material file:
            mtl.addMaterial(material);
        }
    }


    // Serialise OBJ & MTL Objects
    //-------------------------------------------------------------------------
    
    std::string mtlFileName = "output.mtl";

    // scale object by factor of 10 to convert nm to Ang:
    obj.scale(10.0);
    obj.calculateCog();

    // add name of material library:
    obj.setMaterialLibrary(mtlFileName);

    // create OBJ exporter and write to file:
    WavefrontObjExporter objExp;
    objExp.write(fileName, obj);

    // create an MTL exporter and write to file:
    WavefrontMtlExporter mtlExp;
    mtlExp.write(mtlFileName, mtl);
}


/*
 *
 */
RegularVertexGrid
MolecularPathObjExporter::generateGrid(
        SplineCurve3D &centreLine,
        SplineCurve1D &radius,
        std::map<std::string, std::pair<SplineCurve1D, bool>> &properties,
        std::pair<size_t, size_t> resolution,
        std::pair<real, real> range)
{
    // extract resolution:
    size_t numLen = resolution.first;
    size_t numPhi = resolution.second;

    // check if number of intervals is power of two:
    int numInt = numLen - 1;
    if( (numInt & (numInt)) == 0 && numInt > 0 )
    {
        throw std::logic_error("Number of steps along pore must be power of "
                               "two.");
    }

    // generate grid coordinates:
    std::vector<real> s;
    s.reserve(numLen);
    for(int i = 0; i < numLen; i++)
    {
        s.push_back(i*(range.second - range.first)/numLen + range.first);
    }
    std::vector<real> phi;
    phi.reserve(numPhi);
    for(size_t i = 0; i < numPhi; i++)
    {
        phi.push_back(i*2.0*M_PI/numPhi);
    }

    // generate grid from coordinates:
    RegularVertexGrid grid(s, phi);

    // loop over properties and add vertices:
    for(auto &prop : properties)
    {
        generatePropertyGrid(
                centreLine,
                radius,
                prop,
                grid);
    }

    // return the overall grid:
    return grid;
}


/*!
 *TODO this comment is outdated
 *
 * Generates grid along MolecularPath, but uses a tangent vector equal to the
 * channel direction vector. This does not strictly give the correct surface, 
 * as the tangent may vary along the centre line. However, this method will not
 * create overlapping vertices, so long as the centre line does not change 
 * direction with respect to the channel direction vectors, which is guaranteed
 * for the inplane optimised probe path finder. Moreover, all surface points 
 * generated this way are guaranteed to not lie outside the pore, so long as
 * a probe based method was used (this is true because the free distance around 
 * a centre is free in any direction). 
 */
void
MolecularPathObjExporter::generatePropertyGrid(
        SplineCurve3D &centreLine,
        SplineCurve1D &radius,
        std::pair<std::string, std::pair<SplineCurve1D, bool>> property,
        RegularVertexGrid &grid)
{   
    // extract grid coordinates:
//    const std::vector<real> s = grid.s_;
    std::vector<real> s;
    int num = 50;
    real ds = (grid.s_.back() - grid.s_.front())/(num - 1);
    for(size_t i = 0; i < num; i++)
    {
        s.push_back(grid.s_.front() + i*ds);
    }
    std::vector<real> phi = grid.phi_;
    size_t numLen = s.size();

    // sample points, radii, and tangents along molecular path:
    std::vector<gmx::RVec> centres;
    centres.reserve(s.size());
    std::vector<gmx::RVec> tangents;
    tangents.reserve(s.size());
    std::vector<real> radii;
    radii.reserve(s.size());
//    std::vector<real> prop;
//    prop.reserve(s.size());
    for(auto eval : s)
    {
        centres.push_back( centreLine.evaluate(eval, 0) );
        gmx::RVec tv = centreLine.tangentVec(eval);
        unitv(tv, tv);
        tangents.push_back(tv);
        radii.push_back( radius.evaluate(eval, 0) );
//        radii.push_back( 0.02 );
//        prop.push_back( property.second.first.evaluate(eval, 0) );
    }

    // estimate channel direction vector:
    gmx::RVec chanDirVec;
    rvec_sub(centres.back(), centres.front(), chanDirVec);
    unitv(chanDirVec, chanDirVec);

    // sample scalar property along the path:
//    shiftAndScale(prop, property.second.second);

    // tangents always in direction of channel: // FIXME
//    std::vector<gmx::RVec> tangents(centres.size(), chanDirVec);

    // sample normals along molecular path:
    auto normals = generateNormals(tangents);
    
    // store vertex rings:
    std::vector<gmx::RVec> vertRing(phi.size());







    // generate support points:


    std::map<int, std::vector<gmx::RVec>> vertexRings;
    
    std::cout<<"vertexRings initialised"<<std::endl;



    // first vertex ring:
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

        // add to vertex ring:
        vertRing[k] = vertex;
    }
    vertexRings[idxLen] = vertRing;

    // last vertex ring:
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

        // add to vertex ring:
        vertRing[k] = vertex;
    }
    vertexRings[idxLen] = vertRing;


    // build intermediate vertex rings:
    
    for(int i = 1; i <= numLen; i *= 2)
    {
        for(int j = 1; j < i; j += 2)
        {
            idxLen = j*(numLen - 1)/i;
            int idxLower = (j - 1)*(numLen - 1)/i;
            int idxUpper = (j + 1)*(numLen - 1)/i;

            std::cout<<"idxLen = "<<idxLen<<std::endl;

            // create ring of vertices:
            bool hasClashes = false;
            for(size_t k = 0; k < phi.size(); k ++)
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
  
                // difference vectors in neighbouring discs:
                gmx::RVec a;
                rvec_sub(vertex, centres[idxLower], a);
                unitv(a, a);
                gmx::RVec b;
                rvec_sub(vertex, centres[idxUpper], b);
                unitv(b, b);

                // cosine of angles between difference vectors:
                real cosA = iprod(a, tangents[idxLower]);
                real cosB = iprod(b, tangents[idxUpper]);

                // check overlap:
                // TODO: this criterion is confirmed to identify clashes, 
                // just need to fix them now
                real thres = 0.0;
                if( cosA < thres or cosB > -thres )
                {
                    std::cout<<"CLASH  ";
                    std::cout<<""
                             <<"idxLen = "<<idxLen<<"  "
                             <<"k = "<<k<<"  "
                             <<"p = "<<property.first<<"  "
                             <<"cosA = "<<cosA<<"  "
                             <<"cosB = "<<cosB<<"  ";
                    std::cout<<std::endl;

                    hasClashes = true;
                    break;
                }


                vertRing[k] = vertex;
            }
   
            // TODO
//            hasClashes = false;
            if( !hasClashes )
            {
                vertexRings[idxLen] = vertRing;
            }
            hasClashes = false;
        }
    }
    

    // interpolate support points on each equal-phi line:
    std::vector<SplineCurve3D> curves;
    for(size_t k = 0; k < phi.size(); k++)
    {


        // extract support points and parameterisation for interpolation:
        std::vector<real> param;
        std::vector<gmx::RVec> points;
        for(auto vr = vertexRings.begin(); vr != vertexRings.end(); vr++)
        {
            std::cout<<"   vr.size = "<<vr -> second.size()<<"  "
                     <<"vr.first = "<<vr -> first<<"  "
                     <<std::endl;

            param.push_back( s[vr -> first] );
            points.push_back( vr -> second[k] );
        }

        std::cout<<"INTERPOLATION for "
                 <<"k = "<<k<<"  "
                 <<"phi = "<<phi[k]<<"  "
                 <<"vertexRings.size = "<<vertexRings.size()<<"  "
                 <<"param.size = "<<param.size()<<"  "
                 <<"points.size = "<<points.size()<<"  "
                 <<std::endl;
        

        // interpolate these points:
        CubicSplineInterp3D interp;
        curves.push_back( interp(param, points, eSplineInterpBoundaryHermite) );
    }


    // build mesh



    std::vector<real> prop;
    for(size_t i = 0; i < grid.s_.size(); i++)
    {
        prop.push_back( property.second.first.evaluate(grid.s_[i], 0) );
    }


    // sample scalar property along the path:
    shiftAndScale(prop, property.second.second);



    int co = 0;
    for(size_t i = 0; i < grid.s_.size(); i++)
    {
        for(size_t k = 0; k < grid.phi_.size(); k++)
        {
//            real prop = property.second.first.evaluate(grid.s_[i], 0);


            std::cout<<"i = "<<i<<"  "<<"s = "<<grid.s_[i]<<"  "
                     <<"k = "<<k<<"  "<<"phi = "<<grid.phi_[k]<<"  "
                     <<"p = "<<property.first<<"  "
                     <<"curves.size = "<<curves.size()<<"  "
                     <<"s.size = "<<grid.s_.size()<<"  "
                     <<"prop = "<<prop[i]<<"  "
                     <<std::endl;

            grid.addVertex(
                    i, 
                    k, 
                    property.first, 
                    curves[k].evaluate(grid.s_[i], 0),
                    prop[i]);
            co++;
        }
    }

    std::cout<<"MESH COMPLETE for property "
             <<property.first<<"  "
             <<"co = "<<co<<"  "
             <<std::endl;










/*
    // first vertex ring:
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

        // add to vertex ring:
        vertRing[k] = vertex;

        // add to grid:
        grid.addVertex(idxLen, k, property.first, vertex, prop[idxLen]);
    }

    // last vertex ring:
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

        // add to vertex ring:
        vertRing[k] = vertex;

        // add to grid:
        grid.addVertex(idxLen, k, property.first, vertex, prop[idxLen]);
    }

    // build intermediate vertex rings:
    for(int i = 1; i <= numLen; i *= 2)
    {
        for(int j = 1; j < i; j += 2)
        {
            idxLen = j*(numLen - 1)/i;
            int idxLower = (j - 1)*(numLen - 1)/i;
            int idxUpper = (j + 1)*(numLen - 1)/i;

            // create ring of vertices:
            bool hasClashes = false;
            for(size_t k = 0; k < phi.size(); k ++)
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


                // TODO check vertex!
                auto vertLo = grid.getVertex(idxLower, k, property.first);
                auto vertHi = grid.getVertex(idxUpper, k, property.first);
  
                // difference vectors in neighbouring discs:
                gmx::RVec a;
                rvec_sub(vertex, centres[idxLower], a);
                unitv(a, a);
                gmx::RVec b;
                rvec_sub(vertex, centres[idxUpper], b);
                unitv(b, b);

                // cosine of angles between difference vectors:
                real cosA = iprod(a, tangents[idxLower]);
                real cosB = iprod(b, tangents[idxUpper]);

                // check overlap:
                // TODO: this criterion is confirmed to identify clashes, 
                // just need to fix them now
                real thres = 0.0;
                if( cosA < thres or cosB > -thres )
                {
                    std::cout<<"CLASH  ";
                    std::cout<<""
                             <<"idxLen = "<<idxLen<<"  "
                             <<"k = "<<k<<"  "
                             <<"p = "<<property.first<<"  "
                             <<"cosA = "<<cosA<<"  "
                             <<"cosB = "<<cosB<<"  ";
                    std::cout<<std::endl;

                    hasClashes = true;
                    break;
                    // fix by interpolation:
//                    rvec_add(vertLo, vertHi, vertex);
//                    svmul(0.5, vertex, vertex);
                }


                vertRing[k] = vertex;
            }
           
            if( hasClashes )
            {
                for(int k = 0; k < vertRing.size(); k++)
                {
                    auto vertLo = grid.getVertex(idxLower, k, property.first);
                    auto vertHi = grid.getVertex(idxUpper, k, property.first);
                    rvec_add(vertLo, vertHi, vertRing[k]);
                    svmul(0.5, vertRing[k], vertRing[k]);

                    // also correct tangent:
                    rvec_add(tangents[idxLower], tangents[idxUpper], tangents[idxLen]);
                    svmul(0.5, tangents[idxLen], tangents[idxLen]);
                }
            }
            hasClashes = false;

            // add vertices to grid:
            for(size_t k = 0; k < vertRing.size(); k++)
            {
                grid.addVertex(
                        idxLen, 
                        k, 
                        property.first, 
                        vertRing[k], 
                        prop[idxLen]);
            }
        }
    }
    */
}


/*
 *
 */
/*
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
//    auto radii  = molPath.sampleRadii(s);
//    auto tangents = molPath.sampleNormTangents(s);
    std::vector<gmx::RVec> tangents(centres.size(), gmx::RVec(0.0, 0.0, 1.0));

    std::vector<real> radii(tangents.size(), 0.025);
    std::vector<real> prop(tangents.size(), 1.0);
    
    // sample normals along molecular path:
    auto normals = generateNormals(tangents);
    

    // store vertex rings:
    std::vector<gmx::RVec> vertRing(phi.size());
    std::vector<std::vector<gmx::RVec>> vertexRings(tangents.size());


    // first vertex ring:
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

        // add to vertex ring:
        vertRing[k] = vertex;

        // add to grid:
        grid.addVertex(idxLen, k, vertex, prop[idxLen]);
        grid.addVertexNormal(idxLen, k, rotNormal);
    }
    vertexRings[idxLen] = vertRing;

    // last vertex ring:
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

        // add to vertex ring:
        vertRing[k] = vertex;

        // add to grid:
        grid.addVertex(idxLen, k, vertex, prop[idxLen]);
        grid.addVertexNormal(idxLen, k, rotNormal);
    }
    vertexRings[idxLen] = vertRing;

    // build intermediate vertex rings:
    int numClash = 0;
    int clashLower = 0;
    int clashUpper = 0;
    int numVert = 0;
    int numSpecial = 0;
    int numPers = 0;
    for(int i = 1; i <= numLen; i *= 2)
    {
        for(int j = 1; j < i; j += 2)
        {
            idxLen = j*(numLen - 1)/i;
            int idxLower = (j - 1)*(numLen - 1)/i;
            int idxUpper = (j + 1)*(numLen - 1)/i;

            // FIXME
            tangents[idxLen] = gmx::RVec(0, 0, 1);
            tangents[idxLower] = gmx::RVec(0, 0, 1);
            tangents[idxUpper] = gmx::RVec(0, 0, 1);

            // create ring of vertices:
            bool hasClashes = false;
            std::vector<gmx::RVec> vertNormRing;
            for(size_t k = 0; k < phi.size(); k ++)
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

                // distance vector from upper and lower circle centres:
                gmx::RVec distLower;
                rvec_sub(vertex, centres[idxLower], distLower);
                gmx::RVec distUpper;
                rvec_sub(vertex, centres[idxUpper], distUpper);

                // project distance vector onto tangent:
                real cosLower = iprod(tangents[idxLower], distLower);                
                real cosUpper = iprod(tangents[idxUpper], distUpper);                


                real angleThreshold = 0.5;
                if( cosUpper > angleThreshold || cosLower < angleThreshold )
                {
                    hasClashes = true;
                }

                vertRing[k] = vertex;


                // BELOW: second attempt to fix
                ///////////////////////////////////////////////////////////////



                // distance vector from upper and lower circle centres:
                gmx::RVec distLower;
                rvec_sub(vertex, centres[idxLower], distLower);
                gmx::RVec distUpper;
                rvec_sub(vertex, centres[idxUpper], distUpper);

                // project distance vector onto tangent:
                real cosLower = iprod(tangents[idxLower], distLower);                
                real cosUpper = iprod(tangents[idxUpper], distUpper);                
                numVert++;

                real angleThreshold = 0.0;


                if( cosLower < angleThreshold && cosUpper > angleThreshold )
                {
                    numSpecial++;
                    std::cout<<"SPECIAL CASE"<<std::endl;
//                    std::abort();
                }

                real test = iprod(tangents[idxLower], tangents[idxUpper]);

 
                // check for clashes:
                if( cosLower < angleThreshold )
                {
               std::cout<<"LOWER clash"
                        <<"numRings = "<<vertexRings.size()<<"  "
                         <<"idxLen = "<<idxLen<<"  "
                         <<"idxLower = "<<idxLower<<"  "
                         <<"idxUpper = "<<idxUpper<<"  "
//                         <<"lower.size = "<<vertexRings[idxLower].size()<<"  "
//                         <<"upper.size = "<<vertexRings[idxUpper].size()<<"  "
                         <<"cosLower = "<<cosLower<<"  "
                         <<"cosUpper = "<<cosUpper<<"  "
                         <<"dLo = "<<norm(distLower)<<"  "
                         <<"dUp = "<<norm(distUpper)<<"  "
                         <<"test = "<<test<<"  "
//                         <<"cux = "<<centres[idxUpper][XX]<<"  "
//                         <<"cuy = "<<centres[idxUpper][YY]<<"  "
//                         <<"cuz = "<<centres[idxUpper][ZZ]<<"  "
//                         <<"vx = "<<vertex[XX]<<"  "
//                         <<"vy = "<<vertex[YY]<<"  "
//                         <<"vz = "<<vertex[ZZ]<<"  "
                         <<std::endl;
                    clashLower++;
                    // reuse lower vertex:
                    gmx::RVec vert;
                    rvec_add(
                            vertexRings.at(idxLower).at(k), 
                            vertexRings.at(idxUpper).at(k), 
                            vert);
                    svmul(0.5, vert, vert);

//                    vertRing[k] = vertexRings.at(idxLower).at(k);
                    vertRing[k] = vertex;
                }
                else if( cosUpper > angleThreshold )
                {
               std::cout<<"UPPER clash"
                        <<"numRings = "<<vertexRings.size()<<"  "
                         <<"idxLen = "<<idxLen<<"  "
                         <<"idxLower = "<<idxLower<<"  "
                         <<"idxUpper = "<<idxUpper<<"  "
//                         <<"lower.size = "<<vertexRings[idxLower].size()<<"  "
//                         <<"upper.size = "<<vertexRings[idxUpper].size()<<"  "
                         <<"cosLower = "<<cosLower<<"  "
                         <<"cosUpper = "<<cosUpper<<"  "
                         <<"dLo = "<<norm(distLower)<<"  "
                         <<"dUp = "<<norm(distUpper)<<"  "
                         <<"test = "<<test<<"  "
//                         <<"cux = "<<centres[idxUpper][XX]<<"  "
//                         <<"cuy = "<<centres[idxUpper][YY]<<"  "
//                         <<"cuz = "<<centres[idxUpper][ZZ]<<"  "
//                         <<"vx = "<<vertex[XX]<<"  "
//                         <<"vy = "<<vertex[YY]<<"  "
//                         <<"vz = "<<vertex[ZZ]<<"  "
                         <<std::endl;
                    clashUpper++;
                    gmx::RVec vert;
                    rvec_add(
                            vertexRings.at(idxLower).at(k), 
                            vertexRings.at(idxUpper).at(k), 
                            vert);
                    svmul(0.5, vert, vert);
                    // reuse upper vertex:
//                    vertRing[k] = vertexRings.at(idxUpper).at(k);
                    vertRing[k] = vertex;
                     
                }
                else
                {
                    // use new vertex:
                    vertRing[k] = vertex;
                }





                vertNormRing.push_back(rotNormal);
            }
 


            if( hasClashes == true )
            {
                std::cout<<"has clashes!"<<std::endl;

                // form averaged tangent:
                rvec_add(tangents[idxLower], tangents[idxUpper], tangents[idxLen]);
                svmul(0.5, tangents[idxLen], tangents[idxLen]);
                unitv(tangents[idxLen], tangents[idxLen]);

                // recompute vertices:
                for(size_t k = 0; k < phi.size(); k ++)
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

                    vertRing[k] = vertex;
                }
            }















            for(auto vert : vertRing)
            {
                // distance vector from upper and lower circle centres:
                gmx::RVec distLower;
                rvec_sub(vert, centres[idxLower], distLower);
                gmx::RVec distUpper;
                rvec_sub(vert, centres[idxUpper], distUpper);

                // project distance vector onto tangent:
                real cosLower = iprod(tangents[idxLower], distLower);                
                real cosUpper = iprod(tangents[idxUpper], distUpper);                

                real angleThreshold = 0.0;
                if( cosLower < angleThreshold )
                {
                    std::cout<<"PERSISTENT LOWER CLASH"<<std::endl;
                    numPers++;
                }
                if( cosUpper > angleThreshold )
                {
                    std::cout<<"PERSISTENT Upper CLASH"<<std::endl;
                    numPers++;
                }
            }


            


            // add vertices to grid:
            for(size_t k = 0; k < vertRing.size(); k++)
            {
                grid.addVertex(idxLen, k, vertRing[k], prop[idxLen]);
            }
 
            // add vertex normals to grid:
            for(size_t k = 0; k < vertNormRing.size(); k++)
            {
                grid.addVertexNormal(idxLen, k, vertNormRing[k]);
            }

            // add vertices to ring storage:
            vertexRings[idxLen] = vertRing;


            // store 


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

    std::cout<<"numClash = "<<numClash<<std::endl;


    std::cout<<"clashUpper = "<<clashUpper<<"  "
             <<"clashLower = "<<clashLower<<"  "
             <<"numVert = "<<numVert<<"  "
             <<"numSpecial = "<<numSpecial<<"  "
             <<"numPers = "<<numPers<<"  "
             <<std::endl;

    // correct mesh:
    
    // FIXME I think the porblem is that interpolation in XYZ does also shift
    // the vertex in S direction. Might be better to interpolate R if a con-
    // flict is found...
//    grid.interpolateMissing();

    // return grid:
    return grid;
}
*/



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
int
MolecularPathObjExporter::numPlanarVertices(real &d, real &r)
{
    return std::max(static_cast<int>(std::ceil(M_PI/(2.0*std::acos(1.0 - d*d/(2.0*r*r))))), 4);
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
            std::cout<<"vex = "<<vertex[XX]<<"  "
                     <<"vey = "<<vertex[YY]<<"  "
                     <<"vez = "<<vertex[ZZ]<<"  "
                     <<"bx = "<<base[XX]<<"  "
                     <<"by = "<<base[YY]<<"  "
                     <<"bz = "<<base[ZZ]<<"  "
                     <<"rx = "<<rotNormal[XX]<<"  "
                     <<"ry = "<<rotNormal[YY]<<"  "
                     <<"rz = "<<rotNormal[ZZ]<<"  "
                     <<"radius = "<<radius<<"  "
                     <<std::endl;
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


/*
 *
 *
 */
real
MolecularPathObjExporter::cosAngle(
        const gmx::RVec &vecA,
        const gmx::RVec &vecB)
{
    return iprod(vecA, vecB) / ( norm(vecA) * norm(vecB) );
}


/*!
 * Shift ands scales all values in input vector so that they lie in the unit
 * interval.
 *
 * The divergent flag controls how the data is scaled. For a divergent scale, 
 * both positive and negative values are scaled by the same factor and then
 * shifted to the unit interval, in this case the zero of the scale is
 * precisely 0.5. For a sequential colour scale, all values are first shifted 
 * to the positive real range and then scaled to the unit interval, this does
 * obviously not preserve the zero of the original array.
 */
void
MolecularPathObjExporter::shiftAndScale(
        std::vector<real> &prop,
        bool divergent)
{
    // find data range:
    real minProp = *std::min_element(prop.begin(), prop.end()); 
    real maxProp = *std::max_element(prop.begin(), prop.end());
    
    // scale for divergent colour scale?
    if( std::fabs(maxProp - minProp) < std::numeric_limits<real>::epsilon() )
    {
        // special case of constant [roperty value, shift to middle of scale:
        real shift = -minProp + 0.5;

        // just shift values in this case:
        std::for_each(prop.begin(), prop.end(), [shift](real &p){p += shift;});
    }
    else if( divergent == false )
    {
        // for sequential colour scale, shift data to positive real range and
        // scale by length of data interval:
        real shift = -minProp;
        real scale = 1.0/(maxProp - minProp);  
       
        // shift and scale the property array:
        std::for_each(prop.begin(), prop.end(), [shift](real &p){p += shift;});
        std::for_each(prop.begin(), prop.end(), [scale](real &p){p *= scale;});
    }
    else
    {
        // for divergent colour scale, scale both positive and negative values
        // such that data lies in [-0.5, 0.5], then shift by 0.5:
        real shift = 0.5;
        real scale = 1.0/std::max(std::fabs(minProp), std::fabs(maxProp))/2.0; 

        // scale and shift:
        std::for_each(prop.begin(), prop.end(), [scale](real &p){p *= scale;});
        std::for_each(prop.begin(), prop.end(), [shift](real &p){p += shift;});
    }

}

