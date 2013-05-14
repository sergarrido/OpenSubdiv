//
//     Copyright (C) Pixar. All rights reserved.
//
//     This license governs use of the accompanying software. If you
//     use the software, you accept this license. If you do not accept
//     the license, do not use the software.
//
//     1. Definitions
//     The terms "reproduce," "reproduction," "derivative works," and
//     "distribution" have the same meaning here as under U.S.
//     copyright law.  A "contribution" is the original software, or
//     any additions or changes to the software.
//     A "contributor" is any person or entity that distributes its
//     contribution under this license.
//     "Licensed patents" are a contributor's patent claims that read
//     directly on its contribution.
//
//     2. Grant of Rights
//     (A) Copyright Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free copyright license to reproduce its contribution,
//     prepare derivative works of its contribution, and distribute
//     its contribution or any derivative works that you create.
//     (B) Patent Grant- Subject to the terms of this license,
//     including the license conditions and limitations in section 3,
//     each contributor grants you a non-exclusive, worldwide,
//     royalty-free license under its licensed patents to make, have
//     made, use, sell, offer for sale, import, and/or otherwise
//     dispose of its contribution in the software or derivative works
//     of the contribution in the software.
//
//     3. Conditions and Limitations
//     (A) No Trademark License- This license does not grant you
//     rights to use any contributor's name, logo, or trademarks.
//     (B) If you bring a patent claim against any contributor over
//     patents that you claim are infringed by the software, your
//     patent license from such contributor to the software ends
//     automatically.
//     (C) If you distribute any portion of the software, you must
//     retain all copyright, patent, trademark, and attribution
//     notices that are present in the software.
//     (D) If you distribute any portion of the software in source
//     code form, you may do so only under this license by including a
//     complete copy of this license with your distribution. If you
//     distribute any portion of the software in compiled or object
//     code form, you may only do so under a license that complies
//     with this license.
//     (E) The software is licensed "as-is." You bear the risk of
//     using it. The contributors give no express warranties,
//     guarantees or conditions. You may have additional consumer
//     rights under your local laws which this license cannot change.
//     To the extent permitted under your local laws, the contributors
//     exclude the implied warranties of merchantability, fitness for
//     a particular purpose and non-infringement.
//

#ifndef FAR_PATCH_TABLES_FACTORY_H
#define FAR_PATCH_TABLES_FACTORY_H

#include "../version.h"

#include "../far/patchTables.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

/// \brief A specialized factory for feature adaptive FarPatchTables
///
/// FarPatchTables contain the lists of vertices for each patch of an adaptive
/// mesh representation. This  specialized factory is a private helper for FarMeshFactory.
///
/// Separating the factory allows us to isolate Far data structures from Hbr dependencies.
///
template <class T> class FarPatchTablesFactory {

protected:
    template <class X, class Y> friend class FarMeshFactory;
    
    /// Factory constructor
    ///
    /// @param mesh Hbr mesh to generate tables for
    ///
    /// @param nfaces Number of faces in the mesh (cached for speed)
    ///
    /// @param remapTable Vertex remapping table generated by FarMeshFactory
    ///
    FarPatchTablesFactory( HbrMesh<T> const * mesh, int nfaces, std::vector<int> const & remapTable );
    
    /// Returns a FarPatchTables instance 
    ///
    /// @param maxlevel Highest level of refinement processed
    ///
    /// @param maxvalence Maximum vertex valence in the mesh
    ///
    /// @param requireFVarData Flag for generating face-varying data
    ///
    /// @return a new instance of FarPatchTables
    ///
    FarPatchTables * Create( int maxlevel, int maxvalence, bool requireFVarData=false );

private:

    // Returns true if one of v's neighboring faces has vertices carrying the tag "wasTagged"
    static bool vertexHasTaggedNeighbors(HbrVertex<T> * v);

    // Returns the rotation for a boundary patch
    static unsigned char computeBoundaryPatchRotation( HbrFace<T> * f );

    // Returns the rotation for a corner patch
    static unsigned char computeCornerPatchRotation( HbrFace<T> * f );

    // Populates an array of indices with the "one-ring" vertices for the given face
    void getOneRing( HbrFace<T> * f, int ringsize, unsigned int const * remap, unsigned int * result );
    
    // Populates the Gregory patch quad offsets table
    static void getQuadOffsets( HbrFace<T> * f, unsigned int * result );

    // Hbr mesh accessor
    HbrMesh<T> const * getMesh() const { return _mesh; }

    // Number of faces in the Hbr mesh (cached for speed)
    int getNumFaces() const { return _nfaces; }

    // The number of patch arrays in the mesh
    int getNumPatchArrays() const;

    // A convenience container for the different types of feature adaptive patches
    template<class TYPE> struct PatchTypes {
        TYPE R,       // regular patch 
             B[4],    // boundary patch (4 rotations)
             C[4],    // corner patch (4 rotations)
             G[2];    // gregory patch (boundary & corner)
        
        PatchTypes() { memset(this, 0, sizeof(PatchTypes<TYPE>)); }
        
        // Returns the number of patches based on the patch type in the descriptor
        TYPE & getValue( FarPatchTables::Descriptor desc );
        
        // Counts the number of arrays required to store each type of patch used
        // in the primitive
        int getNumPatchArrays() const;
    };

    // Useful typedefs 
    typedef PatchTypes<unsigned int*>  CVPointers;
    typedef PatchTypes<FarPtexCoord *> PtexPointers;
    typedef PatchTypes<float *>        FVarPointers;
    typedef PatchTypes<int>            Counter;
    
    // Creates a PatchArray and appends it to a vector and keeps track of both
    // vertex and patch offsets
    void pushPatchArray( FarPatchTables::Descriptor desc,
                         FarPatchTables::PatchArrayVector & parray,
                         Counter & counter,                                 
                         int * voffset, int * poffset, int * qoffset );

    Counter _patchCtr[6];  // counters for full and transition patches
             
    HbrMesh<T> const * _mesh;

    // Reference to the vertex remapping table generated by FarMeshFactory
    std::vector<int> const &_remapTable;
    
    int _nfaces;
};

