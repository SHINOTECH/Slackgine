/////////////////////////////////////////////////////////////
//
// Slackgine - Copyright (C) 2010-2012
// The Slackgine development team
//
// See the LICENSE file in the top-level directory.
//
// FILE:        blender.cpp
// PURPOSE:     Import models from .blend files.
// AUTHORS:     Alberto Alonso <rydencillo@gmail.com>
//


#if defined(__linux__) && defined(__GNUC__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <fenv.h>
#endif

#if (defined(__APPLE__) && (defined(__i386__) || defined(__x86_64__)))
#define OSX_SSE_FPE
#include <xmmintrin.h>
#endif

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/* for setuid / getuid */
#ifdef __sgi
#include <sys/types.h>
#include <unistd.h>
#endif

/* This little block needed for linking to Blender... */

#include "MEM_guardedalloc.h"

#ifdef WIN32
#include "BLI_winstuff.h"
#endif

#include "BLI_args.h"
#include "BLI_threads.h"
#include "BLI_scanfill.h" // for BLI_setErrorCallBack, TODO, move elsewhere
#include "BLI_utildefines.h"

#include "DNA_ID.h"
#include "DNA_scene_types.h"

#include "BLI_blenlib.h"

#define __LITTLE_ENDIAN__
#include "BKE_action.h"
#include "BKE_armature.h"
#include "BKE_utildefines.h"
#include "BKE_blender.h"
#include "BKE_context.h"
#include "BKE_depsgraph.h" // for DAG_on_visible_update
#include "BKE_font.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_material.h"
#include "BKE_packedFile.h"
#include "BKE_scene.h"
#include "BKE_node.h"
#include "BKE_report.h"
#include "BKE_sound.h"

extern "C" {
#include "IMB_imbuf.h"	// for IMB_init
#include "IMB_imbuf_types.h"
}

#ifdef WITH_PYTHON
#include "BPY_extern.h"
#endif

#include "RE_pipeline.h"

//XXX #include "playanim_ext.h"
#include "ED_datafiles.h"

#include "WM_api.h"

#include "RNA_define.h"

#include "GPU_draw.h"
#include "GPU_extensions.h"

#ifdef WITH_BUILDINFO_HEADER
#define BUILD_DATE
#endif

/* for passing information between creator and gameengine */
#ifdef WITH_GAMEENGINE
#include "BL_System.h"
#else /* dummy */
#define SYS_SystemHandle int
#endif

#include <signal.h>

#ifdef __FreeBSD__
# include <sys/types.h>
# include <floatingpoint.h>
# include <sys/rtprio.h>
#endif

#ifdef WITH_BINRELOC
#include "binreloc.h"
#endif

#include <algorithm>
#include <set>
#include <string>
#include <sstream>
#include "BLO_readfile.h"
#include "DNA_armature_types.h"
#include "DNA_camera_types.h"
#include "DNA_object_types.h"
#include "DNA_material_types.h"
#include "DNA_mesh_types.h"
#include "DNA_meshdata_types.h"
#include "DNA_scene_types.h"
#include "DNA_key_types.h"
#include "DNA_packedFile_types.h"
#include "DNA_modifier_types.h"
#include "BKE_customdata.h"
#include "BKE_object.h"
#include "BKE_deform.h"
#include "BKE_key.h"
#include "BLI_math.h"
#include "shared/platform.h"
#include "math/vector.h"
#include "math/transform.h"
#include "math/util.h"
#include "l3m/l3m.h"
#include "l3m/components/components.h"
#include "renderer/mesh.h"
#include "renderer/vertex_weight.h"

extern "C" {
  #include "BKE_DerivedMesh.h"
}

extern void startup_blender (int argc, const char** argv);

static std::map<std::string, l3m::Material*> gMaterials;

/**
Translation map.
Used to translate every COLLADA id to a valid id, no matter what "wrong" letters may be
included. Look at the IDREF XSD declaration for more.
Follows strictly the COLLADA XSD declaration which explicitly allows non-english chars,
like special chars (e.g. micro sign), umlauts and so on.
The COLLADA spec also allows additional chars for member access ('.'), these
must obviously be removed too, otherwise they would be heavily misinterpreted.
*/
const unsigned char translate_start_name_map[256] = {
95,  95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
65,  66,  67,  68,  69,  70,  71,  72,
73,  74,  75,  76,  77,  78,  79,  80,
81,  82,  83,  84,  85,  86,  87,  88,
89,  90,  95,  95,  95,  95,  95,  95,
97,  98,  99,  100,  101,  102,  103,  104,
105,  106,  107,  108,  109,  110,  111,  112,
113,  114,  115,  116,  117,  118,  119,  120,
121,  122,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  192,
193,  194,  195,  196,  197,  198,  199,  200,
201,  202,  203,  204,  205,  206,  207,  208,
209,  210,  211,  212,  213,  214,  95,  216,
217,  218,  219,  220,  221,  222,  223,  224,
225,  226,  227,  228,  229,  230,  231,  232,
233,  234,  235,  236,  237,  238,  239,  240,
241,  242,  243,  244,  245,  246,  95,  248,
249,  250,  251,  252,  253,  254,  255};

const unsigned char translate_name_map[256] = {
95,  95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  45,  95,  95,  48,
49,  50,  51,  52,  53,  54,  55,  56,
57,  95,  95,  95,  95,  95,  95,  95,
65,  66,  67,  68,  69,  70,  71,  72,
73,  74,  75,  76,  77,  78,  79,  80,
81,  82,  83,  84,  85,  86,  87,  88,
89,  90,  95,  95,  95,  95,  95,  95,
97,  98,  99,  100,  101,  102,  103,  104,
105,  106,  107,  108,  109,  110,  111,  112,
113,  114,  115,  116,  117,  118,  119,  120,
121,  122,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  95,  95,
95,  95,  95,  95,  95,  95,  183,  95,
95,  95,  95,  95,  95,  95,  95,  192,
193,  194,  195,  196,  197,  198,  199,  200,
201,  202,  203,  204,  205,  206,  207,  208,
209,  210,  211,  212,  213,  214,  95,  216,
217,  218,  219,  220,  221,  222,  223,  224,
225,  226,  227,  228,  229,  230,  231,  232,
233,  234,  235,  236,  237,  238,  239,  240,
241,  242,  243,  244,  245,  246,  95,  248,
249,  250,  251,  252,  253,  254,  255};