template <class T> bool
FarPatchTablesFactory<T>::vertexHasTaggedNeighbors(HbrVertex<T> * v) {

    assert(v);
    
    HbrHalfedge<T> * start = v->GetIncidentEdge(),
                   * next=start;
    do {
        HbrFace<T> * right = next->GetRightFace(),
                   * left = next->GetLeftFace();

        if (right and (not right->hasTaggedVertices()))
            return true;

        if (left and (not left->hasTaggedVertices()))
            return true;

        next = v->GetNextEdge(next);

    } while (next and next!=start);
    return false;
}

// Returns a rotation index for boundary patches (range [0-3])
template <class T> unsigned char 
FarPatchTablesFactory<T>::computeBoundaryPatchRotation( HbrFace<T> * f ) {
    unsigned char rot=0;
    for (unsigned char i=0; i<4;++i) {
        if (f->GetVertex(i)->OnBoundary() and
            f->GetVertex((i+1)%4)->OnBoundary())
            break;
        ++rot;
    }
    return rot;
}

// Returns a rotation index for corner patches (range [0-3])
template <class T> unsigned char 
FarPatchTablesFactory<T>::computeCornerPatchRotation( HbrFace<T> * f ) {
    unsigned char rot=0;
    for (unsigned char i=0; i<4; ++i) {
        if (not f->GetVertex((i+3)%4)->OnBoundary())
            break;
        ++rot;
    }
    return rot;
}