typedef std::map< std::string, std::vector<std::string> > map_string_list;
map_string_list global_id_map;

void clear_global_id_map()
{
	global_id_map.clear();
}

/** Look at documentation of translate_map */
std::string translate_id(const std::string &id)
{
	if (id.size() == 0)
	{ return id; }
	std::string id_translated = id;
	id_translated[0] = translate_start_name_map[(unsigned int)id_translated[0]];
	for (unsigned int i=1; i < id_translated.size(); i++)
	{
		id_translated[i] = translate_name_map[(unsigned int)id_translated[i]];
	}
	// It's so much workload now, the if() should speed up things.
	if (id_translated != id)
	{
		// Search duplicates
		map_string_list::iterator iter = global_id_map.find(id_translated);
		if (iter != global_id_map.end())
		{
			unsigned int i = 0;
			bool found = false;
			for (i=0; i < iter->second.size(); i++)
			{
				if (id == iter->second[i])
				{ 
					found = true;
					break;
				}
			}
			bool convert = false;
			if (found)
			{
			  if (i > 0)
			  { convert = true; }
			}
			else
			{ 
				convert = true;
				global_id_map[id_translated].push_back(id);
			}
			if (convert)
			{
				std::stringstream out;
				out << ++i;
				id_translated += out.str();
			}
		}
		else { global_id_map[id_translated].push_back(id); }
	}
	return id_translated;
}

std::string id_name(void *id)
{
	return ((ID*)id)->name + 2;
}

std::string get_geometry_id(Object *ob)
{
	return translate_id(id_name(ob->data)) + "";
}

std::string get_light_id(Object *ob)
{
	return translate_id(id_name(ob)) + "";
}

std::string get_joint_id(Bone *bone, Object *ob_arm)
{
	return translate_id(/*id_name(ob_arm) + "_" +*/ bone->name);
}

std::string get_camera_id(Object *ob)
{
	return translate_id(id_name(ob)) + "";
}

std::string get_material_id(Material *mat)
{
	return translate_id(id_name(mat)) + "";
}

std::string get_armature_id(Object* ob_arm, Object* ob)
{
    return translate_id(id_name(ob_arm));
}

static Object* get_assigned_armature(Object *ob)
{
	Object *ob_arm = NULL;

	if (ob->parent && ob->partype == PARSKEL && ob->parent->type == OB_ARMATURE) {
		ob_arm = ob->parent;
	}
	else {
		ModifierData *mod = (ModifierData*)ob->modifiers.first;
		while (mod) {
			if (mod->type == eModifierType_Armature) {
				ob_arm = ((ArmatureModifierData*)mod)->object;
			}

			mod = mod->next;
		}
	}

	return ob_arm;
}

static bool is_skinned_mesh(Object *ob)
{
	return get_assigned_armature(ob) != NULL;
}

static Matrix get_node_matrix_ob(Object* ob)
{
    Transform transform;

    transform.translation() = Vector3 ( ob->obmat[3][0], ob->obmat[3][1], ob->obmat[3][2] );
    for ( u8 i = 0; i < 3; ++i )
        for ( u8 j = 0; j < 3; ++j )
            transform.orientation().vector()[i*3+j] = ob->obmat[i][j];

    return Transform2Matrix(transform);
}

static Transform get_node_transform_ob(Object *ob, Matrix3* scaling)
{
    Transform transform;

    transform.translation() = Vector3 ( ob->obmat[3][0], ob->obmat[3][1], ob->obmat[3][2] );
    Matrix3 basis;
    for ( u8 i = 0; i < 3; ++i )
        for ( u8 j = 0; j < 3; ++j )
            basis.vector()[i*3+j] = ob->obmat[i][j];
    Matrix3 Q, R;
    Matrix3::QRDecompose ( basis, &Q, &R );
    transform.orientation() = Q;
    *scaling = R;

    return transform;
}

Bone* get_bone_from_defgroup(Object *ob_arm, bDeformGroup* def)
{
	bPoseChannel *pchan = get_pose_channel(ob_arm->pose, def->name);
	return pchan ? pchan->bone : NULL;
}

bool is_bone_defgroup(Object *ob_arm, bDeformGroup* def)
{
	return get_bone_from_defgroup(ob_arm, def) != NULL;
}



static bool findBoneIndex ( l3m::Pose* pose, const std::string& boneName, u32* ret )
{
    for ( u32 i = 0; i < pose->pose().numJoints(); ++i )
    {
        if ( pose->pose().jointNames()[i] == boneName )
        {
            *ret = i;
            return true;
        }
    }
    return false;
}




static void ImportVertex ( Renderer::Vertex* to, MVert* from )
{
    to->pos() = from->co;
    to->norm() = Vector3 ( from->no[0] / 32767.0f, from->no[1] / 32767.0f, from->no[2] / 32767.0f );
}

static bool ImportMesh ( Renderer::Geometry* g, const std::string& name, u32 mat_index, Object* ob, l3m::Model* model )
{
    // Get the actual face count
    Mesh* me = (Mesh *)ob->data;
    u32 totface = me->totface;
    MFace *faces = me->mface;
    u32 actualFaceCount = 0;
    
    for ( u32 i = 0; i < totface; ++i )
    {
        MFace* face = &faces[i];
        if ( face->mat_nr == mat_index )
        {
            if ( face->v4 == 0 )
                ++actualFaceCount;
            else
                actualFaceCount += 2;
        }
    }
    
    // Make all the indices
    u32* indices = ( u32* )sgMalloc ( sizeof(u32) * actualFaceCount * 3 );
    u32 indexIdx = 0;
    u32 curIndex = 0;
    for ( u32 i = 0; i < totface; ++i )
    {
        MFace* face = &faces[i];
        if ( face->mat_nr == mat_index )
        {
            indices[indexIdx++] = curIndex;
            indices[indexIdx++] = curIndex+1;
            indices[indexIdx++] = curIndex+2;
        }
        curIndex += 3;

        if ( face->v4 != 0 )
        {
            if ( face->mat_nr == mat_index )
            {
                indices[indexIdx++] = curIndex;
                indices[indexIdx++] = curIndex+1;
                indices[indexIdx++] = curIndex+2;
            }
            curIndex += 3;
        }
    }

    
    Renderer::Mesh* mesh = sgNew Renderer::Mesh ();
    mesh->set ( indices, actualFaceCount * 3, Renderer::Mesh::TRIANGLES );
    mesh->name() = name;
    // Import the material
    if ( me->mat != 0 && me->mat[mat_index] != 0 )
    {
        ::Material* ma = me->mat[mat_index];
        std::string material_id = get_material_id(ma);
        std::map<std::string, l3m::Material*>::iterator iter = gMaterials.find(material_id);
        if ( iter != gMaterials.end() )
        {
            mesh->material() = &iter->second->material();
        }
    }
    g->loadMesh( mesh );
    
    return true;
}