template <class T>
FarPatchTablesFactory<T>::FarPatchTablesFactory( HbrMesh<T> const * mesh, int nfaces,  std::vector<int> const & remapTable ) :
    _mesh(mesh),
    _remapTable(remapTable),
    _nfaces(nfaces)
{
    assert(mesh and nfaces>0);

    // First pass : identify transition / watertight-critical
    for (int i=0; i<nfaces; ++i) {

        HbrFace<T> * f = mesh->GetFace(i);

        if (f->_adaptiveFlags.isTagged and (not f->IsHole())) {
            HbrVertex<T> * v = f->Subdivide();
            assert(v);
            v->_adaptiveFlags.wasTagged=true;
        }

        int nv = f->GetNumVertices();
        for (int j=0; j<nv; ++j) {
        
            if (f->IsCoarse())
                f->GetVertex(j)->_adaptiveFlags.wasTagged=true;
                    
            HbrHalfedge<T> * e = f->GetEdge(j);

            // Flag transition edge that require a triangulated transition
            if (f->_adaptiveFlags.isTagged) {
                e->_adaptiveFlags.isTriangleHead=true;

                // Both half-edges need to be tagged if an opposite exists
                if (e->GetOpposite())
                    e->GetOpposite()->_adaptiveFlags.isTriangleHead=true;
            }
            
            HbrFace<T> * left = e->GetLeftFace(),
                       * right = e->GetRightFace();
            
            if (not (left and right))
                continue;
            
            // a tagged edge w/ no children is inside a hole
            if (e->HasChild() and (left->_adaptiveFlags.isTagged ^ right->_adaptiveFlags.isTagged)) {
                
                e->_adaptiveFlags.isTransition = true;
                
                HbrVertex<T> * child = e->Subdivide();
                assert(child);
                
                // These edges will require extra rows of CVs to maintain water-tightness
                // Note : vertices inside holes have no children
                if (e->GetOrgVertex()->HasChild()) {
                    HbrHalfedge<T> * org = child->GetEdge(e->GetOrgVertex()->Subdivide());
                    if (org)
                        org->_adaptiveFlags.isWatertightCritical=true;
                }

                if (e->GetDestVertex()->HasChild()) {
                    HbrHalfedge<T> * dst = child->GetEdge(e->GetDestVertex()->Subdivide());
                    if (dst)
                        dst->_adaptiveFlags.isWatertightCritical=true;
                }
            }
        }
    }

    // Second pass : count boundaries / identify transition constellation
    for (int i=0; i<nfaces; ++i) {

        HbrFace<T> * f = mesh->GetFace(i);

        if (mesh->GetSubdivision()->FaceIsExtraordinary(mesh,f))
            continue;
            
        if (f->IsHole())
            continue;
           
        bool isTagged=0, wasTagged=0, isConnected=0, isWatertightCritical=0, isExtraordinary=0;
        int  triangleHeads=0, boundaryVerts=0;

        int nv = f->GetNumVertices();
        for (int j=0; j<nv; ++j) {
            HbrVertex<T> * v = f->GetVertex(j);
            
            if (v->OnBoundary()) {
                boundaryVerts++;
                
                // Boundary vertices with valence higher than 3 aren't Full Boundary
                // patches, they are Gregory Boundary patches.
                if (v->IsSingular() or v->GetValence()>3)
                    isExtraordinary=true;
                    
            } else if (v->IsExtraordinary())
                isExtraordinary=true;
                
            if (f->GetParent() and (not isWatertightCritical))
                isWatertightCritical = vertexHasTaggedNeighbors(v);
            
            if (v->_adaptiveFlags.isTagged)
                isTagged=1;

            if (v->_adaptiveFlags.wasTagged)
                wasTagged=1;

            // Count the number of triangle heads to find which transition
            // pattern to use.
            HbrHalfedge<T> * e = f->GetEdge(j);
            if (e->_adaptiveFlags.isTriangleHead) {

                ++triangleHeads;
                if (f->GetEdge((j+1)%4)->_adaptiveFlags.isTriangleHead)
                    isConnected=true;
            }
        }

        f->_adaptiveFlags.bverts=boundaryVerts;
        f->_adaptiveFlags.isCritical=isWatertightCritical;

        // Regular Boundary Patch
        if (wasTagged)
            // XXXX manuelk - need to implement end patches
            f->_adaptiveFlags.patchType = HbrFace<T>::kEnd;
        
        if (f->_adaptiveFlags.isTagged)
            continue;
        
        assert(f->_adaptiveFlags.rots==0 and nv==4);
        
        if (not isTagged and wasTagged) {

            if (triangleHeads==0) {

                 if (not isExtraordinary and boundaryVerts!=1) {

                    // Full Patches
                    f->_adaptiveFlags.patchType = HbrFace<T>::kFull;

                    switch (boundaryVerts) {
                    
                        case 0 : {   // Regular patch
                                     _patchCtr[0].R++;
                                 } break; 
                        
                        case 2 : {   // Boundary patch
                                     f->_adaptiveFlags.rots=computeBoundaryPatchRotation(f);
                                     _patchCtr[0].B[0]++;
                                 } break;
                        
                        case 3 : {   // Corner patch
                                     f->_adaptiveFlags.rots=computeCornerPatchRotation(f);
                                     _patchCtr[0].C[0]++;
                                 } break;
                        
                        default : break;
                    }
                } else {
                
                    // Default to Gregory Patch
                    f->_adaptiveFlags.patchType = HbrFace<T>::kGregory;

                    switch (boundaryVerts) {
                    
                        case 0 : {   // Regular Gregory patch
                                     _patchCtr[0].G[0]++;
                                 } break; 
 
                        
                        default : { // Boundary Gregory patch
                                     _patchCtr[0].G[1]++;
                                  } break;
                    }
                }
            
            } else {
                
                // Transition Patch
                
                // Resolve transition constellation : 5 types (see p.5 fig. 7)
                switch (triangleHeads) {

                    case 1 : {   for (unsigned char j=0; j<4; ++j) {
                                     if (f->GetEdge(j)->IsTriangleHead())
                                         break;
                                     f->_adaptiveFlags.rots++;
                                 }
                                 f->_adaptiveFlags.transitionType = HbrFace<T>::kTransition0;
                            } break;

                    case 2 : {   for (unsigned char j=0; j<4; ++j) {
                                     if (isConnected) {
                                         if (f->GetEdge(j)->IsTriangleHead() and 
                                             f->GetEdge((j+3)%4)->IsTriangleHead())
                                             break;
                                     } else {
                                         if (f->GetEdge(j)->IsTriangleHead())
                                             break;
                                     }
                                     f->_adaptiveFlags.rots++;
                                 }

                                 if (isConnected)
                                    f->_adaptiveFlags.transitionType = HbrFace<T>::kTransition1;
                                 else
                                    f->_adaptiveFlags.transitionType = HbrFace<T>::kTransition4;
                             } break;

                    case 3 : {   for (unsigned char j=0; j<4; ++j) {
                                     if (not f->GetEdge(j)->IsTriangleHead())
                                         break;
                                     f->_adaptiveFlags.rots++;
                                 }
                                 f->_adaptiveFlags.transitionType = HbrFace<T>::kTransition2;
                             } break;

                    case 4 : f->_adaptiveFlags.transitionType = HbrFace<T>::kTransition3;
                             break;

                    default: break;
                }

                int tidx = f->_adaptiveFlags.transitionType;
                assert(tidx>=0);
                
                // Correct rotations for corners & boundaries
                if (not isExtraordinary and boundaryVerts!=1) {
                    
                    switch (boundaryVerts) {
                    
                        case 0 : {   // regular patch
                                     _patchCtr[tidx+1].R++;
                                 } break; 
                        
                        case 2 : {   // boundary patch
                                     unsigned char rot=computeBoundaryPatchRotation(f);

                                     f->_adaptiveFlags.brots=(4-f->_adaptiveFlags.rots+rot)%4;
                                     
                                     f->_adaptiveFlags.rots=rot; // override the transition rotation
                                     
                                     _patchCtr[tidx+1].B[f->_adaptiveFlags.brots]++;
                                 } break;
                        
                        case 3 : {   // corner patch
                                     unsigned char rot=computeCornerPatchRotation(f);
                                     
                                     f->_adaptiveFlags.brots=(4-f->_adaptiveFlags.rots+rot)%4;
                                     
                                     f->_adaptiveFlags.rots=rot; // override the transition rotation
                                     
                                     _patchCtr[tidx+1].C[f->_adaptiveFlags.brots]++;
                                 } break;
                        
                        default : assert(0); break;
                    }
                } else {
                    // Use Gregory Patch transition ?
                }
            }
        }
    }
}