static bool compareVertexWeights ( const Renderer::VertexWeight& a, const Renderer::VertexWeight& b )
{
    return a.weight > b.weight;
}

static bool ImportVertexWeight ( u32 vertexIdxx, Renderer::VertexWeightSOA* vertexWeight, MDeformVert* dv, std::map<i32, i32>& defToJoint )
{
    std::vector<Renderer::VertexWeight> vecWeights;

    for ( u32 i = 0; i < dv->totweight; ++i )
    {
        Renderer::VertexWeight w;
        w.joint = defToJoint[(i32)dv->dw[i].def_nr];
        if ( defToJoint[(i32)dv->dw[i].def_nr] == -1 )
        {
            fprintf ( stderr, "Error: Importing an invalid joint\n" );
            return false;
        }
        w.weight = dv->dw[i].weight;
        vecWeights.push_back ( w );
    }
    
    // Fill all the unused weights
    Renderer::VertexWeight nullWeight;
    nullWeight.weight = 0.0f;
    nullWeight.joint = 0;
    std::sort ( vecWeights.begin(), vecWeights.end(), compareVertexWeights );
    for ( u32 i = vecWeights.size(); i < Renderer::VertexWeight::MAX_ASSOCIATIONS; ++i )
        vecWeights.push_back ( nullWeight );
    
    // Take the top "MAX_ASSOCIATIONS" weights
    float weightsum = 0.0f;
    for ( u32 i = 0; i < Renderer::VertexWeight::MAX_ASSOCIATIONS; ++i )
    {
        vertexWeight->joint[i] = vecWeights[i].joint;
        vertexWeight->weight[i] = vecWeights[i].weight;
        weightsum += vecWeights[i].weight;
    }

    
    // Normalize the weights
    if ( weightsum != 0.0f )
    {
        for ( u32 i = 0; i < Renderer::VertexWeight::MAX_ASSOCIATIONS; ++i )
        {
            vertexWeight->weight[i] /= weightsum;
        }
    }
    
    return true;
}

static bool ImportShapeKeys( l3m::Geometry* g, Object* ob, l3m::Model* model )
{
  Mesh* me = (Mesh *) ob->data;
  Key* key = me->key;

  if ( key && key->type == KEY_RELATIVE && key->refkey )
  {
    int numVert = me->totvert;
    int numKeys = key->totkey;
    int numKeysUsed = 0;
    
    Vector3* refVertexPos = (Vector3*) key->refkey->data;

    // abort if the reference key has different number of vertex than the mesh.
    // or if the number of keys is <= 1 (only the base key is defined)
    if ( key->refkey->totelem != numVert || numKeys <= 1) {
      return false;
    }
    
    // Create a morph component in the model
    l3m::Morph* modelMorph = (l3m::Morph *)model->createComponent( "morph" );
    if (modelMorph == 0) {
      fprintf(stderr, "error: could not create morph component.\n");
      return false;
    }
    Renderer::Morph& morph = modelMorph->morph();
    
    // use the geometry name as name for the morph object
    morph.name() = get_geometry_id(ob);
    
    // storage for the vertices of each shape/key
    Renderer::Vertex* shapes = (Renderer::Vertex*) sgMalloc( sizeof(Renderer::Vertex) * numVert * numKeys );

    int keyNum = 0;
    for ( KeyBlock* kb = (KeyBlock*) key->block.first; kb; kb = kb->next, keyNum++ )
    {
      if (numKeysUsed > numKeys) {
        fprintf(stderr, "Warning: model number of keys and actual number of keys do not match\n");
        break;
      }
      
      Vector3* vertexPos = (Vector3*) kb->data;
      Renderer::Vertex* curShapeVertex = &shapes[numKeysUsed * numVert];
      
#ifdef DEBUG_SHAPE_KEYS
      fprintf(stderr, "%d: '%s'\n", keyNum, kb->name);
      fprintf(stderr, "%16s: %5.3f\n", "value", kb->curval);
      fprintf(stderr, "%16s: %.3f - %.3f\n", "range", kb->slidermin, kb->slidermax);
      fprintf(stderr, "%16s: %d\n", "type", kb->type);
      fprintf(stderr, "%16s: %d\n", "relative", kb->relative);
      fprintf(stderr, "%16s: %d\n", "num. elem", numVert);
#endif

      // blender use cardinal keys for meshes. There are two other types
      // (LINEAR and BSPLINE) that i don't know when it use them.
      if ( kb->type != KEY_CARDINAL ) {
        fprintf(stderr, "warning: ignoring shape key of unsupported type!\n");
        continue;
      }
      
      // keys with a vertex group in blender are defined for every vertex in
      // the mesh but blender ignore the values outside of the vertex group.
      // since using vertex groups has no benefits we ignore them for the 
      // sake of simplicity.
      if ( kb->vgroup[0] != 0 ) {
        fprintf(stderr, "warning: ignoring vertex group for shape key!\n");
      }

      // the key has different number of vertex than the mesh.
      // as far as i can tell this should never happens
      if ( kb->totelem != numVert ) {
        fprintf(stderr, "warning: ignoring incomplete shape key!\n");
        continue;
      }

      // the refkey or 'Basis' is ignored because it matches the mesh
      if ( kb == key->refkey ) {
        continue;
      }

      // apply the limit of shapes of the engine
      if (numKeysUsed >= morph.MAX_SHAPES) {
        fprintf(stderr, "warning: limit of shapes reached, %d keys ignored!\n",
          (numKeys - keyNum));
        break;
      }
      
      // Add the shape to the morph object
      {
        // store the shape name in the morph object
        morph.shapeNames()[numKeysUsed] = kb->name;

        // make the shape active if its weight is big enough
        if (fabs(kb->curval) > 0.01) {
          morph.addActiveShape(numKeysUsed, kb->curval);
        }
      }
      
      // create absolute vertex positions (if they are relative to the refkey)
      if ( kb->relative && kb != key->refkey) {
        for ( int i=0; i<numVert; i++ ) {
          vertexPos[i] += refVertexPos[i];
        }
        kb->relative = 0;
      }      
      
      // create a derived mesh to autogenerate normals
      DerivedMesh * dme = (DerivedMesh *) 
        mesh_create_derived (me, ob, (float (*)[3]) kb->data);
      MVert * vertexArray = (MVert *) DM_get_vert_data_layer(dme, CD_MVERT);

      // loop for all the vertex extracting the relative vertex offsets
      for ( int i = 0; i < numVert; i++ )
      {
        MVert * vert;
        const float normScale = 1/32767.0f;
        
        // position and normal in the current shape key
        vert = &vertexArray[i];
        Vector3 pos(vert->co[0], vert->co[1], vert->co[2]);
        Vector3 norm(vert->no[0], vert->no[1], vert->no[2]);
        norm *= normScale;

        // actual position and normal of the vertices
        vert = &me->mvert[i];
        Vector3 meshPos(vert->co[0], vert->co[1], vert->co[2]);
        Vector3 meshNorm(vert->no[0], vert->no[1], vert->no[2]);
        meshNorm *= normScale;

        // the relative value is stored in the current shape
        curShapeVertex[i].pos() = pos - meshPos;
        curShapeVertex[i].norm() = norm - meshNorm;
        
#ifdef DEBUG_SHAPE_KEYS_EXTRA            
        // display for debugging
        pos = curShapeVertex[i].pos();
        norm = curShapeVertex[i].norm();
        if ( pos.length() > 0.001 ) {
          fprintf(stderr, 
            "\t%3d [% 5.3f, % 5.3f, % 5.3f] [% 5.3f, % 5.3f, % 5.3f]\n", 
            i, pos.x(), pos.y(), pos.z(), norm.x(), norm.y(), norm.z());
        }
#endif
      }
            
      // release derived mesh
      DM_release(dme);        
      
      // we have just used one more key
      numKeysUsed++;
    }
    
    // Calculate the number of shapes actives in the morph object
    morph.numShapes() = numKeysUsed;    
    morph.checkAndFix();
    
    #ifdef DEBUG_SHAPE_KEYS
      fprintf(stderr, "Morph object created:\n");
      fprintf(stderr, "  numShapes:       %d\n", morph.numShapes());
      for (int i=0; i<morph.numShapes(); i++) {
        fprintf(stderr, "    %2d name:    %s\n", 
          i, morph.shapeNames()[i].c_str());
      }
      fprintf(stderr, "  numActiveShapes: %d\n", morph.numActiveShapes());
      for (int i=0; i<morph.numActiveShapes(); i++) {
        fprintf(stderr, "    %2d shape:   %d, weight: %.3f\n", 
          i, morph.activeShapes()[i], morph.activeWeights()[i]);
      }
    #endif

    // Calculate the indexes of the vertices in 'face order'
    if (numKeysUsed > 0) {
      int actualVertexCount = g->geometry().numVertices();
      u32* vertexIdx = (u32*) sgMalloc(sizeof (u32) * actualVertexCount);     
      
      u32 vertexNum = 0;
      for ( u32 i = 0; i < me->totface; i++ )
      {
        MFace* face = &me->mface[i];
        
        if (vertexNum+3 > actualVertexCount) {
          fprintf(stderr, "Warning: the geometry doesn't include all the vertices\n");
          break;
        }
        
        vertexIdx[vertexNum+0] = face->v1;
        vertexIdx[vertexNum+1] = face->v2;
        vertexIdx[vertexNum+2] = face->v3;
        vertexNum += 3;
        
        if ( face->v4 != 0 )
        {
          if (vertexNum+3 > actualVertexCount) {
            fprintf(stderr, "Warning: the geometry doesn't include all the vertices\n");
            break;
          }
          
          vertexIdx[vertexNum+0] = face->v1;
          vertexIdx[vertexNum+1] = face->v3;
          vertexIdx[vertexNum+2] = face->v4;
          vertexNum += 3;
        }
      }
    
      // Create the vertex layer
      if (!g->geometry().createVertexLayer<Renderer::Vertex>("shapes", numKeysUsed, 0)) {
        fprintf(stderr, "Error: could not create the vertex layer 'shapes'!\n");
      }
      
      // Copy the vertex data to the vertex layer
      for ( u32 i=0; i<numKeysUsed; i++ )
      {
        // layer is the current level of the vertex layer and has actualVertexCount vertices
        Renderer::Vertex* layer = g->geometry().getVertexLayer<Renderer::Vertex>("shapes", i);
        
        // curShapeVertex is the vertex data of the current shape and has numVert unique vertices
        Renderer::Vertex* curShapeVertex = &shapes[i * numVert];
        
        if (!layer) {
          fprintf(stderr, "Warning: could not get shape %d\n", i);
          continue;
        }
        
        // do the copy
        for ( u32 n = 0; n < actualVertexCount; n++ )
        {
          u32 idx = vertexIdx[n];
          if (idx > numVert) {
            fprintf(stderr, "Warning: vertex overflow requesting index %d\n", idx);
            idx = 0;
          }          
          layer[n] = curShapeVertex[idx];
        }
      }
      
#ifdef DEBUG_SHAPE_KEYS
      fprintf(stderr, "A layer with %d levels and %d vertices has been created.\n"
        "The original mesh had %d vertex and %d shapes (%d usable shapes).\n",
        g->geometry().getVertexLayerLevelCount("shapes"),
        actualVertexCount, numVert, numKeys, numKeysUsed);
#endif
      
      sgFree(vertexIdx);
    }
    
    sgFree(shapes);        
    return (numKeysUsed > 0);
  }
  return false;
}