template <class T> 
    template <class TYPE> TYPE & 
FarPatchTablesFactory<T>::PatchTypes<TYPE>::getValue( FarPatchTables::Descriptor desc ) {
    switch (desc.GetType()) {
        case FarPatchTables::REGULAR          : return R;
        case FarPatchTables::BOUNDARY         : return B[desc.GetRotation()];
        case FarPatchTables::CORNER           : return C[desc.GetRotation()];
        case FarPatchTables::GREGORY          : return G[0];
        case FarPatchTables::GREGORY_BOUNDARY : return G[1];
        default : assert(0);
    }
    // can't be reached (suppress compiler warning)
    return R;
}

template <class T> 
    template <class TYPE> int
FarPatchTablesFactory<T>::PatchTypes<TYPE>::getNumPatchArrays() const {

    int result=0;

    if (R) ++result;
    for (int i=0; i<4; ++i) {
        if (B[i]) ++result;
        if (C[i]) ++result;
        if ((i<2) and G[i]) ++result;
    }
    return result;
}

template <class T> int 
FarPatchTablesFactory<T>::getNumPatchArrays() const {

    int result = 0;
    
    for (int i=0; i<6; ++i)
        result += _patchCtr[i].getNumPatchArrays();
        
    return result;
}

template <class T> void
FarPatchTablesFactory<T>::pushPatchArray( FarPatchTables::Descriptor desc, 
                                          FarPatchTables::PatchArrayVector & parray, 
                                          typename FarPatchTablesFactory<T>::Counter & counter, 
                                          int * voffset, int * poffset, int * qoffset ) {

    int npatches = counter.getValue( desc );
    
    if (npatches>0) {
        parray.push_back( FarPatchTables::PatchArray(desc, *voffset, *poffset, npatches, *qoffset) );

        *voffset += npatches * desc.GetNumControlVertices();
        *poffset += npatches;
        *qoffset += (desc.GetType() == FarPatchTables::GREGORY) ? npatches * 4 : 0;
    }
}

template <class T> FarPatchTables *
FarPatchTablesFactory<T>::Create( int maxlevel, int maxvalence, bool requireFVarData ) {

    static const unsigned int remapRegular        [16] = {5,6,10,9,4,0,1,2,3,7,11,15,14,13,12,8};
    static const unsigned int remapRegularBoundary[12] = {1,2,6,5,0,3,7,11,10,9,8,4};
    static const unsigned int remapRegularCorner  [ 9] = {1,2,5,4,0,8,7,6,3};

    assert(getMesh() and getNumFaces()>0);

    typedef FarPatchTables::Descriptor Descriptor;


    FarPatchTables * result = new FarPatchTables(maxvalence);
    
    // Populate the patch array descriptors
    FarPatchTables::PatchArrayVector & parray = result->_patchArrays;
    parray.reserve( getNumPatchArrays() );

    int voffset=0, poffset=0, qoffset=0;


    for (Descriptor::iterator it=Descriptor::begin(); it!=Descriptor::end(); ++it) {
        pushPatchArray( *it, parray, _patchCtr[it->GetPattern()], &voffset, &poffset, &qoffset );
    }

    int nverts = result->GetNumControlVertices(),
        npatches = result->GetNumPatches(),
        fvarwidth = getMesh()->GetTotalFVarWidth();

    // Reserve memory for the tables
    result->_patches.resize( nverts );

    // Allocate memory for the ptex coord tables
    result->_ptexTable.resize( npatches );

    if (requireFVarData) {
        result->_fvarTable.resize( npatches * 4 * fvarwidth );
    }

    FarPatchTables::QuadOffsetTable quad_G_C0; // Quad-offsets tables (for Gregory patches)
    quad_G_C0.resize(_patchCtr[0].G[0]*4);

    FarPatchTables::QuadOffsetTable quad_G_C1;
    quad_G_C1.resize(_patchCtr[0].G[1]*4);

    FarPatchTables::QuadOffsetTable::value_type *quad_G_C0_P = quad_G_C0.empty() ? 0 : &quad_G_C0[0];
    FarPatchTables::QuadOffsetTable::value_type *quad_G_C1_P = quad_G_C1.empty() ? 0 : &quad_G_C1[0];

    CVPointers    iptrs[6];
    PtexPointers  pptrs[6];
    FVarPointers  fptrs[6];

    for (Descriptor::iterator it=Descriptor::begin(); it!=Descriptor::end(); ++it) {
    
        FarPatchTables::PatchArray * pa = result->findPatchArray(*it);

        if (not pa)
            continue;

        iptrs[(int)pa->GetDescriptor().GetPattern()].getValue( *it ) = &result->_patches[pa->GetVertIndex()];
        pptrs[(int)pa->GetDescriptor().GetPattern()].getValue( *it ) = &result->_ptexTable[pa->GetPatchIndex()];
        if (requireFVarData)
            fptrs[(int)pa->GetDescriptor().GetPattern()].getValue( *it ) = &result->_fvarTable[pa->GetPatchIndex() * 4 * fvarwidth];
    }
 
    // Populate patch index tables with vertex indices
    for (int i=0; i<getNumFaces(); ++i) {
        
        HbrFace<T> * f = getMesh()->GetFace(i);
    
        if (not f->isTransitionPatch() ) {
        
            // Full / End patches

            if (f->_adaptiveFlags.patchType==HbrFace<T>::kFull) {
                if (not f->_adaptiveFlags.isExtraordinary and f->_adaptiveFlags.bverts!=1) {

                    switch (f->_adaptiveFlags.bverts) {
                        case 0 : {   // Regular Patch (16 CVs)
                                     getOneRing(f, 16, remapRegular, iptrs[0].R);
                                     iptrs[0].R+=16;
                                     pptrs[0].R = computePtexCoordinate(f, pptrs[0].R);
                                     fptrs[0].R = computeFVarData(f, fvarwidth, fptrs[0].R, /*isAdaptive=*/true);
                                 } break;

                        case 2 : {   // Boundary Patch (12 CVs)
                                     f->_adaptiveFlags.brots = (f->_adaptiveFlags.rots+1)%4;
                                     getOneRing(f, 12, remapRegularBoundary, iptrs[0].B[0]);
                                     iptrs[0].B[0]+=12;
                                     pptrs[0].B[0] = computePtexCoordinate(f, pptrs[0].B[0]);
                                     fptrs[0].B[0] = computeFVarData(f, fvarwidth, fptrs[0].B[0], /*isAdaptive=*/true);
                                 } break;

                        case 3 : {   // Corner Patch (9 CVs)
                                     f->_adaptiveFlags.brots = (f->_adaptiveFlags.rots+1)%4;
                                     getOneRing(f, 9, remapRegularCorner, iptrs[0].C[0]);
                                     iptrs[0].C[0]+=9;
                                     pptrs[0].C[0] = computePtexCoordinate(f, pptrs[0].C[0]);
                                     fptrs[0].C[0] = computeFVarData(f, fvarwidth, fptrs[0].C[0], /*isAdaptive=*/true);
                                 } break;

                        default : assert(0);
                    }
                }
            } else if (f->_adaptiveFlags.patchType==HbrFace<T>::kGregory) {

                if (f->_adaptiveFlags.bverts==0) {
                
                    // Gregory Regular Patch (4 CVs + quad-offsets / valence tables)
                    for (int j=0; j<4; ++j)
                        iptrs[0].G[0][j] = _remapTable[f->GetVertex(j)->GetID()];
                    iptrs[0].G[0]+=4;
                    getQuadOffsets(f, quad_G_C0_P);
                    quad_G_C0_P += 4;
                    pptrs[0].G[0] = computePtexCoordinate(f, pptrs[0].G[0]);
                    fptrs[0].G[0] = computeFVarData(f, fvarwidth, fptrs[0].G[0], /*isAdaptive=*/true);
                } else {
                
                    // Gregory Boundary Patch (4 CVs + quad-offsets / valence tables)
                    for (int j=0; j<4; ++j)
                        iptrs[0].G[1][j] = _remapTable[f->GetVertex(j)->GetID()];
                    iptrs[0].G[1]+=4;
                    getQuadOffsets(f, quad_G_C1_P);
                    quad_G_C1_P += 4;
                    pptrs[0].G[1] = computePtexCoordinate(f, pptrs[0].G[1]);
                    fptrs[0].G[1] = computeFVarData(f, fvarwidth, fptrs[0].G[1], /*isAdaptive=*/true);
                }
            } else {
                // XXXX manuelk - end patches here
            }
        } else {
         
            // Transition patches
            
            int tcase = f->_adaptiveFlags.transitionType;
            assert( tcase>=HbrFace<T>::kTransition0 and tcase<=HbrFace<T>::kTransition4 );
            ++tcase;  // TransitionPattern begin with NON_TRANSITION

            if (not f->_adaptiveFlags.isExtraordinary and f->_adaptiveFlags.bverts!=1) {

                switch (f->_adaptiveFlags.bverts) {
                    case 0 : {   // Regular Transition Patch (16 CVs)
                                 getOneRing(f, 16, remapRegular, iptrs[tcase].R);

                                 iptrs[tcase].R+=16;
                                 pptrs[tcase].R = computePtexCoordinate(f, pptrs[tcase].R);
                                 fptrs[tcase].R = computeFVarData(f, fvarwidth, fptrs[tcase].R, /*isAdaptive=*/true);
                             } break;

                    case 2 : {   // Boundary Transition Patch (12 CVs)
                                 unsigned rot = f->_adaptiveFlags.brots;
                                 getOneRing(f, 12, remapRegularBoundary, iptrs[tcase].B[rot]);
                                 iptrs[tcase].B[rot]+=12;
                                 pptrs[tcase].B[rot] = computePtexCoordinate(f, pptrs[tcase].B[rot]);
                                 fptrs[tcase].B[rot] = computeFVarData(f, fvarwidth, fptrs[tcase].B[rot], /*isAdaptive=*/true);
                             } break;

                    case 3 : {   // Corner Transition Patch (9 CVs)
                                 unsigned rot = f->_adaptiveFlags.brots;
                                 getOneRing(f, 9, remapRegularCorner, iptrs[tcase].C[rot]);
                                 iptrs[tcase].C[rot]+=9;
                                 pptrs[tcase].C[rot] = computePtexCoordinate(f, pptrs[tcase].C[rot]);
                                 fptrs[tcase].C[rot] = computeFVarData(f, fvarwidth, fptrs[tcase].C[rot], /*isAdaptive=*/true);
                             } break;
                }
            } else
                // No transition Gregory patches
                assert(false);
        }
    }
     
    // Build Gregory patches vertex valence indices table
    if ((_patchCtr[0].G[0] > 0) or (_patchCtr[0].G[1] > 0)) {

        // MAX_VALENCE is a property of hardware shaders and needs to be matched in OSD
        const int perVertexValenceSize = 2*maxvalence + 1;

        const int nverts = getMesh()->GetNumVertices();

        FarPatchTables::VertexValenceTable & table = result->_vertexValenceTable;
        table.resize(nverts * perVertexValenceSize);

        class GatherNeighborsOperator : public HbrVertexOperator<T> {
        public:
            HbrVertex<T> * center;
            FarPatchTables::VertexValenceTable & table;
            int offset, valence;
            std::vector<int> const & remap;

            GatherNeighborsOperator(FarPatchTables::VertexValenceTable & itable, int ioffset, HbrVertex<T> * v, std::vector<int> const & iremap) : 
                center(v), table(itable), offset(ioffset), valence(0), remap(iremap) { }

            // Operator iterates over neighbor vertices of v and accumulates
            // pairs of indices the neighbor and diagonal vertices
            //
            //          Regular case
            //                                           Boundary case
            //      o ------- o      D3 o
            //   D0        N0 |         |
            //                |         |             o ------- o      D2 o
            //                |         |          D0        N0 |         |
            //                |         |                       |         |
            //      o ------- o ------- o                       |         |
            //   N1 |       V |      N3                         |         |
            //      |         |                       o ------- o ------- o
            //      |         |                    N1          V       N2
            //      |         |
            //      o         o ------- o
            //   D1         N2        D2
            //
            virtual void operator() (HbrVertex<T> &v) {

                table[offset++] = remap[v.GetID()];

                HbrVertex<T> * diagonal=&v;

                HbrHalfedge<T> * e = center->GetEdge(&v);
                if ( e ) {
                    // If v is on a boundary, there may not be a diagonal vertex
                    diagonal = e->GetNext()->GetDestVertex();
                }
                //else {
                //    diagonal = v.GetQEONext( center );
                //}

                table[offset++] = remap[diagonal->GetID()];

                ++valence;
            }
        };

        for (int i=0; i<nverts; ++i) {
            HbrVertex<T> * v = getMesh()->GetVertex(i);

            int outputVertexID = _remapTable[v->GetID()];
            int offset = outputVertexID * perVertexValenceSize;

            // feature adaptive refinement can generate un-connected face-vertices
            // that have a valence of 0
            if (not v->IsConnected()) {
                assert( v->GetParentFace() );
                table[offset] = 0;
                continue;
            }

            // "offset+1" : the first table entry is the vertex valence, which
            // is gathered by the operator (see note below)
            GatherNeighborsOperator op( table, offset+1, v, _remapTable );
            v->ApplyOperatorSurroundingVertices( op );

            // Valence sign bit used to mark boundary vertices
            table[offset] = v->OnBoundary() ? -op.valence : op.valence;

            // Note : some topologies can cause v to be singular at certain
            // levels of adaptive refinement, which prevents us from using
            // the GetValence() function. Fortunately, the GatherNeighbors
            // operator above just performed a similar traversal, so it is
            // very convenient to use it to accumulate the actionable valence.
        }
    } else {
        result->_vertexValenceTable.clear();
    }

    // Combine quad offset buffers
    result->_quadOffsetTable.resize((_patchCtr[0].G[0]+_patchCtr[0].G[1])*4);
    std::copy(quad_G_C0.begin(), quad_G_C0.end(), result->_quadOffsetTable.begin());
    std::copy(quad_G_C1.begin(), quad_G_C1.end(), result->_quadOffsetTable.begin()+_patchCtr[0].G[0]*4);

    return result;
}