static bool ImportGeometry ( l3m::Geometry* g, Object* ob, l3m::Model* model )
{
    Mesh* me = (Mesh *)ob->data;
    MFace *faces = me->mface;
    MVert* verts = me->mvert;
    u32 totcol = me->totcol;
    u32 totface = me->totface;
    
    // Count the total number of vertices
    u32 actualVertexCount = 0;
    for ( u32 i = 0; i < totface; ++i )
    {
        MFace* face = &faces[i];
        if ( face->v4 != 0 )
            actualVertexCount += 6;
        else
            actualVertexCount += 3;
    }
    
    // Import the geometry vertices
    Renderer::Vertex* vertexArray = (Renderer::Vertex *)sgMalloc ( sizeof(Renderer::Vertex) * actualVertexCount );
    memset ( vertexArray, 0, sizeof(Renderer::Vertex) * actualVertexCount );
    u32 curVertex = 0;
    
    for ( u32 i = 0; i < totface; ++i )
    {
        MFace* face = &faces[i];
        ImportVertex ( &vertexArray [ curVertex++ ], &verts[ face->v1 ] );
        ImportVertex ( &vertexArray [ curVertex++ ], &verts[ face->v2 ] );
        ImportVertex ( &vertexArray [ curVertex++ ], &verts[ face->v3 ] );

        if ( face->v4 != 0 )
        {
            ImportVertex ( &vertexArray [ curVertex++ ], &verts[ face->v1 ] );
            ImportVertex ( &vertexArray [ curVertex++ ], &verts[ face->v3 ] );
            ImportVertex ( &vertexArray [ curVertex++ ], &verts[ face->v4 ] );
        }
    }
    g->geometry().set( vertexArray, actualVertexCount );
    
    // Import the geometry UV texture coordinates
    bool has_uvs = (bool)CustomData_has_layer(&me->fdata, CD_MTFACE);
    if ( has_uvs )
    {
        curVertex = 0;
        int layerCount = CustomData_number_of_layers(&me->fdata, CD_MTFACE);
        Vector2* uvData = (Vector2*)sgMalloc ( sizeof(Vector2) * layerCount * actualVertexCount );
        
        for ( u32 l = 0; l < layerCount; ++l )
        {
            MTFace *tface = (MTFace*)CustomData_get_layer_n(&me->fdata, CD_MTFACE, l);
            
            for ( u32 i = 0; i < totface; ++i )
            {
                MFace* face = &faces[i];
                
                uvData [ curVertex++ ] = Vector2 ( tface[i].uv[0] );
                uvData [ curVertex++ ] = Vector2 ( tface[i].uv[1] );
                uvData [ curVertex++ ] = Vector2 ( tface[i].uv[2] );

                if ( face->v4 != 0 )
                {
                    uvData [ curVertex++ ] = Vector2 ( tface[i].uv[0] );
                    uvData [ curVertex++ ] = Vector2 ( tface[i].uv[2] );
                    uvData [ curVertex++ ] = Vector2 ( tface[i].uv[3] );
                }
            }
        }
        
        g->geometry().createVertexLayer( "uv", layerCount, uvData, sizeof(Vector2) );
        sgFree ( uvData );
    }
    
    // Import the vertex colors
    bool has_color = (bool)CustomData_has_layer(&me->fdata, CD_MCOL);
    if ( has_color )
    {
        Color* colorData = (Color *)sgMalloc( sizeof(Color) * actualVertexCount );
        int index = CustomData_get_active_layer_index(&me->fdata, CD_MCOL);
        curVertex = 0;

        MCol *mcol = (MCol*)me->fdata.layers[index].data;
        
        for ( u32 i = 0; i < totface; ++i, mcol += 4 )
        {
            colorData [ curVertex++ ] = Color ( mcol[0].r, mcol[0].g, mcol[0].b, mcol[0].a );
            colorData [ curVertex++ ] = Color ( mcol[1].r, mcol[1].g, mcol[1].b, mcol[1].a );
            colorData [ curVertex++ ] = Color ( mcol[2].r, mcol[2].g, mcol[2].b, mcol[2].a );
            
            if ( faces[i].v4 != 0 )
            {
                colorData [ curVertex++ ] = Color ( mcol[0].r, mcol[0].g, mcol[0].b, mcol[0].a );
                colorData [ curVertex++ ] = Color ( mcol[2].r, mcol[2].g, mcol[2].b, mcol[2].a );
                colorData [ curVertex++ ] = Color ( mcol[3].r, mcol[3].g, mcol[3].b, mcol[3].a );
            }
        }
        
        g->geometry().createVertexLayer("color", 1, colorData, sizeof(Color) );
        sgFree ( colorData );
    }
    
    // Load the skinning info
    Object *ob_arm = get_assigned_armature(ob);
    if ( ob_arm != 0 )
    {
        std::string id = get_armature_id(ob_arm, ob);
        l3m::Pose* pose = l3m::Util::findPose ( model, id );
        if ( pose != 0 )
        {
            g->poseUrl() = pose->pose().name ();
            
            bArmature* arm = (bArmature*)ob_arm->data;
            
            // Find the def index -> joint index associations
            std::map<i32, i32> defToJoint;
            {
                bDeformGroup *def;
                ListBase* defbase = &ob->defbase;
                i32 i;
                for (def = (bDeformGroup*)defbase->first, i = 0; def; def = def->next, i++)
                {
                    if ( is_bone_defgroup(ob_arm, def))
                    {
                        u32 idx;
                        if ( findBoneIndex(pose, def->name, &idx) )
                        {
                            defToJoint[i] = idx;
                            continue;
                        }
                    }
                    defToJoint[i] = -1;
                }
            }
            
            
            // Load the vertex weights
            Renderer::VertexWeightSOA* weights = (Renderer::VertexWeightSOA*)g->geometry().createVertexLayer( "weights", 1, 0, sizeof(Renderer::VertexWeightSOA) );
            curVertex = 0;
            
            for ( u32 i = 0; i < totface; ++i )
            {
                MFace* face = &faces[i];
                if ( !ImportVertexWeight ( curVertex, &weights [ curVertex ], &me->dvert[ face->v1 ], defToJoint ) ||
                     !ImportVertexWeight ( curVertex+1, &weights [ curVertex+1 ], &me->dvert[ face->v2 ], defToJoint ) ||
                     !ImportVertexWeight ( curVertex+2, &weights [ curVertex+2 ], &me->dvert[ face->v3 ], defToJoint ) )
                    return false;
                curVertex += 3;

                if ( face->v4 != 0 )
                {
                    if ( !ImportVertexWeight ( curVertex, &weights [ curVertex ], &me->dvert[ face->v1 ], defToJoint ) ||
                         !ImportVertexWeight ( curVertex+1, &weights [ curVertex + 1 ], &me->dvert[ face->v3 ], defToJoint ) ||
                         !ImportVertexWeight ( curVertex+2, &weights [ curVertex + 2 ], &me->dvert[ face->v4 ], defToJoint ) )
                        return false;
                    curVertex += 3;
                }
            }
        }
    }
    
    // Load the shape keys
    ImportShapeKeys(g, ob, model);
    
    
    // Load every mesh in this geometry
    if ( !totcol )
        return ImportMesh ( &g->geometry(), g->geometry().name(), 0, ob, model );
    else
    {
        for ( u32 i = 0; i < totcol; ++i )
        {
            char name [ 512 ];
            snprintf ( name, sizeof(name), "%s-%u", g->geometry().name().c_str(), i );
            if ( !ImportMesh ( &g->geometry(), name, i, ob, model ) )
                return false;
        }
        return true;
    }
}