// The One Ring vertices to rule them all !
template <class T> void 
FarPatchTablesFactory<T>::getOneRing( HbrFace<T> * f, int ringsize, unsigned int const * remap, unsigned int * result) {

    assert( f and f->GetNumVertices()==4 and ringsize >=4 );

    int idx=0;

    for (unsigned char i=0; i<4; ++i)
        result[remap[idx++ % ringsize]] = _remapTable[f->GetVertex( (i+f->_adaptiveFlags.rots)%4 )->GetID()];

    if (ringsize==16) {

        // Regular case
        //
        //       |      |      |      |      
        //       | 4    | 15   | 14   | 13    
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       | 5    | 0    | 3    | 12    
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       | 6    | 1    | 2    | 11    
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       | 7    | 8    | 9    | 10    
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       |      |      |      |      

        for (int i=0; i<4; ++i) {
            int rot = i+f->_adaptiveFlags.rots;
            HbrVertex<T> * v0 = f->GetVertex( rot % 4 ),
                         * v1 = f->GetVertex( (rot+1) % 4 );

            HbrHalfedge<T> * e = v0->GetNextEdge( v0->GetNextEdge( v0->GetEdge(v1) ) );

            for (int j=0; j<3; ++j) {
                e = e->GetNext();
                result[remap[idx++ % ringsize]] = _remapTable[e->GetOrgVertex()->GetID()];
            }
        }            
    } else if (ringsize==12) {

        // Boundary case
        //
        //         4      0      3      5
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       | 11   | 1    | 2    | 6    
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       | 10   | 9    | 8    | 7    
        //  ---- o ---- o ---- o ---- o ---- 
        //       |      |      |      |      
        //       |      |      |      |      

        HbrVertex<T> * v[4];
        for (int i=0; i<4; ++i)
            v[i] = f->GetVertex( (i+f->_adaptiveFlags.rots)%4 );

        HbrHalfedge<T> * e;

        e = v[0]->GetIncidentEdge()->GetPrev()->GetOpposite()->GetPrev();
        result[remap[idx++ % ringsize]] = _remapTable[e->GetOrgVertex()->GetID()];

        e = v[1]->GetIncidentEdge();
        result[remap[idx++ % ringsize]] = _remapTable[e->GetDestVertex()->GetID()];

        e = v[2]->GetNextEdge( v[2]->GetEdge(v[1]) );
        for (int i=0; i<3; ++i) {
            e = e->GetNext();
            result[remap[idx++ % ringsize]] = _remapTable[e->GetOrgVertex()->GetID()];
        }

        e = v[3]->GetNextEdge( v[3]->GetEdge(v[2]) );
        for (int i=0; i<3; ++i) {
            e = e->GetNext();
            result[remap[idx++ % ringsize]] = _remapTable[e->GetOrgVertex()->GetID()];
        }
    } else if (ringsize==9) {

        // Corner case
        //
        //     0      1      4
        //   o ---- o ---- o ----
        //   |      |      |
        //   | 3    | 2    | 5
        //   o ---- o ---- o ----
        //   |      |      |
        //   | 8    | 7    | 6
        //   o ---- o ---- o ----
        //   |      |      |
        //   |      |      |

        HbrVertex<T> * v0 = f->GetVertex( (0+f->_adaptiveFlags.rots)%4 ),
                     * v2 = f->GetVertex( (2+f->_adaptiveFlags.rots)%4 ),
                     * v3 = f->GetVertex( (3+f->_adaptiveFlags.rots)%4 );

        HbrHalfedge<T> * e;

        e = v0->GetIncidentEdge()->GetPrev()->GetOpposite()->GetPrev();
        result[remap[idx++ % ringsize]] = _remapTable[e->GetOrgVertex()->GetID()];

        e = v2->GetIncidentEdge();
        result[remap[idx++ % ringsize]] = _remapTable[e->GetDestVertex()->GetID()];

        e = v3->GetNextEdge( v3->GetEdge(v2) );
        for (int i=0; i<3; ++i) {
            e = e->GetNext();
            result[remap[idx++ % ringsize]] = _remapTable[e->GetOrgVertex()->GetID()];
        }
    }
    assert(idx==ringsize);
}