void printVector(const char * pre, const Vector3 & v, const char * post = "")
{
  fprintf(stderr, "%s[ %5.3f %5.3f %5.3f ]%s", pre, v.x(), v.y(), v.z(), post);
}

static bool ImportSceneObject ( l3m::Model* model, Object* ob, ::Scene* sce, l3m::Scene* modelScene )
{
    switch ( ob->type )
    {
        case OB_MESH:
        {
            Mesh* me = (Mesh*)ob->data;
            l3m::Scene::Node& node = modelScene->createGeometryNode();
            node.url = get_geometry_id(ob);

            Matrix preTransform;
            Matrix normalMatrix;
            
            if ( is_skinned_mesh(ob) )
            {
                // Get the object transform, and the armature transform
                Matrix3 armScale;
                node.transform = get_node_transform_ob(get_assigned_armature(ob), &armScale);

                preTransform = Matrix(&ob->obmat[0][0]);
            }
            else
            {
                // Get the transform
                Matrix3 scale;
                node.transform = get_node_transform_ob(ob, &scale);
                preTransform = scale;
            }
            
            normalMatrix = MatrixForNormals(preTransform);
            
            // Apply the transform
            // Find the geometry associated to this url.
            for ( l3m::Model::ComponentVector::iterator miter = model->components().begin();
                miter != model->components().end();
                ++miter )
            {
                if ( (*miter)->type() == "geometry" && static_cast < l3m::Geometry* > ( *miter )->geometry().name() == node.url )
                {
                    Renderer::Geometry& g = static_cast<l3m::Geometry *>(*miter)->geometry();

                    // Prepare pre-transformation loops
                    u32 numVertices = g.numVertices();
                    u32 numShapes = g.getVertexLayerLevelCount("shapes");                    
                    Renderer::Vertex * oldVertex = (Renderer::Vertex*) sgMalloc(numVertices * sizeof(Renderer::Vertex));

                    // Apply the transformation to every vertex
                    Renderer::Vertex* v = g.vertices();
                    for ( u32 i = 0; i < numVertices; i++ )
                    {
                        oldVertex[i] = v[i];
                        
                        v[i].pos() *= preTransform;
                        v[i].norm() *= normalMatrix;
                        v[i].norm().normalize();
                    }
                    
                    // Apply the transformation to every shape
                    for ( u32 level = 0; level < numShapes; level++ )
                    {
                        Renderer::Vertex * shape = g.getVertexLayer<Renderer::Vertex>("shapes", level);
                        for ( u32 i = 0; i < numVertices; i++ )
                        {                          
                          // make the shape absolute for transformation
                          shape[i].pos() += oldVertex[i].pos();
                          shape[i].norm() += oldVertex[i].norm();
                          
                          shape[i].pos() *= preTransform;
                          shape[i].norm() *= normalMatrix;
                          shape[i].norm().normalize();
                          
                          // make the shape relative again
                          shape[i].pos() -= v[i].pos();
                          shape[i].norm() -= v[i].norm();
                        }
                    }
                    
                    // cleanup
                    sgFree(oldVertex);
                }
            }

            // Import the set of textures
            bool has_uvs = (bool)CustomData_has_layer(&me->fdata, CD_MTFACE);
            if ( has_uvs )
            {
                int layerCount = CustomData_number_of_layers(&me->fdata, CD_MTFACE);
                for ( u32 l = 0; l < layerCount; ++l )
                {
                    MTFace *tface = (MTFace*)CustomData_get_layer_n(&me->fdata, CD_MTFACE, l);
                    Image* image = tface->tpage;
                    if ( image != 0 )
                        node.textures.push_back(translate_id(id_name(image)));
                }
            }
            
            break;
        }
    }
    
    return true;
}

static bool ImportScene ( l3m::Model* model, ::Scene* sce, l3m::Scene* modelScene )
{
    Base *base= (Base*) sce->base.first;
    while(base) {
            Object *ob = base->object;

            switch(ob->type) {
                    case OB_MESH:
                    case OB_CAMERA:
                    case OB_LAMP:
                    case OB_ARMATURE:
                    case OB_EMPTY:
                            if ( ImportSceneObject(model, ob, sce, modelScene) == false )
                                return false;
                            break;
            }

            base= base->next;
    }
    
    // Import the scene camera
    if ( sce->camera != 0 )
        modelScene->cameraUrl() = get_camera_id ( sce->camera );
    
    // Import the scene rendering size
    modelScene->width() = sce->r.xsch;
    modelScene->height() = sce->r.ysch;
    
    return true;
}

static bool ImportImage ( ::Image* image, ::Scene* sce, const char* filename, l3m::Model* model )
{
    std::string name(id_name(image));
    name = translate_id(name);
                            
    // The image is packed
    if (image->packedfile)
    {
        int flag = IB_rect|IB_multilayer;
        if(image->flag & IMA_DO_PREMUL) flag |= IB_premul;
        ImBuf* ibuf = IMB_ibImageFromMemory((unsigned char*)image->packedfile->data, (size_t)image->packedfile->size, flag, "");
        if (ibuf)
        {
            Pixmap pix;
            pix.create( ibuf->x, ibuf->y, (Color *)ibuf->rect );
            l3m::Texture* tex = (l3m::Texture *)model->createComponent("texture");
            tex->id() = name;
            tex->pixmap() = pix;
        }
    }
    else
    {
        char rel[FILE_MAX];
        char abs[FILE_MAX];
        char dir[FILE_MAX];

        BLI_split_dirfile(filename, abs, dir, FILE_MAX, FILE_MAX);
        BKE_rebase_path(abs, sizeof(abs), rel, sizeof(rel), G.main->name, image->name, dir);


        Pixmap pix;
        if ( !pix.load(abs) )
        {
            fprintf ( stderr, "Warning: Cannot open the image: %s\n", abs );
            return true;
        }
        l3m::Texture* tex = (l3m::Texture *)model->createComponent("texture");
        tex->id() = name;
        tex->pixmap() = pix;
    }
    
    return true;
}

static bool ImportImages ( ::Scene* sce, const char* filename, l3m::Model* model )
{
    std::vector<std::string> mImages;

    // For each mesh...
    Base *base= (Base*) sce->base.first;
    while(base) {
            Object *ob = base->object;

            switch(ob->type)
            {
                case OB_MESH:
                {
                    Mesh* me = (Mesh *)ob->data;
                    
                    // Check for the material images
                    for ( u32 a = 0; a < ob->totcol; ++a )
                    {
                        if ( me->mat[a] && me->mat[a]->mtex[0] && me->mat[a]->mtex[0]->tex )
                        {
                            Image* ima = me->mat[a]->mtex[0]->tex->ima;
                            if ( ima )
                            {
                                std::string name(id_name(ima));
                                name = translate_id(name);
                                if (std::find(mImages.begin(), mImages.end(), name) == mImages.end())
                                {
                                    mImages.push_back(name);
                                    if ( !ImportImage(ima, sce, filename, model) )
                                        return false;
                                }
                            }
                        }
                    }

                    // For each mesh uv...
                    bool has_uvs = (bool)CustomData_has_layer(&me->fdata, CD_MTFACE);
                    if ( has_uvs )
                    {
                        int layerCount = CustomData_number_of_layers(&me->fdata, CD_MTFACE);
                        for ( u32 l = 0; l < layerCount; ++l )
                        {
                            MTFace *tface = (MTFace*)CustomData_get_layer_n(&me->fdata, CD_MTFACE, l);
                            Image* image = tface->tpage;
                            if ( image == 0 )
                                continue;
                            
                            std::string name(id_name(image));
                            name = translate_id(name);
                            if (std::find(mImages.begin(), mImages.end(), name) == mImages.end())
                            {
                                mImages.push_back(name);
                                if ( !ImportImage(image, sce, filename, model) )
                                    return false;
                            }
                        }
                    }
                    break;
                }
                default: break;
            }

            base= base->next;
    }
    
    return true;
}

static bool ImportMaterials ( ::Scene* sce, l3m::Model* model )
{
    std::vector < std::string > mMat;
    
    // For each mesh object...
    Base *base= (Base*) sce->base.first;
    while(base)
    {
        Object *ob = base->object;

        switch(ob->type)
        {
            case OB_MESH:
            {
                for ( u32 a = 0; a < ob->totcol; ++a )
                {
                    ::Material *ma = give_current_material(ob, a+1);
                    if (!ma)
                        continue;

                    std::string translated_id = get_material_id(ma);
                    if (std::find(mMat.begin(), mMat.end(), translated_id) == mMat.end())
                    {
                        l3m::Material* comp = (l3m::Material *)model->createComponent("material");
                        
                        Renderer::Material mat;
                        Color ambient ( ma->amb * 255.0f, ma->amb * 255.0f, ma->amb * 255.0f, 255.0f );
                        Color diffuse ( ma->r * ma->ref * 255.0f, ma->g * ma->ref * 255.0f, ma->b * ma->ref * 255.0f, 255.0f );
                        Color specular ( ma->specr * ma->spec * 255.0f, ma->specg * ma->spec * 255.0f, ma->specb * ma->spec * 255.0f, 255.0f );
                        Color emission ( ma->r * ma->emit * 255.0f, ma->g * ma->emit * 255.0f, ma->b * ma->emit * 255.0f, 255.0f );
                        float shininess = ma->har;
                        
                        mat.name() = translated_id;
                        mat.ambient() = ambient;
                        mat.diffuse() = diffuse;
                        mat.specular() = specular;
                        mat.emission() = emission;
                        mat.shininess() = shininess;
                        mat.isShadeless() = (ma->mode & MA_SHLESS) == MA_SHLESS;
                        mat.isTransparent() = (ma->mode & MA_TRANSP) && (ma->mode & MA_ZTRANSP);
                        
                        // Check for textures
                        // TODO: Multi-texturing
                        if ( ma->mtex[0] != 0 )
                        {
                            ::Tex* tex = ma->mtex[0]->tex;
                            if ( tex != 0 )
                            {
                                ::Image* ima = tex->ima;
                                if ( ima != 0 )
                                {
                                    std::string name(id_name(ima));
                                    name = translate_id(name);
                                    mat.texture() = name;
                                }
                            }
                        }
                        
                        comp->material() = mat;
                        
                        gMaterials[translated_id] = comp;
                        mMat.push_back(translated_id);
                    }
                }
            }
        }
        
        base= base->next;
    }
    
    return true;
}

static bool ImportCamera ( l3m::Camera* cam, ::Object* ob, ::Scene* sce )
{
    // Get the transform
    Transform transform;
    float orientation[9];
    
    // Blender uses Z as up direction, but we use Y as up direction.
    transform.translation() = Vector3 ( ob->obmat[3][0], ob->obmat[3][1], ob->obmat[3][2] );
    
    for ( u32 i = 0; i < 3; ++i )
        for ( u32 j = 0; j < 3; ++j )
            orientation[i*3+j] = ob->obmat[i][j];
    // WARNING: Here we rotate the camera because, for some reason, Blender cameras look forward by looking downwards.
    transform.orientation() = Matrix3(orientation) * RotationMatrix(EulerAngles(0.0f,deg2rad(-90.0f), 0.0f));
    cam->transform() = transform;
    
    // Get the camera type
    ::Camera* c = (::Camera *)ob->data;
    if (!c)
        return false;
    
    // Import the data according to the type
    switch ( c->type )
    {
        case CAM_PERSP:
            cam->type() = l3m::Camera::CAMERA_PERSPECTIVE;
            cam->perspectiveData().aspect = (float)(sce->r.xsch)/(float)(sce->r.ysch);
            cam->perspectiveData().fov = focallength_to_fov(c->lens, c->sensor_x);
            cam->perspectiveData().near = c->clipsta;
            cam->perspectiveData().far = c->clipend;
            break;
        case CAM_ORTHO:
            cam->type() = l3m::Camera::CAMERA_ORTHOGRAPHIC;
            cam->orthographicData().left = -c->ortho_scale * (float)(sce->r.xsch)/(float)(sce->r.ysch);
            cam->orthographicData().right = -cam->orthographicData().left;
            cam->orthographicData().top = c->ortho_scale;
            cam->orthographicData().bottom = -c->ortho_scale;
            cam->orthographicData().near = c->clipsta;
            cam->orthographicData().far = c->clipend;
            break;
        default:
            cam->type() = l3m::Camera::CAMERA_UNKNOWN;
            break;
    }

    
    return true;
}