// Populate the quad-offsets table used by Gregory patches
template <class T> void
FarPatchTablesFactory<T>::getQuadOffsets( HbrFace<T> * f, unsigned int * result ) {

    assert( f and f->GetNumVertices()==4 );
    
    // Builds a table of value pairs for each vertex of the patch.
    // 
    //            o
    //         N0 |
    //            |
    //            |
    //   o ------ o ------ o
    // N1       V | .... M3
    //            | .......
    //            | .......
    //            o .......
    //          N2
    //
    // [...] [N2 - N3] [...]
    //
    // Each value pair is composed of 2 index values in range [0-4[ pointing
    // to the 2 neighbor vertices to the vertex that belong to the Gregory patch. 
    // Neighbor ordering is valence counter-clockwise and must match the winding
    // used to build the vertexValenceTable.
    // 

    class GatherOffsetsOperator : public HbrVertexOperator<T> {
    public:
        HbrVertex<T> ** verts; int offsets[2]; int index; int count;

        GatherOffsetsOperator(HbrVertex<T> ** iverts) : verts(iverts) { }

        void reset() {
            index=count=offsets[0]=offsets[1]=0;
        }

        virtual void operator() (HbrVertex<T> &v) {
             // Resolve which 2 neighbor vertices of v belong to the Gregory patch
             for (unsigned char i=0; i<4; ++i)
                if (&v==verts[i]) {
                    assert(count<3);
                    offsets[count++]=index;
                    break;
                }
            ++index;
        }
    };

    // 4 central CVs of the Gregory patch
    HbrVertex<T> * fvs[4] = { f->GetVertex(0),
                              f->GetVertex(1),
                              f->GetVertex(2),
                              f->GetVertex(3) };


    // Hbr vertex operator that iterates over neighbor vertices
    GatherOffsetsOperator op( fvs );

    for (unsigned char i=0; i<4; ++i) {
    
        op.reset();
    
        fvs[i]->ApplyOperatorSurroundingVertices( op );
  
        if (op.offsets[1] - op.offsets[0] != 1)
            std::swap(op.offsets[0], op.offsets[1]);

        // Pack the 2 indices in 16 bits
        result[i] = (op.offsets[0] | (op.offsets[1] << 8));
    }
}

} // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

} // end namespace OpenSubdiv

#endif /* FAR_PATCH_TABLES_FACTORY_H */