static bool ImportPose ( ::Scene* sce, l3m::Model* model, const std::string& pose_id, Object* ob, Object* ob_arm )
{
    l3m::Pose* modelPose = (l3m::Pose *)model->createComponent( "pose" );
    modelPose->pose().name() = pose_id;
    Renderer::Pose& pose = modelPose->pose();
    bArmature* arm = (bArmature*)ob_arm->data;
    
    // Iterate by all the bones to gather the bone pose positions
    std::vector<Bone *> vecBones;
    for (Bone *bone = (Bone*)arm->bonebase.first; bone; bone = bone->next)
    {
        // start from root bones
        if (!bone->parent)
            vecBones.push_back ( bone );
    }
    
    u32 numJoints = 0;
    while ( vecBones.size() >  0 )
    {
        Bone* bone = vecBones.back ();
        vecBones.pop_back ();
        
        // Push its children
        for (Bone *child = (Bone*)bone->childbase.first; child; child = child->next)
            vecBones.push_back ( child );
        
        pose.jointNames()[numJoints] = bone->name;
        
        // Get the bone transform
        bPoseChannel *pchan = get_pose_channel(ob_arm->pose, bone->name);
        if ( pchan != 0 )
        {
            Matrix chanMat ( &pchan->chan_mat[0][0] );
            pose.transforms()[numJoints] = Matrix2QTransform(chanMat);
        }
        else
        {
            pose.transforms()[numJoints] = Matrix2QTransform(IdentityMatrix ());
        }
        
        ++numJoints;
    }
    pose.numJoints() = numJoints;

    return true;
}

static bool ImportPoses ( ::Scene* sce, l3m::Model* model )
{
    std::vector < std::string > mPoses;
    
    // For each mesh object...
    Base *base= (Base*) sce->base.first;
    while(base)
    {
        Object *ob = base->object;

        switch(ob->type)
        {
            case OB_MESH:
            {
                Object *ob_arm = get_assigned_armature(ob);
                if ( ob_arm != 0 )
                {
                    std::string id = get_armature_id(ob_arm, ob);

                    if (std::find(mPoses.begin(), mPoses.end(), id) == mPoses.end())
                    {
                        if ( ! ImportPose ( sce, model, id, ob, ob_arm ) )
                            return false;
                        mPoses.push_back ( id );
                    }
                }
            }
        }
        
        base = base->next;
    }
    
    return true;
}

static bool import_blender ( ::Scene* sce, const char* filename, l3m::Model* model )
{
    // Import the materials
    if ( !ImportMaterials ( sce, model ) )
    {
        fprintf ( stderr, "Error importing the model materials\n" );
        return false;
    }
    
    // Import the poses
    if ( !ImportPoses ( sce, model ) )
    {
        fprintf ( stderr, "Error importing the model poses\n" );
        return false;
    }
    
    // Import all geometries
    std::set<std::string> exportedGeometry;
    for ( Base* base = (Base *)sce->base.first; base != 0; base = base->next )
    {
        Object* ob = base->object;
        if ( ob->type == OB_MESH && ob->data )
        {
            std::string geom_id = get_geometry_id(ob);
            if (exportedGeometry.find(geom_id) == exportedGeometry.end())
            {
                exportedGeometry.insert(geom_id);
                l3m::Geometry* g = (l3m::Geometry *)model->createComponent("geometry");
                g->geometry().name() = geom_id;

                if ( ! ImportGeometry ( g, ob, model ) )
                {
                    fprintf ( stderr, "Error exporting a geometry\n" );
                    return false;
                }
            }
        }
    }
    
    // Import the scene cameras
    std::set<std::string> exportedCameras;
    for ( Base* base = (Base *)sce->base.first; base != 0; base = base->next )
    {
        Object* ob = base->object;
        if ( ob->type == OB_CAMERA && ob->data )
        {
            std::string cam_id = get_camera_id(ob);
            if (exportedCameras.find(cam_id) == exportedCameras.end())
            {
                exportedCameras.insert(cam_id);
                l3m::Camera* cam = (l3m::Camera *)model->createComponent("camera");
                cam->name() = cam_id;

                if ( ! ImportCamera ( cam, ob, sce ) )
                {
                    fprintf ( stderr, "Error importing a camera\n" );
                    return false;
                }
            }
        }
    }
    
    // Import the visual scene
    l3m::Scene* modelScene = (l3m::Scene *)model->createComponent("scene");
    if ( !ImportScene ( model, sce, modelScene ) )
    {
        fprintf ( stderr, "Error importing the visual scene\n" );
        return false;
    }
    
    // Import the images
    if ( !ImportImages ( sce, filename, model ) )
    {
        fprintf ( stderr, "Error importing the scene images\n" );
        return false;
    }

    return true;
}

bool import_blender ( int argc, const char** argv, const char* file, l3m::Model* model )
{
    startup_blender(argc, argv);
    BlendFileData* data = BLO_read_from_file(file, NULL);
    if ( data == 0 )
    {
        fprintf ( stderr, "Unable to read the blender file.\n" );
        return false;
    }
    
    return import_blender ( data->curscene, file, model );
}

#if 0
bool import_blender ( int argc, const char** argv, std::istream& is, l3m::Model* model )
{
    startup_blender(argc, argv);

    u32 memSize = 4096;
    char* mem = sgNew char [ memSize ];
    
    char temp [ 512 ];
    u32 totalSize = 0;
    u32 currentSize;
    
    while ( !is.eof () )
    {
        is.read ( temp, sizeof(temp) );
        currentSize = is.gcount();

        if ( currentSize > 0 )
        {
            if ( totalSize + currentSize > memSize )
            {
                memSize *= 2;
                char* newMem = sgNew char [ memSize ];
                memcpy ( newMem, mem, totalSize );
                sgDelete [] mem;
                mem = newMem;
            }
            memcpy ( &mem[totalSize], temp, currentSize );
            totalSize += currentSize;
        }
    }
    
    BlendFileData* data = BLO_read_from_memory(mem, totalSize, NULL);
    if ( data == 0 )
    {
        sgDelete [] mem;
        return false;
    }
    
    bool ret = import_blender ( data->curscene, argv[1], model );
    sgDelete [] mem;
    return ret;
}
#endif