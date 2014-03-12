/******************************************************************************/
/* Copyright (c) 2013-2014 VectorChief (at github, bitbucket, sourceforge)    */
/* Distributed under the MIT software license, see the accompanying           */
/* file COPYING or http://www.opensource.org/licenses/mit-license.php         */
/******************************************************************************/

#include <string.h>

#include "object.h"
#include "rtgeom.h"
#include "rtimag.h"

/******************************************************************************/
/*********************************   LEGEND   *********************************/
/******************************************************************************/

/*
 * object.cpp: Implementation of the objects hierarchy.
 *
 * Main companion file of the engine responsible for instantiating and managing
 * the objects hierarchy. It contains the definition of Object class (the root
 * of the hierarchy) and its derivative classes along with the set of algorithms
 * needed to construct and update per-object fields and cross-object relations.
 *
 * Object handles first three phases of the update initiated by the engine:
 * 0.5 phase (sequential) - hierarchical update of matrices in the objects tree
 * - compute transform matrix from the root down to the leaf objects
 * - determine intermediate transform nodes used later for transform caching
 * - rebuild surface's custom clipping list based on scene-defined relations
 * - determine intermediate bounding volume nodes based on scene data
 * 1st phase (multi-threaded) - update auxiliary per-object data fields
 * - compute array's inverse transform matrix needed in backend (tracer.cpp)
 * - compute surface's inverse transform matrix, bounding and clipping boxes,
 *   bounding volume (sphere), backend-related SIMD fields (tracer.h)
 * 1.5 phase (sequential) - hierarchical update of array bounds from surfaces
 * - compute array's bounding box and volume (sphere) from surfaces' bounding
 *   boxes (trnode part: aux) and sub-arrays' bounding boxes (bvnode part: box)
 *
 * In order to avoid cross-dependencies on the engine, object file contains
 * the definition of Registry interface inherited by the engine's Scene class,
 * instance of which is then passed to object's constructors and serves as
 * both objects registry and memory heap (system.cpp).
 *
 * Registry's heap allocations are not allowed in multi-threaded phases
 * as SceneThread's heaps are used in this case to avoid race conditions.
 */

/******************************************************************************/
/*********************************   OBJECT   *********************************/
/******************************************************************************/

/*
 * Instantiate object.
 */
rt_Object::rt_Object(rt_Registry *rg, rt_Object *parent, rt_OBJECT *obj)
{
    if (obj == RT_NULL)
    {
        throw rt_Exception("null-pointer in object");
    }

    this->rg = rg;

    this->obj = obj;
    /* save original transform data */
    this->otm = obj->trm;
    this->trm = &obj->trm;
    this->pos = this->mtx[3];
    this->tag = obj->obj.tag;

    this->obj_changed = 0;
    this->obj_has_trm = 0;
    this->mtx_has_trm = 0;

    this->parent = parent;
    this->trnode = RT_NULL;
    this->bvnode = RT_NULL;

    /* init "box" bound structure used for bvnode if present */
    box = (rt_BOUND *)rg->alloc(RT_IS_SURFACE(this) ?
                        sizeof(rt_SHAPE) : sizeof(rt_BOUND), RT_QUAD_ALIGN);

    memset(box, 0, sizeof(rt_BOUND));
    box->obj = this;
    box->tag = this->tag;
    box->pinv = &this->inv;
    box->pmtx = &this->mtx;
    box->pos = this->mtx[3];
    box->map = this->map;
    box->sgn = this->sgn;
    box->opts = &rg->opts;

    obj->time = -1;
}

/*
 * Build relations template based on given template "lst" from scene data.
 */
rt_void rt_Object::add_relation(rt_ELEM *lst)
{

}

/*
 * Update bvnode pointer with given "mode".
 */
rt_void rt_Object::update_bvnode(rt_Array *bvnode, rt_bool mode)
{
    /* bvnode cannot be its own bvnode,
     * there is no bvnode for boundless surfaces */
    if (bvnode == this || box->verts_num == 0)
    {
        return;
    }
    if (mode == RT_TRUE  && this->bvnode == RT_NULL)
    {
        this->bvnode = bvnode;
    }
    if (mode == RT_FALSE && this->bvnode == bvnode)
    {
        this->bvnode = RT_NULL;
    }
}

/*
 * Update object with given "time", matrix "mtx" and "flags".
 */
rt_void rt_Object::update_matrix(rt_long time, rt_mat4 mtx, rt_cell flags)
{
    if (obj->f_anim != RT_NULL && obj->time != time)
    {
        obj->f_anim(time, obj->time < 0 ? 0 : obj->time, trm, RT_NULL);
    }

    obj->time = time;

    /* inherit changed status from the hierarchy */
    obj_changed = flags & RT_UPDATE_FLAG_ARR;

    if (obj->f_anim != RT_NULL)
    {
        obj_changed = RT_UPDATE_FLAG_ARR;
    }

    if (obj_changed == 0)
    {
        return;
    }

    /* determine object's own transform for transform caching,
     * which allows to apply single matrix transform
     * in rendering backend for array of objects
     * with trivial transform in relation to array node */
    rt_cell i, c;

    /* reset object's own transform flags */
    mtx_has_trm = 0;

    rt_real scl[] = {-1.0f, +1.0f};

    for (i = 0, c = 0; i < RT_ARR_SIZE(scl); i++)
    {
        if (trm->scl[RT_X] == scl[i]) c++;
        if (trm->scl[RT_Y] == scl[i]) c++;
        if (trm->scl[RT_Z] == scl[i]) c++;
    }

    /* determine if object itself has
     * non-trivial scaling */
    mtx_has_trm |= (c == 3) ? 0 : RT_UPDATE_FLAG_SCL;

    rt_real rot[] = {-270.0f, -180.0f, -90.0f, 0.0f, +90.0f, +180.0f, +270.0f};

    for (i = 0, c = 0; i < RT_ARR_SIZE(rot); i++)
    {
        if (trm->rot[RT_X] == rot[i]) c++;
        if (trm->rot[RT_Y] == rot[i]) c++;
        if (trm->rot[RT_Z] == rot[i]) c++;
    }

    /* determine if object itself has
     * non-trivial rotation */
    mtx_has_trm |= (c == 3) ? 0 : RT_UPDATE_FLAG_ROT;

    if (mtx_has_trm
#if RT_OPTS_FSCALE != 0
    && (rg->opts & RT_OPTS_FSCALE) == 0
#endif /* RT_OPTS_FSCALE */
       )
    {
        mtx_has_trm = RT_UPDATE_FLAG_SCL | RT_UPDATE_FLAG_ROT;
    }

    /* determine if object's full matrix has
     * non-trivial transform */
    obj_has_trm = mtx_has_trm |
                        (flags & RT_UPDATE_FLAG_SCL) |
                        (flags & RT_UPDATE_FLAG_ROT);

    /* search for object's trnode
     * (node up in the hierarchy with non-trivial transform,
     * relative to which object has trivial transform),
     * can be potentially optimized out by passing
     * correct trnode as parameter to update */
    for (trnode = parent; trnode != RT_NULL; trnode = trnode->parent)
    {
        if (trnode->mtx_has_trm)
        {
            break;
        }
    }

    /* if object has its parent as trnode,
     * object's transform matrix has only its own transform,
     * except the case of scaling with trivial rotation,
     * when trnode's axis mapping is passed to sub-objects */
    if (trnode != RT_NULL && trnode == parent && mtx_has_trm == 0
    &&  obj_has_trm & RT_UPDATE_FLAG_ROT)
    {
        matrix_from_transform(this->mtx, trm);
    }
    else
    /* if object itself has non-trivial transform, recombine matrices
     * before and after trnode with object's own matrix
     * to obtain object's full transform matrix
     * (no caching for this obj, it is its own trnode) */
    if (trnode != RT_NULL && trnode != parent && mtx_has_trm)
    {
        rt_mat4 obj_mtx, tmp_mtx;
        matrix_from_transform(obj_mtx, trm);
        matrix_mul_matrix(tmp_mtx, trnode->mtx, mtx);
        matrix_mul_matrix(this->mtx, tmp_mtx, obj_mtx);
    }
    else
    /* compute object's transform matrix as matrix from the hierarchy
     * (either from trnode or from root) multiplied by its own matrix */
    {
        rt_mat4 obj_mtx;
        matrix_from_transform(obj_mtx, trm);
        matrix_mul_matrix(this->mtx, mtx, obj_mtx);
    }

    /* if object itself has non-trivial transform, it is its own trnode,
     * not considering the case when object's transform is compensated
     * by parents' transforms resulting in object being axis-aligned */
    if (mtx_has_trm)
    {
        trnode = this;
    }

    /* always compute full transform matrix
     * for non-surface / non-array objects
     * or all objects if transform caching is disabled */
    if (trnode != RT_NULL && trnode != this
#if RT_OPTS_TARRAY != 0
    && ((rg->opts & RT_OPTS_TARRAY) == 0 || tag > RT_TAG_SURFACE_MAX)
#endif /* RT_OPTS_TARRAY */
       )
    {
        rt_mat4 tmp_mtx;
        matrix_mul_matrix(tmp_mtx, trnode->mtx, this->mtx);
        memcpy(this->mtx, tmp_mtx, sizeof(rt_mat4));

        trnode = this;
    }

    box->trnode = trnode != RT_NULL ? trnode->box : RT_NULL;

    /* axis mapping for trivial transform */
    map[RT_I] = RT_X;
    map[RT_J] = RT_Y;
    map[RT_K] = RT_Z;
    map[RT_L] = RT_W;

    sgn[RT_I] = 1;
    sgn[RT_J] = 1;
    sgn[RT_K] = 1;
    sgn[RT_L] = 1;

    /* axis mapping shorteners */
    mp_i = map[RT_I];
    mp_j = map[RT_J];
    mp_k = map[RT_K];
    mp_l = map[RT_L];
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Object::update_fields()
{

}

/*
 * Destroy object.
 */
rt_Object::~rt_Object()
{
    /* restore original transform data */
    obj->trm = otm;
}

/******************************************************************************/
/*********************************   CAMERA   *********************************/
/******************************************************************************/

/*
 * Instantiate camera object.
 */
rt_Camera::rt_Camera(rt_Registry *rg, rt_Object *parent, rt_OBJECT *obj) :

    rt_Object(rg, parent, obj),
    rt_List<rt_Camera>(rg->get_cam())
{
    rg->put_cam(this);

    this->cam = (rt_CAMERA *)obj->obj.pobj;

    if (cam->col.val != 0x0)
    {
        cam->col.hdr[RT_R] = ((cam->col.val >> 0x10) & 0xFF) / 255.0f;
        cam->col.hdr[RT_G] = ((cam->col.val >> 0x08) & 0xFF) / 255.0f;
        cam->col.hdr[RT_B] = ((cam->col.val >> 0x00) & 0xFF) / 255.0f;
    }

    hor = this->mtx[0];
    ver = this->mtx[1];
    nrm = this->mtx[2];

    pov = cam->vpt[0] <= 0.0f ? 1.0f : /* default pov */
          cam->vpt[0] <= 2 * RT_CLIP_THRESHOLD ? /* minimum positive pov */
                         2 * RT_CLIP_THRESHOLD : cam->vpt[0];

    cam_changed = 0;
}

/*
 * Update object with given "time", matrix "mtx" and "flags".
 */
rt_void rt_Camera::update_matrix(rt_long time, rt_mat4 mtx, rt_cell flags)
{
    rt_Object::update_matrix(time, mtx, flags | cam_changed);

    update_fields();
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Camera::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Object::update_fields();

    hor_sin = RT_SINA(trm->rot[RT_Z]);
    hor_cos = RT_COSA(trm->rot[RT_Z]);

    cam_changed = 0;
}

/*
 * Update camera with given "time" and "action".
 */
rt_void rt_Camera::update_action(rt_long time, rt_cell action)
{
    rt_real t = (time - obj->time) / 50.0f;

    switch (action)
    {
        /* vertical movement */
        case RT_CAMERA_MOVE_UP:
        trm->pos[RT_Z] += cam->dps[RT_K] * t;
        break;

        case RT_CAMERA_MOVE_DOWN:
        trm->pos[RT_Z] -= cam->dps[RT_K] * t;
        break;

        /* horizontal movement */
        case RT_CAMERA_MOVE_LEFT:
        trm->pos[RT_X] -= cam->dps[RT_I] * t * hor_cos;
        trm->pos[RT_Y] -= cam->dps[RT_I] * t * hor_sin;
        break;

        case RT_CAMERA_MOVE_RIGHT:
        trm->pos[RT_X] += cam->dps[RT_I] * t * hor_cos;
        trm->pos[RT_Y] += cam->dps[RT_I] * t * hor_sin;
        break;

        case RT_CAMERA_MOVE_BACK:
        trm->pos[RT_X] += cam->dps[RT_J] * t * hor_sin;
        trm->pos[RT_Y] -= cam->dps[RT_J] * t * hor_cos;
        break;

        case RT_CAMERA_MOVE_FORWARD:
        trm->pos[RT_X] -= cam->dps[RT_J] * t * hor_sin;
        trm->pos[RT_Y] += cam->dps[RT_J] * t * hor_cos;
        break;

        /* horizontal rotation */
        case RT_CAMERA_ROTATE_LEFT:
        trm->rot[RT_Z] += cam->drt[RT_I] * t;
        if (trm->rot[RT_Z] >= +180.0f)
        {
            trm->rot[RT_Z] -= +360.0f;
        }
        break;

        case RT_CAMERA_ROTATE_RIGHT:
        trm->rot[RT_Z] -= cam->drt[RT_I] * t;
        if (trm->rot[RT_Z] <= -180.0f)
        {
            trm->rot[RT_Z] += +360.0f;
        }
        break;

        /* vertical rotation */
        case RT_CAMERA_ROTATE_UP:
        if (trm->rot[RT_X] <  0.0f)
        {
            trm->rot[RT_X] += cam->drt[RT_J] * t;
            if (trm->rot[RT_X] >  0.0f)
                trm->rot[RT_X] =  0.0f;
        }
        break;

        case RT_CAMERA_ROTATE_DOWN:
        if (trm->rot[RT_X] > -180.0f)
        {
            trm->rot[RT_X] -= cam->drt[RT_J] * t;
            if (trm->rot[RT_X] < -180.0f)
                trm->rot[RT_X] = -180.0f;
        }
        break;

        default:
        break;
    }

    cam_changed = RT_UPDATE_FLAG_ARR;
}

/*
 * Destroy camera object.
 */
rt_Camera::~rt_Camera()
{

}

/******************************************************************************/
/**********************************   LIGHT   *********************************/
/******************************************************************************/

/*
 * Instantiate light object.
 */
rt_Light::rt_Light(rt_Registry *rg, rt_Object *parent, rt_OBJECT *obj) :

    rt_Object(rg, parent, obj),
    rt_List<rt_Light>(rg->get_lgt())
{
    rg->put_lgt(this);

    this->lgt = (rt_LIGHT *)obj->obj.pobj;

    if (lgt->col.val != 0x0)
    {
        lgt->col.hdr[RT_R] = ((lgt->col.val >> 0x10) & 0xFF) / 255.0f;
        lgt->col.hdr[RT_G] = ((lgt->col.val >> 0x08) & 0xFF) / 255.0f;
        lgt->col.hdr[RT_B] = ((lgt->col.val >> 0x00) & 0xFF) / 255.0f;
    }

/*  rt_SIMD_LIGHT */

    s_lgt = (rt_SIMD_LIGHT *)rg->alloc(sizeof(rt_SIMD_LIGHT), RT_SIMD_ALIGN);

    RT_SIMD_SET(s_lgt->t_max, 1.0f);

    RT_SIMD_SET(s_lgt->col_r, lgt->col.hdr[RT_R] * lgt->lum[1]);
    RT_SIMD_SET(s_lgt->col_g, lgt->col.hdr[RT_G] * lgt->lum[1]);
    RT_SIMD_SET(s_lgt->col_b, lgt->col.hdr[RT_B] * lgt->lum[1]);

    RT_SIMD_SET(s_lgt->a_qdr, lgt->atn[3]);
    RT_SIMD_SET(s_lgt->a_lnr, lgt->atn[2]);
    RT_SIMD_SET(s_lgt->a_cnt, lgt->atn[1] + 1.0f);
    RT_SIMD_SET(s_lgt->a_rng, lgt->atn[0]);
}

/*
 * Update object with given "time", matrix "mtx" and "flags".
 */
rt_void rt_Light::update_matrix(rt_long time, rt_mat4 mtx, rt_cell flags)
{
    rt_Object::update_matrix(time, mtx, flags);

    update_fields();
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Light::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Object::update_fields();

    RT_SIMD_SET(s_lgt->pos_x, pos[RT_X]);
    RT_SIMD_SET(s_lgt->pos_y, pos[RT_Y]);
    RT_SIMD_SET(s_lgt->pos_z, pos[RT_Z]);
}

/*
 * Destroy light object.
 */
rt_Light::~rt_Light()
{

}

/******************************************************************************/
/**********************************   NODE   **********************************/
/******************************************************************************/

static
rt_EDGE bx_edges[] = 
{
    {0x0, 0x1},
    {0x1, 0x2},
    {0x2, 0x3},
    {0x3, 0x0},
    {0x0, 0x4},
    {0x1, 0x5},
    {0x2, 0x6},
    {0x3, 0x7},
    {0x7, 0x6},
    {0x6, 0x5},
    {0x5, 0x4},
    {0x4, 0x7},
};

static
rt_FACE bx_faces[] = 
{
    {0x0, 0x1, 0x2, 0x3},
    {0x0, 0x4, 0x5, 0x1},
    {0x1, 0x5, 0x6, 0x2},
    {0x2, 0x6, 0x7, 0x3},
    {0x3, 0x7, 0x4, 0x0},
    {0x7, 0x6, 0x5, 0x4},
};

/*
 * Instantiate node object.
 */
rt_Node::rt_Node(rt_Registry *rg, rt_Object *parent,
                 rt_OBJECT *obj, rt_cell ssize) :

    rt_Object(rg, parent, obj)
{
    /* reset relations template */
    rel = RT_NULL;

/*  rt_SIMD_SURFACE */

    s_srf = (rt_SIMD_SURFACE *)rg->alloc(ssize, RT_SIMD_ALIGN);

    s_srf->mat_p[0] = RT_NULL; /* outer material */
    s_srf->mat_p[1] = RT_NULL; /* outer material props */
    s_srf->mat_p[2] = RT_NULL; /* inner material */
    s_srf->mat_p[3] = RT_NULL; /* inner material props */

    s_srf->srf_p[0] = RT_NULL; /* surf ptr, filled in update0 */
    s_srf->srf_p[1] = RT_NULL; /* reserved */
    s_srf->srf_p[2] = RT_NULL; /* clip ptr, filled in update0 */
    s_srf->srf_p[3] = (rt_pntr)tag; /* tag */

    s_srf->msc_p[0] = RT_NULL; /* screen tiles */
    s_srf->msc_p[1] = RT_NULL; /* reserved */
    s_srf->msc_p[2] = RT_NULL; /* custom clippers */
    s_srf->msc_p[3] = RT_NULL; /* trnode's simd ptr */

    s_srf->lst_p[0] = RT_NULL; /* outer lights/shadows */
    s_srf->lst_p[1] = RT_NULL; /* outer surfaces for rfl/rfr */
    s_srf->lst_p[2] = RT_NULL; /* inner lights/shadows */
    s_srf->lst_p[3] = RT_NULL; /* inner surfaces for rfl/rfr */

    RT_SIMD_SET(s_srf->sbase, 0x00000000);
    RT_SIMD_SET(s_srf->smask, 0x80000000);
    RT_SIMD_SET(s_srf->c_tmp, 0xFFFFFFFF);
}

/*
 * Build relations template based on given template "lst" from scene data.
 */
rt_void rt_Node::add_relation(rt_ELEM *lst)
{
    rt_Object::add_relation(lst);
}

/*
 * Update bvnode pointer with given "mode".
 */
rt_void rt_Node::update_bvnode(rt_Array *bvnode, rt_bool mode)
{
    rt_Object::update_bvnode(bvnode, mode);
}

/*
 * Update object with given "time", matrix "mtx" and "flags".
 */
rt_void rt_Node::update_matrix(rt_long time, rt_mat4 mtx, rt_cell flags)
{
    rt_Object::update_matrix(time, mtx, flags);

    if (obj_changed == 0)
    {
        return;
    }

    rt_cell i, j;
    rt_vec4 scl;

    /* determine axis mapping for trivial transform
     * (multiple of 90 degree rotation, +/-1.0 scalers),
     * applicable to objects without trnode or with trnode
     * other that the object itself (transform caching),
     * to objects which have scaling with trivial rotation
     * in their full transform matrix */
    if (trnode != this
    ||  obj_has_trm == RT_UPDATE_FLAG_SCL)
    {
        for (i = 0; i < 3; i++)
        {
            for (j = 0; j < 3; j++)
            {
                if ((this->mtx[i][0] != 0.0f) == (iden4[j][0] != 0.0f)
                &&  (this->mtx[i][1] != 0.0f) == (iden4[j][1] != 0.0f)
                &&  (this->mtx[i][2] != 0.0f) == (iden4[j][2] != 0.0f))
                {
                    map[i] = j;
                    sgn[i] = RT_SIGN(this->mtx[i][j]);
                    scl[i] = RT_FABS(this->mtx[i][j]);
                }
            }
        }

        /* axis mapping shorteners */
        mp_i = map[RT_I];
        mp_j = map[RT_J];
        mp_k = map[RT_K];
        mp_l = map[RT_L];
    }

    /* if object itself has non-trivial transform
     * and it is scaling with trivial rotation,
     * separate axis mapping from transform matrix,
     * which would then only have scalers on main diagonal */
    if (trnode == this
    &&  obj_has_trm == RT_UPDATE_FLAG_SCL)
    {
        for (i = 0; i < 3; i++)
        {
            j = map[i];
            this->mtx[j][0] = iden4[j][0] * scl[i];
            this->mtx[j][1] = iden4[j][1] * scl[i];
            this->mtx[j][2] = iden4[j][2] * scl[i];
        }
    }
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Node::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Object::update_fields();

    /* compute object's inverted transform matrix and store
     * its values into backend fields along with current position */
    if (trnode == this)
    {
        matrix_inverse(inv, this->mtx);

        RT_SIMD_SET(s_srf->tci_x, inv[RT_X][RT_I]);
        RT_SIMD_SET(s_srf->tci_y, inv[RT_Y][RT_I]);
        RT_SIMD_SET(s_srf->tci_z, inv[RT_Z][RT_I]);

        RT_SIMD_SET(s_srf->tcj_x, inv[RT_X][RT_J]);
        RT_SIMD_SET(s_srf->tcj_y, inv[RT_Y][RT_J]);
        RT_SIMD_SET(s_srf->tcj_z, inv[RT_Z][RT_J]);

        RT_SIMD_SET(s_srf->tck_x, inv[RT_X][RT_K]);
        RT_SIMD_SET(s_srf->tck_y, inv[RT_Y][RT_K]);
        RT_SIMD_SET(s_srf->tck_z, inv[RT_Z][RT_K]);
    }

    RT_SIMD_SET(s_srf->pos_x, pos[RT_X]);
    RT_SIMD_SET(s_srf->pos_y, pos[RT_Y]);
    RT_SIMD_SET(s_srf->pos_z, pos[RT_Z]);
}

/*
 * Update bounding box and volume geometry.
 */
rt_void rt_Node::update_bbgeom(rt_BOUND *box)
{
    if (box->obj != this)
    {
        throw rt_Exception("incorrect box in update_bbgeom");
    }

    do /* use "do {break} while(0)" instead of "goto label" */
    {
        /* bvnode's bbox is always in world space,
         * thus skip transform (in else branch) even if trnode is present */
        if (trnode != RT_NULL && !(RT_IS_ARRAY(this) && this->box == box))
        {
            rt_mat4 *pmtx = &trnode->mtx;

            rt_vec4 vt0;
            vt0[mp_i] = box->bmax[mp_i];
            vt0[mp_j] = box->bmax[mp_j];
            vt0[mp_k] = box->bmax[mp_k];
            vt0[mp_l] = 1.0f; /* takes pos in mtx into account */

            rt_vec4 vt1;
            vt1[mp_i] = box->bmin[mp_i];
            vt1[mp_j] = box->bmax[mp_j];
            vt1[mp_k] = box->bmax[mp_k];
            vt1[mp_l] = 1.0f; /* takes pos in mtx into account */

            rt_vec4 vt2;
            vt2[mp_i] = box->bmin[mp_i];
            vt2[mp_j] = box->bmin[mp_j];
            vt2[mp_k] = box->bmax[mp_k];
            vt2[mp_l] = 1.0f; /* takes pos in mtx into account */

            rt_vec4 vt3;
            vt3[mp_i] = box->bmax[mp_i];
            vt3[mp_j] = box->bmin[mp_j];
            vt3[mp_k] = box->bmax[mp_k];
            vt3[mp_l] = 1.0f; /* takes pos in mtx into account */

            matrix_mul_vector(box->verts[0x0].pos, *pmtx, vt0);
            matrix_mul_vector(box->verts[0x1].pos, *pmtx, vt1);
            matrix_mul_vector(box->verts[0x2].pos, *pmtx, vt2);
            matrix_mul_vector(box->verts[0x3].pos, *pmtx, vt3);

            box->edges[0x0].k = 3;
            box->edges[0x1].k = 3;
            box->edges[0x2].k = 3;
            box->edges[0x3].k = 3;

            box->faces[0x0].k = 3;
            box->faces[0x0].i = 3;
            box->faces[0x0].j = 3;

            if (tag == RT_TAG_PLANE)
            {
                break;
            }

            rt_vec4 vt4;
            vt4[mp_i] = box->bmax[mp_i];
            vt4[mp_j] = box->bmax[mp_j];
            vt4[mp_k] = box->bmin[mp_k];
            vt4[mp_l] = 1.0f; /* takes pos in mtx into account */

            rt_vec4 vt5;
            vt5[mp_i] = box->bmin[mp_i];
            vt5[mp_j] = box->bmax[mp_j];
            vt5[mp_k] = box->bmin[mp_k];
            vt5[mp_l] = 1.0f; /* takes pos in mtx into account */

            rt_vec4 vt6;
            vt6[mp_i] = box->bmin[mp_i];
            vt6[mp_j] = box->bmin[mp_j];
            vt6[mp_k] = box->bmin[mp_k];
            vt6[mp_l] = 1.0f; /* takes pos in mtx into account */

            rt_vec4 vt7;
            vt7[mp_i] = box->bmax[mp_i];
            vt7[mp_j] = box->bmin[mp_j];
            vt7[mp_k] = box->bmin[mp_k];
            vt7[mp_l] = 1.0f; /* takes pos in mtx into account */

            matrix_mul_vector(box->verts[0x4].pos, *pmtx, vt4);
            matrix_mul_vector(box->verts[0x5].pos, *pmtx, vt5);
            matrix_mul_vector(box->verts[0x6].pos, *pmtx, vt6);
            matrix_mul_vector(box->verts[0x7].pos, *pmtx, vt7);

            box->edges[0x4].k = 3;
            box->edges[0x5].k = 3;
            box->edges[0x6].k = 3;
            box->edges[0x7].k = 3;

            box->edges[0x8].k = 3;
            box->edges[0x9].k = 3;
            box->edges[0xA].k = 3;
            box->edges[0xB].k = 3;

            box->faces[0x1].k = 3;
            box->faces[0x1].i = 3;
            box->faces[0x1].j = 3;

            box->faces[0x2].k = 3;
            box->faces[0x2].i = 3;
            box->faces[0x2].j = 3;

            box->faces[0x3].k = 3;
            box->faces[0x3].i = 3;
            box->faces[0x3].j = 3;

            box->faces[0x4].k = 3;
            box->faces[0x4].i = 3;
            box->faces[0x4].j = 3;

            box->faces[0x5].k = 3;
            box->faces[0x5].i = 3;
            box->faces[0x5].j = 3;
        }
        else
        {
            box->verts[0x0].pos[mp_i] = box->bmax[mp_i];
            box->verts[0x0].pos[mp_j] = box->bmax[mp_j];
            box->verts[0x0].pos[mp_k] = box->bmax[mp_k];
            box->verts[0x0].pos[mp_l] = 1.0f;

            box->verts[0x1].pos[mp_i] = box->bmin[mp_i];
            box->verts[0x1].pos[mp_j] = box->bmax[mp_j];
            box->verts[0x1].pos[mp_k] = box->bmax[mp_k];
            box->verts[0x1].pos[mp_l] = 1.0f;

            box->verts[0x2].pos[mp_i] = box->bmin[mp_i];
            box->verts[0x2].pos[mp_j] = box->bmin[mp_j];
            box->verts[0x2].pos[mp_k] = box->bmax[mp_k];
            box->verts[0x2].pos[mp_l] = 1.0f;

            box->verts[0x3].pos[mp_i] = box->bmax[mp_i];
            box->verts[0x3].pos[mp_j] = box->bmin[mp_j];
            box->verts[0x3].pos[mp_k] = box->bmax[mp_k];
            box->verts[0x3].pos[mp_l] = 1.0f;

            box->edges[0x0].k = mp_i;
            box->edges[0x1].k = mp_j;
            box->edges[0x2].k = mp_i;
            box->edges[0x3].k = mp_j;

            box->faces[0x0].k = mp_k;
            box->faces[0x0].i = mp_i;
            box->faces[0x0].j = mp_j;

            if (tag == RT_TAG_PLANE)
            {
                break;
            }

            box->verts[0x4].pos[mp_i] = box->bmax[mp_i];
            box->verts[0x4].pos[mp_j] = box->bmax[mp_j];
            box->verts[0x4].pos[mp_k] = box->bmin[mp_k];
            box->verts[0x4].pos[mp_l] = 1.0f;

            box->verts[0x5].pos[mp_i] = box->bmin[mp_i];
            box->verts[0x5].pos[mp_j] = box->bmax[mp_j];
            box->verts[0x5].pos[mp_k] = box->bmin[mp_k];
            box->verts[0x5].pos[mp_l] = 1.0f;

            box->verts[0x6].pos[mp_i] = box->bmin[mp_i];
            box->verts[0x6].pos[mp_j] = box->bmin[mp_j];
            box->verts[0x6].pos[mp_k] = box->bmin[mp_k];
            box->verts[0x6].pos[mp_l] = 1.0f;

            box->verts[0x7].pos[mp_i] = box->bmax[mp_i];
            box->verts[0x7].pos[mp_j] = box->bmin[mp_j];
            box->verts[0x7].pos[mp_k] = box->bmin[mp_k];
            box->verts[0x7].pos[mp_l] = 1.0f;

            box->edges[0x4].k = mp_k;
            box->edges[0x5].k = mp_k;
            box->edges[0x6].k = mp_k;
            box->edges[0x7].k = mp_k;

            box->edges[0x8].k = mp_i;
            box->edges[0x9].k = mp_j;
            box->edges[0xA].k = mp_i;
            box->edges[0xB].k = mp_j;

            box->faces[0x1].k = mp_j;
            box->faces[0x1].i = mp_k;
            box->faces[0x1].j = mp_i;

            box->faces[0x2].k = mp_i;
            box->faces[0x2].i = mp_k;
            box->faces[0x2].j = mp_j;

            box->faces[0x3].k = mp_j;
            box->faces[0x3].i = mp_k;
            box->faces[0x3].j = mp_i;

            box->faces[0x4].k = mp_i;
            box->faces[0x4].i = mp_k;
            box->faces[0x4].j = mp_j;

            box->faces[0x5].k = mp_k;
            box->faces[0x5].i = mp_i;
            box->faces[0x5].j = mp_j;
        }
    }
    while (0);

    RT_VEC3_SET_VAL1(box->mid, 0.0f);
    box->rad = 0.0f;

    /* this function isn't called
     * if box->verts_num == 0 */
    rt_cell i;
    rt_real f = 1.0f / (rt_real)box->verts_num;

    for (i = 0; i < box->verts_num; i++)
    {
        RT_VEC3_MAD_VAL1(box->mid, box->verts[i].pos, f);
    }

    for (i = 0; i < box->verts_num; i++)
    {
        rt_vec4 dff_vec;
        RT_VEC3_SUB(dff_vec, box->mid, box->verts[i].pos);
        rt_real dff_dot = RT_VEC3_DOT(dff_vec, dff_vec);

        if (box->rad < dff_dot)
        {
            box->rad = dff_dot;
        }
    }

    box->rad = RT_SQRT(box->rad);
}

/*
 * Destroy node object.
 */
rt_Node::~rt_Node()
{

}

/******************************************************************************/
/**********************************   ARRAY   *********************************/
/******************************************************************************/

/*
 * Instantiate array object.
 */
rt_Array::rt_Array(rt_Registry *rg, rt_Object *parent,
                   rt_OBJECT *obj, rt_cell ssize) :

    rt_Node(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_SURFACE))),
    rt_List<rt_Array>(rg->get_arr())
{
    rg->put_arr(this);

    /* init "aux" bound structure used for trnode if present */
    aux = (rt_BOUND *)rg->alloc(sizeof(rt_BOUND), RT_QUAD_ALIGN);

    memset(aux, 0, sizeof(rt_BOUND));
    aux->obj = this;
    aux->tag = this->tag;
    aux->pinv = &this->inv;
    aux->pmtx = &this->mtx;
    aux->pos = this->mtx[3];
    aux->map = this->map;
    aux->sgn = this->sgn;
    aux->opts = &rg->opts;

    if (RT_TRUE)
    {
        aux->verts_num = 8;
        aux->verts = (rt_VERT *)
                     rg->alloc(aux->verts_num * sizeof(rt_VERT), RT_ALIGN);

        aux->edges_num = RT_ARR_SIZE(bx_edges);
        aux->edges = (rt_EDGE *)
                     rg->alloc(aux->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(aux->edges, bx_edges, aux->edges_num * sizeof(rt_EDGE));

        aux->faces_num = RT_ARR_SIZE(bx_faces);
        aux->faces = (rt_FACE *)
                     rg->alloc(aux->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(aux->faces, bx_faces, aux->faces_num * sizeof(rt_FACE));
    }

    if (RT_TRUE)
    {
        box->verts_num = 8;
        box->verts = (rt_VERT *)
                     rg->alloc(box->verts_num * sizeof(rt_VERT), RT_ALIGN);

        box->edges_num = RT_ARR_SIZE(bx_edges);
        box->edges = (rt_EDGE *)
                     rg->alloc(box->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(box->edges, bx_edges, box->edges_num * sizeof(rt_EDGE));

        box->faces_num = RT_ARR_SIZE(bx_faces);
        box->faces = (rt_FACE *)
                     rg->alloc(box->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(box->faces, bx_faces, box->faces_num * sizeof(rt_FACE));
    }

    /* process array's objects */
    rt_OBJECT *arr = (rt_OBJECT *)obj->obj.pobj;

    obj_num = obj->obj.obj_num;
    obj_arr = (rt_Object **)rg->alloc(obj_num * sizeof(rt_Object *), RT_ALIGN);

    rt_cell i, j; /* j - for skipping unsupported object tags */

    /* instantiate every object in array from scene data,
     * including sub-arrays (recursive) */
    for (i = 0, j = 0; i < obj->obj.obj_num; i++, j++)
    {
        switch (arr[i].obj.tag)
        {
            case RT_TAG_CAMERA:
            obj_arr[j] = new rt_Camera(rg, this, &arr[i]);
            break;

            case RT_TAG_LIGHT:
            obj_arr[j] = new rt_Light(rg, this, &arr[i]);
            break;

            case RT_TAG_ARRAY:
            obj_arr[j] = new rt_Array(rg, this, &arr[i]);
            break;

            case RT_TAG_PLANE:
            obj_arr[j] = new rt_Plane(rg, this, &arr[i]);
            break;

            case RT_TAG_CYLINDER:
            obj_arr[j] = new rt_Cylinder(rg, this, &arr[i]);
            break;

            case RT_TAG_SPHERE:
            obj_arr[j] = new rt_Sphere(rg, this, &arr[i]);
            break;

            case RT_TAG_CONE:
            obj_arr[j] = new rt_Cone(rg, this, &arr[i]);
            break;

            case RT_TAG_PARABOLOID:
            obj_arr[j] = new rt_Paraboloid(rg, this, &arr[i]);
            break;

            case RT_TAG_HYPERBOLOID:
            obj_arr[j] = new rt_Hyperboloid(rg, this, &arr[i]);
            break;

            default:
            j--;
            obj_num--;
            break;
        }
    }

    /* process array's relations */
    rt_RELATION *rel = obj->obj.prel;

    /* maintain reusable relations template list linked via "simd"
     * used via "ptr", so that list elements are not reallocated */
    rt_ELEM *lst = RT_NULL, *prv = RT_NULL, **ptr = &rg->rel;
    rt_cell acc  = 0;

    rt_Object **obj_arr_l = obj_arr; /* left  sub-array */
    rt_Object **obj_arr_r = obj_arr; /* right sub-array */

    rt_cell     obj_num_l = obj_num; /* left  sub-array size */
    rt_cell     obj_num_r = obj_num; /* right sub-array size */

    /* build relations templates (custom clippers) from scene data */
    for (i = 0; i < obj->obj.rel_num; i++)
    {
        if (rel[i].obj1 >= obj_num_l
        ||  rel[i].obj2 >= obj_num_r)
        {
            continue;
        }

        rt_ELEM *elm = RT_NULL;

        rt_Object *obj = RT_NULL;
        rt_Array *arr = RT_NULL;
        rt_bool mode = RT_FALSE;

        switch (rel[i].rel)
        {
            case RT_REL_INDEX_ARRAY:
            if (rel[i].obj1 >= 0 && rel[i].obj2 >= -1
            &&  RT_IS_ARRAY(obj_arr_l[rel[i].obj1]))
            {
                rt_Array *arr = (rt_Array *)obj_arr_l[rel[i].obj1];
                obj_arr_l = arr->obj_arr; /* select left  sub-array */
                obj_num_l = arr->obj_num;   /* for next left  index */
            }
            if (rel[i].obj1 >= -1 && rel[i].obj2 >= 0
            &&  RT_IS_ARRAY(obj_arr_r[rel[i].obj2]))
            {
                rt_Array *arr = (rt_Array *)obj_arr_r[rel[i].obj2];
                obj_arr_r = arr->obj_arr; /* select right sub-array */
                obj_num_r = arr->obj_num;   /* for next right index */
            }
            break;

            case RT_REL_MINUS_INNER:
            case RT_REL_MINUS_OUTER:
            if (rel[i].obj1 == -1 && rel[i].obj2 >= 0 && acc == 0)
            {
                acc = 1;
                elm = *ptr != RT_NULL ? *ptr :
                      (rt_ELEM *)rg->alloc(sizeof(rt_ELEM), RT_QUAD_ALIGN);
                if (*ptr == RT_NULL)
                {
                   *ptr = elm;
                    elm->simd = RT_NULL;
                }
                /* use original marker value as elements are inserted
                 * into the list's tail here and the original relations
                 * template from scene data is inverted twice, first
                 * in surface's add_relation and second in engine's sclip,
                 * thus accum markers will end up in correct order */
                elm->data = RT_ACCUM_ENTER;
                elm->temp = RT_NULL; /* accum marker */
                elm->next = RT_NULL;
                lst = elm;
                prv = elm;
                ptr = RT_GET_ADR(elm->simd);
            }
            if (rel[i].obj1 >= -1 && rel[i].obj2 >= 0)
            {
                elm = *ptr != RT_NULL ? *ptr :
                      (rt_ELEM *)rg->alloc(sizeof(rt_ELEM), RT_QUAD_ALIGN);
                if (*ptr == RT_NULL)
                {
                   *ptr = elm;
                    elm->simd = RT_NULL;
                }
                elm->data = rel[i].rel;
                elm->temp = obj_arr_r[rel[i].obj2]->box;
                elm->next = RT_NULL;
                obj_arr_r = obj_arr; /* reset right sub-array after use */
                obj_num_r = obj_num;
            }
            if (rel[i].obj1 == -1 && rel[i].obj2 >= 0)
            {
                prv->next = elm;
                prv = elm;
                ptr = RT_GET_ADR(elm->simd);
            }
            break;

            case RT_REL_MINUS_ACCUM:
            if (rel[i].obj1 >= 0 && rel[i].obj2 == -1 && acc == 1)
            {
                acc = 0;
                elm = *ptr != RT_NULL ? *ptr :
                      (rt_ELEM *)rg->alloc(sizeof(rt_ELEM), RT_QUAD_ALIGN);
                if (*ptr == RT_NULL)
                {
                   *ptr = elm;
                    elm->simd = RT_NULL;
                }
                /* use original marker value as elements are inserted
                 * into the list's tail here and the original relations
                 * template from scene data is inverted twice, first
                 * in surface's add_relation and second in engine's sclip,
                 * thus accum markers will end up in correct order */
                elm->data = RT_ACCUM_LEAVE;
                elm->temp = RT_NULL; /* accum marker */
                elm->next = RT_NULL;
                prv->next = elm;
                elm = lst;
                lst = RT_NULL;
                prv = RT_NULL;
                ptr = &rg->rel;
            }
            break;

            case RT_REL_BOUND_ARRAY:
            if (rel[i].obj1 == -1 && rel[i].obj2 == -1)
            {
                obj = arr = this;
                mode = RT_TRUE;
            }
            if (rel[i].obj1 == -1 && rel[i].obj2 >= 0
            &&  RT_IS_ARRAY(obj_arr_r[rel[i].obj2]))
            {
                obj = arr = (rt_Array *)obj_arr_r[rel[i].obj2];
                mode = RT_TRUE;
            }
            break;

            case RT_REL_UNTIE_ARRAY:
            if (rel[i].obj1 == -1 && rel[i].obj2 == -1)
            {
                obj = arr = this;
                mode = RT_FALSE;
            }
            if (rel[i].obj1 == -1 && rel[i].obj2 >= 0
            &&  RT_IS_ARRAY(obj_arr_r[rel[i].obj2]))
            {
                obj = arr = (rt_Array *)obj_arr_r[rel[i].obj2];
                mode = RT_FALSE;
            }
            break;

            case RT_REL_BOUND_INDEX:
            if (rel[i].obj1 == -1 && rel[i].obj2 >= 0)
            {
                obj = obj_arr_r[rel[i].obj2];
                arr = this;
                mode = RT_TRUE;
            }
            if (rel[i].obj1 >= 0 && rel[i].obj2 >= 0
            &&  RT_IS_ARRAY(obj_arr_l[rel[i].obj1]))
            {
                obj = obj_arr_r[rel[i].obj2];
                arr = (rt_Array *)obj_arr_l[rel[i].obj1];
                mode = RT_TRUE;
            }
            break;

            case RT_REL_UNTIE_INDEX:
            if (rel[i].obj1 == -1 && rel[i].obj2 >= 0)
            {
                obj = obj_arr_r[rel[i].obj2];
                arr = this;
                mode = RT_FALSE;
            }
            if (rel[i].obj1 >= 0 && rel[i].obj2 >= 0
            &&  RT_IS_ARRAY(obj_arr_l[rel[i].obj1]))
            {
                obj = obj_arr_r[rel[i].obj2];
                arr = (rt_Array *)obj_arr_l[rel[i].obj1];
                mode = RT_FALSE;
            }
            break;

            default:
            break;
        }

        if (rel[i].obj1 >= 0 && elm != RT_NULL)
        {
            obj_arr_l[rel[i].obj1]->add_relation(elm);
            obj_arr_l = obj_arr; /* reset left  sub-array after use */
            obj_num_l = obj_num;
        }
        if (obj != RT_NULL && arr != RT_NULL)
        {
#if RT_OPTS_VARRAY != 0
            if ((rg->opts & RT_OPTS_VARRAY) != 0)
            {
                obj->update_bvnode(arr, mode);
            }
#endif /* RT_OPTS_VARRAY */
            if (rel[i].obj1 >= 0)
            {
                obj_arr_l = obj_arr; /* reset left  sub-array after use */
                obj_num_l = obj_num;
            }
            if (rel[i].obj2 >= 0)
            {
                obj_arr_r = obj_arr; /* reset right sub-array after use */
                obj_num_r = obj_num;
            }
        }
    }

/*  rt_SIMD_SURFACE */

    s_box = (rt_SIMD_SURFACE *)
            rg->alloc(RT_MAX(ssize, sizeof(rt_SIMD_SPHERE)), RT_SIMD_ALIGN);

    s_box->mat_p[0] = RT_NULL; /* outer material */
    s_box->mat_p[1] = RT_NULL; /* outer material props */
    s_box->mat_p[2] = RT_NULL; /* inner material */
    s_box->mat_p[3] = RT_NULL; /* inner material props */

    s_box->srf_p[0] = RT_NULL; /* surf ptr, filled in update0 */
    s_box->srf_p[1] = RT_NULL; /* reserved */
    s_box->srf_p[2] = RT_NULL; /* clip ptr, filled in update0 */
    s_box->srf_p[3] = (rt_pntr)tag; /* tag */

    s_box->msc_p[0] = RT_NULL; /* screen tiles */
    s_box->msc_p[1] = RT_NULL; /* reserved */
    s_box->msc_p[2] = RT_NULL; /* custom clippers */
    s_box->msc_p[3] = RT_NULL; /* trnode's simd ptr */

    s_box->lst_p[0] = RT_NULL; /* outer lights/shadows */
    s_box->lst_p[1] = RT_NULL; /* outer surfaces for rfl/rfr */
    s_box->lst_p[2] = RT_NULL; /* inner lights/shadows */
    s_box->lst_p[3] = RT_NULL; /* inner surfaces for rfl/rfr */

    RT_SIMD_SET(s_box->sbase, 0x00000000);
    RT_SIMD_SET(s_box->smask, 0x80000000);
    RT_SIMD_SET(s_box->c_tmp, 0xFFFFFFFF);
}

/*
 * Build relations template based on given template "lst" from scene data.
 */
rt_void rt_Array::add_relation(rt_ELEM *lst)
{
    rt_Node::add_relation(lst);

    rt_cell i;

    for (i = 0; i < obj_num; i++)
    {
        obj_arr[i]->add_relation(lst);
    }
}

/*
 * Update bvnode pointer for all sub-objects,
 * including sub-arrays (recursive).
 */
rt_void rt_Array::update_bvnode(rt_Array *bvnode, rt_bool mode)
{
    rt_Node::update_bvnode(bvnode, mode);

    rt_cell i;

    for (i = 0; i < obj_num; i++)
    {
        obj_arr[i]->update_bvnode(bvnode, mode);
    }
}

/*
 * Update object with given "time", matrix "mtx" and "flags".
 */
rt_void rt_Array::update_matrix(rt_long time, rt_mat4 mtx, rt_cell flags)
{
    /* update the whole hierarchy when called
     * for the first time or triggered explicitly */
#if RT_OPTS_UPDATE != 0
    if (obj->time == -1 && parent == RT_NULL
    || (rg->opts & RT_OPTS_UPDATE) == 0)
#endif /* RT_OPTS_UPDATE */
    {
        flags |= RT_UPDATE_FLAG_ARR;
    }

    rt_Node::update_matrix(time, mtx, flags);

    aux->trnode = trnode != RT_NULL ? ((rt_Array *)trnode)->aux : RT_NULL;

    rt_cell i, j;
    rt_mat4 *pmtx = &this->mtx;

    /* if array itself has non-trivial transform
     * and it is scaling with trivial rotation,
     * separate axis mapping from transform matrix,
     * axis mapping matrix is then passed to sub-objects */
    if (trnode == this
    &&  obj_has_trm == RT_UPDATE_FLAG_SCL)
    {
        if (obj_changed)
        {
            memset(axm, 0, sizeof(rt_mat4));
            axm[3][3] = 1.0f;

            for (i = 0; i < 3; i++)
            {
                j = map[i];
                axm[i][j] = sgn[i];
            }
        }

        pmtx = &axm;
    }

    /* axis mapping for trivial transform */
    map[RT_I] = RT_X;
    map[RT_J] = RT_Y;
    map[RT_K] = RT_Z;
    map[RT_L] = RT_W;

    sgn[RT_I] = 1;
    sgn[RT_J] = 1;
    sgn[RT_K] = 1;
    sgn[RT_L] = 1;

    /* axis mapping shorteners */
    mp_i = map[RT_I];
    mp_j = map[RT_J];
    mp_k = map[RT_K];
    mp_l = map[RT_L];

    /* update every object in array
     * including sub-arrays (recursive),
     * pass array's own transform flags */
    for (i = 0; i < obj_num; i++)
    {
        obj_arr[i]->update_matrix(time, *pmtx,
                                  flags | mtx_has_trm | obj_changed);
    }
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Array::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Node::update_fields();

    rt_cell shift = 0;

    s_srf->a_map[RT_I] = (mp_i + shift) * RT_SIMD_WIDTH * 4;
    s_srf->a_map[RT_J] = (mp_j + shift) * RT_SIMD_WIDTH * 4;
    s_srf->a_map[RT_K] = (mp_k + shift) * RT_SIMD_WIDTH * 4;
    s_srf->a_map[RT_L] = obj_has_trm;

    s_srf->a_sgn[RT_I] = (sgn[RT_I] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_srf->a_sgn[RT_J] = (sgn[RT_J] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_srf->a_sgn[RT_K] = (sgn[RT_K] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_srf->a_sgn[RT_L] = 0;

    s_box->a_map[RT_I] = (mp_i + shift) * RT_SIMD_WIDTH * 4;
    s_box->a_map[RT_J] = (mp_j + shift) * RT_SIMD_WIDTH * 4;
    s_box->a_map[RT_K] = (mp_k + shift) * RT_SIMD_WIDTH * 4;
    s_box->a_map[RT_L] = obj_has_trm;

    s_box->a_sgn[RT_I] = (sgn[RT_I] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_box->a_sgn[RT_J] = (sgn[RT_J] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_box->a_sgn[RT_K] = (sgn[RT_K] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_box->a_sgn[RT_L] = 0;
}

/*
 * Update bounding box and volume along with SIMD fields.
 */
rt_void rt_Array::update_bounds()
{
    RT_VEC3_SET_VAL1(aux->bmin, +RT_INF);
    RT_VEC3_SET_VAL1(aux->bmax, -RT_INF);
    aux->rad = 0.0f;

    RT_VEC3_SET_VAL1(box->bmin, +RT_INF);
    RT_VEC3_SET_VAL1(box->bmax, -RT_INF);
    box->rad = 0.0f;

    rt_cell i, j, k;

    for (i = 0; i < obj_num; i++)
    {
        rt_Node *nd = RT_NULL;
        rt_Array *arr = RT_NULL;

        if (RT_IS_ARRAY(obj_arr[i]))
        {
            nd = (rt_Node *)obj_arr[i];
            arr = (rt_Array *)nd;
            arr->update_bounds();
        }
        else
        if (RT_IS_SURFACE(obj_arr[i]))
        {
            nd = (rt_Node *)obj_arr[i];
        }
        else
        {
            continue;
        }

        if (nd->box->rad == 0.0f)
        {
            continue;
        }

        arr = (rt_Array *)(nd->trnode != nd ? nd->trnode : RT_NULL);

        if (arr != RT_NULL && RT_IS_SURFACE(nd))
        {
            if (nd->box->rad != RT_INF)
            {
                for (k = 0; k < 3; k++)
                {
                    if (arr->aux->bmin[k] > nd->box->bmin[k])
                    {
                        arr->aux->bmin[k] = nd->box->bmin[k];
                    }
                    if (arr->aux->bmax[k] < nd->box->bmax[k])
                    {
                        arr->aux->bmax[k] = nd->box->bmax[k];
                    }
                }
            }
            /* use rad temporarily as a tag for
             * "empty" (rad == 0.0f),
             * "finite" (0.0f < rad < RT_INF),
             * "infinite" (rad == RT_INF) */
            if (arr->aux->rad < nd->box->rad)
            {
                arr->aux->rad = nd->box->rad;
            }
        }

        arr = (rt_Array *)(nd->bvnode);

        if (arr != RT_NULL)
        {
            if (nd->box->rad != RT_INF)
            {
                if (nd->trnode == RT_NULL || RT_IS_ARRAY(nd))
                {
                    for (k = 0; k < 3; k++)
                    {
                        if (arr->box->bmin[k] > nd->box->bmin[k])
                        {
                            arr->box->bmin[k] = nd->box->bmin[k];
                        }
                        if (arr->box->bmax[k] < nd->box->bmax[k])
                        {
                            arr->box->bmax[k] = nd->box->bmax[k];
                        }
                    }
                }
                else
                {
                    for (j = 0; j < nd->box->verts_num; j++)
                    {
                        for (k = 0; k < 3; k++)
                        {
                            if (arr->box->bmin[k] > nd->box->verts[j].pos[k])
                            {
                                arr->box->bmin[k] = nd->box->verts[j].pos[k];
                            }
                            if (arr->box->bmax[k] < nd->box->verts[j].pos[k])
                            {
                                arr->box->bmax[k] = nd->box->verts[j].pos[k];
                            }
                        }
                    }
                }
            }
            /* use rad temporarily as a tag for
             * "empty" (rad == 0.0f),
             * "finite" (0.0f < rad < RT_INF),
             * "infinite" (rad == RT_INF) */
            if (arr->box->rad < nd->box->rad)
            {
                arr->box->rad = nd->box->rad;
            }
        }
    }

    if (aux->rad != 0.0f && aux->rad != RT_INF)
    {
        update_bbgeom(aux);
    }
    if (box->rad != 0.0f && box->rad != RT_INF)
    {
        update_bbgeom(box);
    }
    else
    {
        return;
    }

    RT_SIMD_SET(s_box->pos_x, box->mid[RT_X]);
    RT_SIMD_SET(s_box->pos_y, box->mid[RT_Y]);
    RT_SIMD_SET(s_box->pos_z, box->mid[RT_Z]);

/*  rt_SIMD_SPHERE */

    rt_SIMD_SPHERE *s_xsp = (rt_SIMD_SPHERE *)s_box;

    RT_SIMD_SET(s_xsp->rad_2, box->rad * box->rad);
}

/*
 * Destroy array object.
 */
rt_Array::~rt_Array()
{
    rt_cell i;

    for (i = 0; i < obj_num; i++)
    {
        delete obj_arr[i];
    }
}

/******************************************************************************/
/*********************************   SURFACE   ********************************/
/******************************************************************************/

/*
 * Instantiate surface object.
 */
rt_Surface::rt_Surface(rt_Registry *rg, rt_Object *parent,
                       rt_OBJECT *obj, rt_cell ssize) :

    rt_Node(rg, parent, obj, ssize),
    rt_List<rt_Surface>(rg->get_srf())
{
    rg->put_srf(this);

    this->srf = (rt_SURFACE *)obj->obj.pobj;

    this->srf_changed = 0;

    this->outer = new rt_Material(rg, &srf->side_outer,
                    obj->obj.pmat_outer ? obj->obj.pmat_outer :
                                          srf->side_outer.pmat);

    this->inner = new rt_Material(rg, &srf->side_inner,
                    obj->obj.pmat_inner ? obj->obj.pmat_inner :
                                          srf->side_inner.pmat);

    /* init surface's shape structure */
    shp = (rt_SHAPE *)box;

    shp->rad = RT_INF;
    shp->ptr = &s_srf->msc_p[2];

/*  rt_SIMD_SURFACE */

    s_srf->mat_p[0] = outer->s_mat;
    s_srf->mat_p[1] = (rt_pntr)outer->props;
    s_srf->mat_p[2] = inner->s_mat;
    s_srf->mat_p[3] = (rt_pntr)inner->props;
}

/*
 * Build relations template based on given template "lst" from scene data.
 */
rt_void rt_Surface::add_relation(rt_ELEM *lst)
{
    rt_Node::add_relation(lst);

    /* init surface's relations template */
    rt_ELEM **ptr = RT_GET_ADR(rel);

    /* build surface's relations template from given template "lst",
     * as surface's relations template is inverted in engine's sclip
     * and elements are inserted into the list's head here,
     * the original relations template from scene data is inverted twice,
     * thus accum enter/leave markers will end up in correct order */
    for (; lst != RT_NULL; lst = lst->next)
    {
        rt_ELEM *elm = RT_NULL;
        rt_cell rel = lst->data;
        rt_Object *obj = lst->temp == RT_NULL ? RT_NULL :
                         (rt_Object *)((rt_BOUND *)lst->temp)->obj;

        if (obj == RT_NULL)
        {
            /* alloc new element for accum marker */
            elm = (rt_ELEM *)rg->alloc(sizeof(rt_ELEM), RT_QUAD_ALIGN);
            elm->data = rel;
            elm->simd = RT_NULL; /* accum marker */
            elm->temp = RT_NULL;
            /* insert element as list head */
            elm->next = *ptr;
           *ptr = elm;
        }
        else
        if (RT_IS_ARRAY(obj))
        {
            rt_Array *arr = (rt_Array *)obj;
            rt_cell i;

            /* init array's relations template
             * used to avoid unnecessary allocs */
            rt_ELEM **ptr = RT_GET_ADR(arr->rel);

            /* populate array element with sub-objects */
            for (i = 0; i < arr->obj_num; i++)
            {
                elm = *ptr != RT_NULL ? *ptr :
                      (rt_ELEM *)rg->alloc(sizeof(rt_ELEM), RT_QUAD_ALIGN);
                if (*ptr == RT_NULL)
                {
                   *ptr = elm;
                    elm->simd = RT_NULL;
                }
                elm->data = rel;
                elm->temp = arr->obj_arr[i]->box;
                elm->next = RT_NULL;

                add_relation(elm);
            }
        }
        else
        if (RT_IS_SURFACE(obj))
        {
            rt_Surface *srf = (rt_Surface *)obj;

            /* alloc new element for srf */
            elm = (rt_ELEM *)rg->alloc(sizeof(rt_ELEM), RT_QUAD_ALIGN);
            elm->data = rel;
            elm->simd = srf->s_srf;
            elm->temp = srf->box;
            /* insert element as list head */
            elm->next = *ptr;
           *ptr = elm;
        }
    }
}

/*
 * Update object with given "time", matrix "mtx" and "flags".
 */
rt_void rt_Surface::update_matrix(rt_long time, rt_mat4 mtx, rt_cell flags)
{
    rt_Node::update_matrix(time, mtx, flags);

    if (obj_changed == 0)
    {
        return;
    }

    /* trnode's simd ptr is needed in rendering backend
     * to check if surface and its clippers belong to the same trnode */
    s_srf->msc_p[3] = trnode == RT_NULL ?
                                RT_NULL : ((rt_Node *)trnode)->s_srf;

    /* check bbox geometry limits */
    if (shp->verts_num > RT_VERTS_LIMIT
    ||  shp->edges_num > RT_EDGES_LIMIT
    ||  shp->faces_num > RT_FACES_LIMIT)
    {
        throw rt_Exception("bbox geometry limits exceeded in surface");
    }
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Surface::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Node::update_fields();

    /* if surface or some of its parents has non-trivial transform,
     * select aux vector fields for axis mapping in backend structures */
    rt_cell shift = trnode != RT_NULL ? 3 : 0;

    s_srf->a_map[RT_I] = (mp_i + shift) * RT_SIMD_WIDTH * 4;
    s_srf->a_map[RT_J] = (mp_j + shift) * RT_SIMD_WIDTH * 4;
    s_srf->a_map[RT_K] = (mp_k + shift) * RT_SIMD_WIDTH * 4;
    s_srf->a_map[RT_L] = obj_has_trm;

    s_srf->a_sgn[RT_I] = (sgn[RT_I] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_srf->a_sgn[RT_J] = (sgn[RT_J] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_srf->a_sgn[RT_K] = (sgn[RT_K] >= 0 ? 0 : 1) * RT_SIMD_WIDTH * 4;
    s_srf->a_sgn[RT_L] = 0;
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Surface::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                  rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                  rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_I] = smin[RT_I] > srf->min[RT_I] ? -RT_INF : smin[RT_I];
        cmin[RT_J] = smin[RT_J] > srf->min[RT_J] ? -RT_INF : smin[RT_J];
        cmin[RT_K] = smin[RT_K] > srf->min[RT_K] ? -RT_INF : smin[RT_K];

        cmax[RT_I] = smax[RT_I] < srf->max[RT_I] ? +RT_INF : smax[RT_I];
        cmax[RT_J] = smax[RT_J] < srf->max[RT_J] ? +RT_INF : smax[RT_J];
        cmax[RT_K] = smax[RT_K] < srf->max[RT_K] ? +RT_INF : smax[RT_K];
    }
}

/*
 * Transform sub-world space bounding or clipping box to local space
 * by applying axis mapping (trivial transform).
 * The sub-world space doesn't include trnode's transform matrix,
 * which is used to compute world space bounding box geometry,
 * thus minmax data always remains axis-aligned within sub-world space
 * up to trnode, or within world space if trnode is not present. 
 */
rt_void rt_Surface::invert_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                  rt_vec4 dmin, rt_vec4 dmax) /* dst */
{
    rt_vec4 tmin, tmax; /* tmp */

    rt_vec4  zro = {0.0f, 0.0f, 0.0f, 0.0f};
    rt_real *pps = trnode == this ? zro : pos;

    tmin[RT_X] = smin[RT_X] == -RT_INF ? -RT_INF : smin[RT_X] - pps[RT_X];
    tmin[RT_Y] = smin[RT_Y] == -RT_INF ? -RT_INF : smin[RT_Y] - pps[RT_Y];
    tmin[RT_Z] = smin[RT_Z] == -RT_INF ? -RT_INF : smin[RT_Z] - pps[RT_Z];

    tmax[RT_X] = smax[RT_X] == +RT_INF ? +RT_INF : smax[RT_X] - pps[RT_X];
    tmax[RT_Y] = smax[RT_Y] == +RT_INF ? +RT_INF : smax[RT_Y] - pps[RT_Y];
    tmax[RT_Z] = smax[RT_Z] == +RT_INF ? +RT_INF : smax[RT_Z] - pps[RT_Z];

    dmin[RT_I] = sgn[RT_I] > 0 ? +tmin[mp_i] : -tmax[mp_i];
    dmin[RT_J] = sgn[RT_J] > 0 ? +tmin[mp_j] : -tmax[mp_j];
    dmin[RT_K] = sgn[RT_K] > 0 ? +tmin[mp_k] : -tmax[mp_k];

    dmax[RT_I] = sgn[RT_I] > 0 ? +tmax[mp_i] : -tmin[mp_i];
    dmax[RT_J] = sgn[RT_J] > 0 ? +tmax[mp_j] : -tmin[mp_j];
    dmax[RT_K] = sgn[RT_K] > 0 ? +tmax[mp_k] : -tmin[mp_k];
}

/*
 * Transform local space bounding or clipping box to sub-world space
 * by applying axis mapping (trivial transform).
 * The sub-world space doesn't include trnode's transform matrix,
 * which is used to compute world space bounding box geometry,
 * thus minmax data always remains axis-aligned within sub-world space
 * up to trnode, or within world space if trnode is not present. 
 */
rt_void rt_Surface::direct_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                  rt_vec4 dmin, rt_vec4 dmax) /* dst */
{
    rt_vec4 tmin, tmax; /* tmp */

    rt_vec4  zro = {0.0f, 0.0f, 0.0f, 0.0f};
    rt_real *pps = trnode == this ? zro : pos;

    tmin[mp_i] = sgn[RT_I] > 0 ? +smin[RT_I] : -smax[RT_I];
    tmin[mp_j] = sgn[RT_J] > 0 ? +smin[RT_J] : -smax[RT_J];
    tmin[mp_k] = sgn[RT_K] > 0 ? +smin[RT_K] : -smax[RT_K];

    tmax[mp_i] = sgn[RT_I] > 0 ? +smax[RT_I] : -smin[RT_I];
    tmax[mp_j] = sgn[RT_J] > 0 ? +smax[RT_J] : -smin[RT_J];
    tmax[mp_k] = sgn[RT_K] > 0 ? +smax[RT_K] : -smin[RT_K];

    dmin[RT_X] = tmin[RT_X] == -RT_INF ? -RT_INF : tmin[RT_X] + pps[RT_X];
    dmin[RT_Y] = tmin[RT_Y] == -RT_INF ? -RT_INF : tmin[RT_Y] + pps[RT_Y];
    dmin[RT_Z] = tmin[RT_Z] == -RT_INF ? -RT_INF : tmin[RT_Z] + pps[RT_Z];

    dmax[RT_X] = tmax[RT_X] == +RT_INF ? +RT_INF : tmax[RT_X] + pps[RT_X];
    dmax[RT_Y] = tmax[RT_Y] == +RT_INF ? +RT_INF : tmax[RT_Y] + pps[RT_Y];
    dmax[RT_Z] = tmax[RT_Z] == +RT_INF ? +RT_INF : tmax[RT_Z] + pps[RT_Z];
}

/*
 * Recalculate bounding and clipping boxes based on given "src" box.
 */
rt_void rt_Surface::recalc_minmax(rt_vec4 smin, rt_vec4 smax,  /* src */
                                  rt_vec4 bmin, rt_vec4 bmax,  /* bbox */
                                  rt_vec4 cmin, rt_vec4 cmax)  /* cbox */
{
    rt_vec4 tmin, tmax;
    rt_vec4 lmin, lmax;

    rt_real *pmin = RT_NULL;
    rt_real *pmax = RT_NULL;

    /* accumulate bbox adjustments into cbox */
    if (smin != RT_NULL && smax != RT_NULL
    &&  bmin == RT_NULL && bmax == RT_NULL)
    {
        invert_minmax(smin, smax, tmin, tmax);

        bmin = lmin;
        bmax = lmax;

        pmin = cmin;
        pmax = cmax;

        cmin = RT_NULL;
        cmax = RT_NULL;
    }
    else
    /* apply bbox adjustments from cbox */
    if (smin != RT_NULL && smax != RT_NULL
    &&  cmin != RT_NULL && cmax != RT_NULL)
    {
        invert_minmax(smin, smax, tmin, tmax);

        RT_VEC3_MAX(tmin, tmin, srf->min);
        RT_VEC3_MIN(tmax, tmax, srf->max);
    }
    else
    /* init bbox with original axis clippers */
    if (smin == RT_NULL && smax == RT_NULL)
    {
        RT_VEC3_SET(tmin, srf->min);
        RT_VEC3_SET(tmax, srf->max);
    }

    adjust_minmax(tmin, tmax, bmin, bmax, cmin, cmax);

    /* accumulate bbox adjustments into cbox */
    if (pmin != RT_NULL && pmax != RT_NULL)
    {
        tmin[RT_I] = tmin[RT_I] == bmin[RT_I] ? -RT_INF : bmin[RT_I];
        tmin[RT_J] = tmin[RT_J] == bmin[RT_J] ? -RT_INF : bmin[RT_J];
        tmin[RT_K] = tmin[RT_K] == bmin[RT_K] ? -RT_INF : bmin[RT_K];

        tmax[RT_I] = tmax[RT_I] == bmax[RT_I] ? +RT_INF : bmax[RT_I];
        tmax[RT_J] = tmax[RT_J] == bmax[RT_J] ? +RT_INF : bmax[RT_J];
        tmax[RT_K] = tmax[RT_K] == bmax[RT_K] ? +RT_INF : bmax[RT_K];

        direct_minmax(tmin, tmax, tmin, tmax);

        RT_VEC3_MAX(pmin, pmin, tmin);
        RT_VEC3_MIN(pmax, pmax, tmax);

        bmin = RT_NULL;
        bmax = RT_NULL;
    }

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        direct_minmax(bmin, bmax, bmin, bmax);
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        direct_minmax(cmin, cmax, cmin, cmax);
    }
}

/*
 * Update bounding and clipping boxes data.
 */
rt_void rt_Surface::update_minmax()
{
    srf_changed = obj_changed;

    /* init custom clippers list */
    rt_ELEM *elm = (rt_ELEM *)s_srf->msc_p[2];

    /* no custom clippers or
     * surface itself has non-trivial transform */
#if RT_OPTS_ADJUST != 0
    if (elm == RT_NULL || trnode == this
    || (rg->opts & RT_OPTS_ADJUST) == 0)
#endif /* RT_OPTS_ADJUST */
    {
        /* calculate bbox and cbox based on 
         * original axis clippers and surface shape */
        recalc_minmax(RT_NULL,   RT_NULL,
                      shp->bmin, shp->bmax,
                      shp->cmin, shp->cmax);
        return;
    }

    rt_cell skip = 0;

    /* run through custom clippers list */
    for (; elm != RT_NULL; elm = elm->next)
    {
        rt_Object *obj = elm->temp == RT_NULL ? RT_NULL :
                         (rt_Object *)((rt_BOUND *)elm->temp)->obj;

        /* skip clip accum segments in the list */
        if (obj == RT_NULL)
        {
            skip = 1 - skip;
        }

        if (obj == RT_NULL || skip == 1
        ||  obj->tag == RT_TAG_ARRAY
        ||  obj->tag == RT_TAG_PLANE
        ||  obj->trnode != trnode
        ||  elm->data != RT_REL_MINUS_OUTER)
        {
            continue;
        }

        srf_changed |= obj->obj_changed;
    }

    if (srf_changed == 0)
    {
        return;
    }

    /* first calculate only bbox based on 
     * original axis clippers and surface shape */
    recalc_minmax(RT_NULL,   RT_NULL,
                  shp->bmin, shp->bmax,
                  RT_NULL,   RT_NULL);

    /* prepare cbox as temporary storage
     * for bbox adjustments by custom clippers */
    RT_VEC3_SET_VAL1(shp->cmin, -RT_INF);
    RT_VEC3_SET_VAL1(shp->cmax, +RT_INF);

    /* reinit custom clippers list */
    elm = (rt_ELEM *)s_srf->msc_p[2];

    skip = 0;

    /* run through custom clippers list */
    for (; elm != RT_NULL; elm = elm->next)
    {
        rt_Object *obj = elm->temp == RT_NULL ? RT_NULL :
                         (rt_Object *)((rt_BOUND *)elm->temp)->obj;

        /* skip clip accum segments in the list */
        if (obj == RT_NULL)
        {
            skip = 1 - skip;
        }

        if (obj == RT_NULL || skip == 1
        ||  obj->tag == RT_TAG_ARRAY
        ||  obj->tag == RT_TAG_PLANE
        ||  obj->trnode != trnode
        ||  elm->data != RT_REL_MINUS_OUTER)
        {
            continue;
        }

        rt_Surface *srf = (rt_Surface *)obj;

        /* accumulate bbox adjustments
         * from individual outer clippers into cbox */
        srf->recalc_minmax(shp->bmin, shp->bmax,
                           RT_NULL,   RT_NULL,
                           shp->cmin, shp->cmax);
    }

    /* apply bbox adjustments accumulated in cbox,
     * calculate final bbox and cbox for the surface */
    recalc_minmax(shp->cmin, shp->cmax,
                  shp->bmin, shp->bmax,
                  shp->cmin, shp->cmax);
}

/*
 * Update bounding box and volume along with SIMD fields.
 */
rt_void rt_Surface::update_bounds()
{
    update_minmax();

    if (srf_changed == 0)
    {
        return;
    }

    if (box->verts_num != 0)
    {
        update_bbgeom(box);
    }

    s_srf->min_t[RT_X] = shp->cmin[RT_X] == -RT_INF ? 0 : 1;
    s_srf->min_t[RT_Y] = shp->cmin[RT_Y] == -RT_INF ? 0 : 1;
    s_srf->min_t[RT_Z] = shp->cmin[RT_Z] == -RT_INF ? 0 : 1;

    s_srf->max_t[RT_X] = shp->cmax[RT_X] == +RT_INF ? 0 : 1;
    s_srf->max_t[RT_Y] = shp->cmax[RT_Y] == +RT_INF ? 0 : 1;
    s_srf->max_t[RT_Z] = shp->cmax[RT_Z] == +RT_INF ? 0 : 1;

    rt_vec4  zro = {0.0f, 0.0f, 0.0f, 0.0f};
    rt_real *pps = trnode == this ? zro : pos;

    RT_SIMD_SET(s_srf->min_x, shp->bmin[RT_X] - pps[RT_X]);
    RT_SIMD_SET(s_srf->min_y, shp->bmin[RT_Y] - pps[RT_Y]);
    RT_SIMD_SET(s_srf->min_z, shp->bmin[RT_Z] - pps[RT_Z]);

    RT_SIMD_SET(s_srf->max_x, shp->bmax[RT_X] - pps[RT_X]);
    RT_SIMD_SET(s_srf->max_y, shp->bmax[RT_Y] - pps[RT_Y]);
    RT_SIMD_SET(s_srf->max_z, shp->bmax[RT_Z] - pps[RT_Z]);
}

/*
 * Destroy surface object.
 */
rt_Surface::~rt_Surface()
{
    delete outer;
    delete inner;
}

/******************************************************************************/
/**********************************   PLANE   *********************************/
/******************************************************************************/

/*
 * Instantiate plane surface object.
 */
rt_Plane::rt_Plane(rt_Registry *rg, rt_Object *parent,
                   rt_OBJECT *obj, rt_cell ssize) :

    rt_Surface(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_PLANE)))
{
    this->xpl = (rt_PLANE *)obj->obj.pobj;

    if (srf->min[RT_I] == -RT_INF
    ||  srf->min[RT_J] == -RT_INF
    ||  srf->max[RT_I] == +RT_INF
    ||  srf->max[RT_J] == +RT_INF)
    {
        shp->verts_num = 0;
        shp->verts = RT_NULL;

        shp->edges_num = 0;
        shp->edges = RT_NULL;

        shp->faces_num = 0;
        shp->faces = RT_NULL;
    }
    else
    {
        shp->verts_num = 4;
        shp->verts = (rt_VERT *)
                     rg->alloc(shp->verts_num * sizeof(rt_VERT), RT_ALIGN);

        shp->edges_num = 4;
        shp->edges = (rt_EDGE *)
                     rg->alloc(shp->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(shp->edges, bx_edges, shp->edges_num * sizeof(rt_EDGE));

        shp->faces_num = 1;
        shp->faces = (rt_FACE *)
                     rg->alloc(shp->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(shp->faces, bx_faces, shp->faces_num * sizeof(rt_FACE));
    }

/*  rt_SIMD_PLANE */

    rt_SIMD_PLANE *s_xpl = (rt_SIMD_PLANE *)s_srf;

    RT_SIMD_SET(s_xpl->nrm_k, +1.0f);
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Plane::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Surface::update_fields();

    RT_VEC3_SET_VAL1(shp->sci, 0.0f);
    shp->sci[RT_W] = 0.0f;

    RT_VEC3_SET_VAL1(shp->scj, 0.0f);
    shp->scj[RT_W] = 0.0f;

    RT_VEC3_SET_VAL1(shp->sck, 0.0f);
    shp->sck[RT_W] = 0.0f;

    shp->sck[mp_k] = (rt_real)sgn[RT_K];
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Plane::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Surface::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        bmin[RT_I] = smin[RT_I];
        bmin[RT_J] = smin[RT_J];
        bmin[RT_K] = 0.0f;

        bmax[RT_I] = smax[RT_I];
        bmax[RT_J] = smax[RT_J];
        bmax[RT_K] = 0.0f;
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_K] = -RT_INF;

        cmax[RT_K] = +RT_INF;
    }
}

/*
 * Destroy plane surface object.
 */
rt_Plane::~rt_Plane()
{

}

/******************************************************************************/
/********************************   QUADRIC   *********************************/
/******************************************************************************/

/*
 * Instantiate quadric surface object.
 */
rt_Quadric::rt_Quadric(rt_Registry *rg, rt_Object *parent,
                       rt_OBJECT *obj, rt_cell ssize) :

    rt_Surface(rg, parent, obj, ssize)
{

}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Quadric::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Surface::update_fields();

    RT_VEC3_SET_VAL1(shp->sci, 1.0f);
    shp->sci[RT_W] = 0.0f;

    RT_VEC3_SET_VAL1(shp->scj, 0.0f);
    shp->scj[RT_W] = 0.0f;

    RT_VEC3_SET_VAL1(shp->sck, 0.0f);
    shp->sck[RT_W] = 0.0f;
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Quadric::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                  rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                  rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Surface::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);
}

/*
 * Destroy quadric surface object.
 */
rt_Quadric::~rt_Quadric()
{

}

/******************************************************************************/
/********************************   CYLINDER   ********************************/
/******************************************************************************/

/*
 * Instantiate cylinder surface object.
 */
rt_Cylinder::rt_Cylinder(rt_Registry *rg, rt_Object *parent,
                         rt_OBJECT *obj, rt_cell ssize) :

    rt_Quadric(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_CYLINDER)))
{
    this->xcl = (rt_CYLINDER *)obj->obj.pobj;

    if (srf->min[RT_K] == -RT_INF
    ||  srf->max[RT_K] == +RT_INF)
    {
        shp->verts_num = 0;
        shp->verts = RT_NULL;

        shp->edges_num = 0;
        shp->edges = RT_NULL;

        shp->faces_num = 0;
        shp->faces = RT_NULL;
    }
    else
    {
        shp->verts_num = 8;
        shp->verts = (rt_VERT *)
                     rg->alloc(shp->verts_num * sizeof(rt_VERT), RT_ALIGN);

        shp->edges_num = RT_ARR_SIZE(bx_edges);
        shp->edges = (rt_EDGE *)
                     rg->alloc(shp->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(shp->edges, bx_edges, shp->edges_num * sizeof(rt_EDGE));

        shp->faces_num = RT_ARR_SIZE(bx_faces);
        shp->faces = (rt_FACE *)
                     rg->alloc(shp->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(shp->faces, bx_faces, shp->faces_num * sizeof(rt_FACE));
    }

/*  rt_SIMD_CYLINDER */

    rt_SIMD_CYLINDER *s_xcl = (rt_SIMD_CYLINDER *)s_srf;

    rt_real rad = RT_FABS(xcl->rad);

    RT_SIMD_SET(s_xcl->rad_2, rad * rad);
    RT_SIMD_SET(s_xcl->i_rad, 1.0f / rad);
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Cylinder::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Quadric::update_fields();

    shp->sci[mp_k] = 0.0f;
    shp->sci[RT_W] = xcl->rad * xcl->rad;
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Cylinder::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                   rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                   rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Quadric::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);

    rt_real rad = RT_FABS(xcl->rad);

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        bmin[RT_I] = RT_MAX(smin[RT_I], -rad);
        bmin[RT_J] = RT_MAX(smin[RT_J], -rad);
        bmin[RT_K] = smin[RT_K];

        bmax[RT_I] = RT_MIN(smax[RT_I], +rad);
        bmax[RT_J] = RT_MIN(smax[RT_J], +rad);
        bmax[RT_K] = smax[RT_K];
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_I] = cmin[RT_I] <= -rad ? -RT_INF : cmin[RT_I];
        cmin[RT_J] = cmin[RT_J] <= -rad ? -RT_INF : cmin[RT_J];

        cmax[RT_I] = cmax[RT_I] >= +rad ? +RT_INF : cmax[RT_I];
        cmax[RT_J] = cmax[RT_J] >= +rad ? +RT_INF : cmax[RT_J];
    }
}

/*
 * Destroy cylinder surface object.
 */
rt_Cylinder::~rt_Cylinder()
{

}

/******************************************************************************/
/*********************************   SPHERE   *********************************/
/******************************************************************************/

/*
 * Instantiate sphere surface object.
 */
rt_Sphere::rt_Sphere(rt_Registry *rg, rt_Object *parent,
                     rt_OBJECT *obj, rt_cell ssize) :

    rt_Quadric(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_SPHERE)))
{
    this->xsp = (rt_SPHERE *)obj->obj.pobj;

    if (RT_TRUE)
    {
        shp->verts_num = 8;
        shp->verts = (rt_VERT *)
                     rg->alloc(shp->verts_num * sizeof(rt_VERT), RT_ALIGN);

        shp->edges_num = RT_ARR_SIZE(bx_edges);
        shp->edges = (rt_EDGE *)
                     rg->alloc(shp->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(shp->edges, bx_edges, shp->edges_num * sizeof(rt_EDGE));

        shp->faces_num = RT_ARR_SIZE(bx_faces);
        shp->faces = (rt_FACE *)
                     rg->alloc(shp->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(shp->faces, bx_faces, shp->faces_num * sizeof(rt_FACE));
    }

/*  rt_SIMD_SPHERE */

    rt_SIMD_SPHERE *s_xsp = (rt_SIMD_SPHERE *)s_srf;

    rt_real rad = RT_FABS(xsp->rad);

    RT_SIMD_SET(s_xsp->rad_2, rad * rad);
    RT_SIMD_SET(s_xsp->i_rad, 1.0f / rad);
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Sphere::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Quadric::update_fields();

    shp->sci[RT_W] = xsp->rad * xsp->rad;
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Sphere::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                 rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                 rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Quadric::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);

    rt_real top, r = RT_FABS(xsp->rad);
    rt_real rad[3] = {r, r, r};
    rt_cell i, j, k;

    for (k = 0; k < 3; k++)
    {
        top = smin[k] > 0.0f ? +smin[k] : smax[k] < 0.0f ? -smax[k] : 0.0f;
        r = RT_SQRT(RT_MAX(xsp->rad * xsp->rad - top * top, 0.0f));

        i = (k + 1) % 3;
        if (rad[i] > r)
        {
            rad[i] = r;
        }

        j = (k + 2) % 3;
        if (rad[j] > r)
        {
            rad[j] = r;
        }
    }

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        bmin[RT_I] = RT_MAX(smin[RT_I], -rad[RT_I]);
        bmin[RT_J] = RT_MAX(smin[RT_J], -rad[RT_J]);
        bmin[RT_K] = RT_MAX(smin[RT_K], -rad[RT_K]);

        bmax[RT_I] = RT_MIN(smax[RT_I], +rad[RT_I]);
        bmax[RT_J] = RT_MIN(smax[RT_J], +rad[RT_J]);
        bmax[RT_K] = RT_MIN(smax[RT_K], +rad[RT_K]);
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_I] = cmin[RT_I] <= -rad[RT_I] ? -RT_INF : cmin[RT_I];
        cmin[RT_J] = cmin[RT_J] <= -rad[RT_J] ? -RT_INF : cmin[RT_J];
        cmin[RT_K] = cmin[RT_K] <= -rad[RT_K] ? -RT_INF : cmin[RT_K];

        cmax[RT_I] = cmax[RT_I] >= +rad[RT_I] ? +RT_INF : cmax[RT_I];
        cmax[RT_J] = cmax[RT_J] >= +rad[RT_J] ? +RT_INF : cmax[RT_J];
        cmax[RT_K] = cmax[RT_K] >= +rad[RT_K] ? +RT_INF : cmax[RT_K];
    }
}

/*
 * Destroy sphere surface object.
 */
rt_Sphere::~rt_Sphere()
{

}

/******************************************************************************/
/**********************************   CONE   **********************************/
/******************************************************************************/

/*
 * Instantiate cone surface object.
 */
rt_Cone::rt_Cone(rt_Registry *rg, rt_Object *parent,
                 rt_OBJECT *obj, rt_cell ssize) :

    rt_Quadric(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_CONE)))
{
    this->xcn = (rt_CONE *)obj->obj.pobj;

    if (srf->min[RT_K] == -RT_INF
    ||  srf->max[RT_K] == +RT_INF)
    {
        shp->verts_num = 0;
        shp->verts = RT_NULL;

        shp->edges_num = 0;
        shp->edges = RT_NULL;

        shp->faces_num = 0;
        shp->faces = RT_NULL;
    }
    else
    {
        shp->verts_num = 8;
        shp->verts = (rt_VERT *)
                     rg->alloc(shp->verts_num * sizeof(rt_VERT), RT_ALIGN);

        shp->edges_num = RT_ARR_SIZE(bx_edges);
        shp->edges = (rt_EDGE *)
                     rg->alloc(shp->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(shp->edges, bx_edges, shp->edges_num * sizeof(rt_EDGE));

        shp->faces_num = RT_ARR_SIZE(bx_faces);
        shp->faces = (rt_FACE *)
                     rg->alloc(shp->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(shp->faces, bx_faces, shp->faces_num * sizeof(rt_FACE));
    }

/*  rt_SIMD_CONE */

    rt_SIMD_CONE *s_xcn = (rt_SIMD_CONE *)s_srf;

    rt_real rat = RT_FABS(xcn->rat);

    RT_SIMD_SET(s_xcn->rat_2, rat * rat);
    RT_SIMD_SET(s_xcn->i_rat, 1.0f / (rat * RT_SQRT(rat * rat + 1.0f)));
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Cone::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Quadric::update_fields();

    shp->sci[mp_k] = -(xcn->rat * xcn->rat);
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Cone::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                               rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                               rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Quadric::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);

    rt_real top = RT_MAX(RT_FABS(smin[RT_K]), RT_FABS(smax[RT_K]));
    rt_real rad = top * RT_FABS(xcn->rat);

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        bmin[RT_I] = RT_MAX(smin[RT_I], -rad);
        bmin[RT_J] = RT_MAX(smin[RT_J], -rad);
        bmin[RT_K] = smin[RT_K];

        bmax[RT_I] = RT_MIN(smax[RT_I], +rad);
        bmax[RT_J] = RT_MIN(smax[RT_J], +rad);
        bmax[RT_K] = smax[RT_K];
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_I] = cmin[RT_I] <= -rad ? -RT_INF : cmin[RT_I];
        cmin[RT_J] = cmin[RT_J] <= -rad ? -RT_INF : cmin[RT_J];

        cmax[RT_I] = cmax[RT_I] >= +rad ? +RT_INF : cmax[RT_I];
        cmax[RT_J] = cmax[RT_J] >= +rad ? +RT_INF : cmax[RT_J];
    }
}

/*
 * Destroy cone surface object.
 */
rt_Cone::~rt_Cone()
{

}

/******************************************************************************/
/*******************************   PARABOLOID   *******************************/
/******************************************************************************/

/*
 * Instantiate paraboloid surface object.
 */
rt_Paraboloid::rt_Paraboloid(rt_Registry *rg, rt_Object *parent,
                             rt_OBJECT *obj, rt_cell ssize) :

    rt_Quadric(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_PARABOLOID)))
{
    this->xpb = (rt_PARABOLOID *)obj->obj.pobj;

    if (srf->min[RT_K] == -RT_INF && xpb->par < 0.0f
    ||  srf->max[RT_K] == +RT_INF && xpb->par > 0.0f)
    {
        shp->verts_num = 0;
        shp->verts = RT_NULL;

        shp->edges_num = 0;
        shp->edges = RT_NULL;

        shp->faces_num = 0;
        shp->faces = RT_NULL;
    }
    else
    {
        shp->verts_num = 8;
        shp->verts = (rt_VERT *)
                     rg->alloc(shp->verts_num * sizeof(rt_VERT), RT_ALIGN);

        shp->edges_num = RT_ARR_SIZE(bx_edges);
        shp->edges = (rt_EDGE *)
                     rg->alloc(shp->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(shp->edges, bx_edges, shp->edges_num * sizeof(rt_EDGE));

        shp->faces_num = RT_ARR_SIZE(bx_faces);
        shp->faces = (rt_FACE *)
                     rg->alloc(shp->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(shp->faces, bx_faces, shp->faces_num * sizeof(rt_FACE));
    }

/*  rt_SIMD_PARABOLOID */

    rt_SIMD_PARABOLOID *s_xpb = (rt_SIMD_PARABOLOID *)s_srf;

    rt_real par = xpb->par;

    RT_SIMD_SET(s_xpb->par_2, par / 2.0f);
    RT_SIMD_SET(s_xpb->i_par, par * par / 4.0f);
    RT_SIMD_SET(s_xpb->par_k, par);
    RT_SIMD_SET(s_xpb->one_k, 1.0f);
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Paraboloid::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Quadric::update_fields();

    shp->sci[mp_k] = 0.0f;
    shp->scj[mp_k] = xpb->par * (rt_real)sgn[RT_K];
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Paraboloid::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                     rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                     rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Quadric::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);

    rt_real par = xpb->par;
    rt_real top = RT_MAX(par < 0.0f ? -smin[RT_K] : +smax[RT_K], 0.0f);
    rt_real rad = RT_SQRT(top * RT_FABS(par));

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        bmin[RT_I] = RT_MAX(smin[RT_I], -rad);
        bmin[RT_J] = RT_MAX(smin[RT_J], -rad);
        bmin[RT_K] = smin[RT_K] <= 0.0f &&
                             par > 0.0f ? 0.0f : smin[RT_K];

        bmax[RT_I] = RT_MIN(smax[RT_I], +rad);
        bmax[RT_J] = RT_MIN(smax[RT_J], +rad);
        bmax[RT_K] = smax[RT_K] >= 0.0f && 
                             par < 0.0f ? 0.0f : smax[RT_K];
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_I] = cmin[RT_I] <= -rad ? -RT_INF : cmin[RT_I];
        cmin[RT_J] = cmin[RT_J] <= -rad ? -RT_INF : cmin[RT_J];
        cmin[RT_K] = cmin[RT_K] <= 0.0f &&
                             par > 0.0f ? -RT_INF : cmin[RT_K];

        cmax[RT_I] = cmax[RT_I] >= +rad ? +RT_INF : cmax[RT_I];
        cmax[RT_J] = cmax[RT_J] >= +rad ? +RT_INF : cmax[RT_J];
        cmax[RT_K] = cmax[RT_K] >= 0.0f &&
                             par < 0.0f ? +RT_INF : cmax[RT_K];
    }
}

/*
 * Destroy paraboloid surface object.
 */
rt_Paraboloid::~rt_Paraboloid()
{

}

/******************************************************************************/
/*******************************   HYPERBOLOID   ******************************/
/******************************************************************************/

/*
 * Instantiate hyperboloid surface object.
 */
rt_Hyperboloid::rt_Hyperboloid(rt_Registry *rg, rt_Object *parent,
                               rt_OBJECT *obj, rt_cell ssize) :

    rt_Quadric(rg, parent, obj, RT_MAX(ssize, sizeof(rt_SIMD_HYPERBOLOID)))
{
    this->xhb = (rt_HYPERBOLOID *)obj->obj.pobj;

    if (srf->min[RT_K] == -RT_INF
    ||  srf->max[RT_K] == +RT_INF)
    {
        shp->verts_num = 0;
        shp->verts = RT_NULL;

        shp->edges_num = 0;
        shp->edges = RT_NULL;

        shp->faces_num = 0;
        shp->faces = RT_NULL;
    }
    else
    {
        shp->verts_num = 8;
        shp->verts = (rt_VERT *)
                     rg->alloc(shp->verts_num * sizeof(rt_VERT), RT_ALIGN);

        shp->edges_num = RT_ARR_SIZE(bx_edges);
        shp->edges = (rt_EDGE *)
                     rg->alloc(shp->edges_num * sizeof(rt_EDGE), RT_ALIGN);
        memcpy(shp->edges, bx_edges, shp->edges_num * sizeof(rt_EDGE));

        shp->faces_num = RT_ARR_SIZE(bx_faces);
        shp->faces = (rt_FACE *)
                     rg->alloc(shp->faces_num * sizeof(rt_FACE), RT_ALIGN);
        memcpy(shp->faces, bx_faces, shp->faces_num * sizeof(rt_FACE));
    }

/*  rt_SIMD_HYPERBOLOID */

    rt_SIMD_HYPERBOLOID *s_xhb = (rt_SIMD_HYPERBOLOID *)s_srf;

    rt_real rat = xhb->rat;
    rt_real hyp = xhb->hyp;

    RT_SIMD_SET(s_xhb->rat_2, rat * rat);
    RT_SIMD_SET(s_xhb->i_rat, (1.0f + rat * rat) * rat * rat);
    RT_SIMD_SET(s_xhb->hyp_k, hyp);
    RT_SIMD_SET(s_xhb->one_k, 1.0f);
}

/*
 * Update SIMD and other data fields.
 */
rt_void rt_Hyperboloid::update_fields()
{
    if (obj_changed == 0)
    {
        return;
    }

    rt_Quadric::update_fields();

    shp->sci[mp_k] = -(xhb->rat * xhb->rat);
    shp->sci[RT_W] = xhb->hyp;
}

/*
 * Adjust local space bounding and clipping boxes according to surface shape.
 */
rt_void rt_Hyperboloid::adjust_minmax(rt_vec4 smin, rt_vec4 smax, /* src */
                                      rt_vec4 bmin, rt_vec4 bmax, /* bbox */
                                      rt_vec4 cmin, rt_vec4 cmax) /* cbox */
{
    rt_Quadric::adjust_minmax(smin, smax, bmin, bmax, cmin, cmax);

    rt_real top = RT_MAX(RT_FABS(smin[RT_K]), RT_FABS(smax[RT_K]));
    rt_real rad = RT_SQRT(top * top * xhb->rat * xhb->rat + xhb->hyp);

    if (bmin != RT_NULL && bmax != RT_NULL)
    {
        bmin[RT_I] = RT_MAX(smin[RT_I], -rad);
        bmin[RT_J] = RT_MAX(smin[RT_J], -rad);
        bmin[RT_K] = smin[RT_K];

        bmax[RT_I] = RT_MIN(smax[RT_I], +rad);
        bmax[RT_J] = RT_MIN(smax[RT_J], +rad);
        bmax[RT_K] = smax[RT_K];
    }

    if (cmin != RT_NULL && cmax != RT_NULL)
    {
        cmin[RT_I] = cmin[RT_I] <= -rad ? -RT_INF : cmin[RT_I];
        cmin[RT_J] = cmin[RT_J] <= -rad ? -RT_INF : cmin[RT_J];

        cmax[RT_I] = cmax[RT_I] >= +rad ? +RT_INF : cmax[RT_I];
        cmax[RT_J] = cmax[RT_J] >= +rad ? +RT_INF : cmax[RT_J];
    }
}

/*
 * Destroy hyperboloid surface object.
 */
rt_Hyperboloid::~rt_Hyperboloid()
{

}

/******************************************************************************/
/********************************   MATERIAL   ********************************/
/******************************************************************************/

/*
 * Instantiate texture to keep track of loaded textures.
 */
rt_Texture::rt_Texture(rt_Registry *rg, rt_pstr name) :

    rt_List<rt_Texture>(rg->get_tex())
{
    rg->put_tex(this);

    this->name = name;

    load_image(rg, name, &tex);
}

/*
 * Destroy texture.
 */
rt_Texture::~rt_Texture()
{

}

/*
 * Instantiate material.
 */
rt_Material::rt_Material(rt_Registry *rg, rt_SIDE *sd, rt_MATERIAL *mat) :

    rt_List<rt_Material>(rg->get_mat())
{
    if (mat == RT_NULL)
    {
        throw rt_Exception("null-pointer in material");
    }

    rg->put_mat(this);

    this->mat = mat;
    otx.x_dim = otx.y_dim = -1;
    /* save original texture data */
    if (mat->tex.x_dim == 0 && mat->tex.y_dim == 0)
    {
        otx = mat->tex;
    }
    resolve_texture(rg);
    rt_TEX *tx = &mat->tex;

    props  = 0;
    props |= tx->x_dim == 1 && tx->y_dim == 1 ? 0 : RT_PROP_TEXTURE;
    props |= mat->prp[0] == 0.0f ? 0 : RT_PROP_REFLECT;
    props |= mat->prp[2] == 1.0f ? 0 : RT_PROP_REFRACT;
    props |= mat->lgt[1] == 0.0f ? 0 : RT_PROP_SPECULAR;
    props |= mat->prp[1] == 0.0f ? RT_PROP_OPAQUE : 0;
    props |= mat->prp[1] == 1.0f ? RT_PROP_TRANSP : 0;
    props |= mat->tag == RT_MAT_LIGHT ? RT_PROP_LIGHT : RT_PROP_NORMAL;
    props |= mat->tag == RT_MAT_METAL ? RT_PROP_METAL : 0;

    mtx[0][0] = +RT_COSA(sd->rot);
    mtx[0][1] = +RT_SINA(sd->rot);
    mtx[1][0] = -RT_SINA(sd->rot);
    mtx[1][1] = +RT_COSA(sd->rot);

    rt_cell map[2], sgn[2];
    rt_cell match = 0;

    rt_cell i, j;

    for (i = 0; i < 2; i++)
    {
        for (j = 0; j < 2; j++)
        {
            if (RT_FABS(this->mtx[i][0]) == iden4[j][0]
            &&  RT_FABS(this->mtx[i][1]) == iden4[j][1])
            {
                map[i] = j;
                sgn[i] = RT_SIGN(this->mtx[i][j]);
                match++;
            }
        }
    }

    if (match < 2)
    {
        map[RT_X] = RT_U;
        sgn[RT_X] = 1;

        map[RT_Y] = RT_V;
        sgn[RT_Y] = 1;
    }

/*  rt_SIMD_MATERIAL */

    s_mat = (rt_SIMD_MATERIAL *)
            rg->alloc(sizeof(rt_SIMD_MATERIAL),
                                RT_SIMD_ALIGN);

    s_mat->t_map[RT_X] = map[RT_X] * RT_SIMD_WIDTH * 4;
    s_mat->t_map[RT_Y] = map[RT_Y] * RT_SIMD_WIDTH * 4;
    s_mat->t_map[2] = 0;
    s_mat->t_map[3] = 0;

    RT_SIMD_SET(s_mat->xscal, tx->x_dim / sd->scl[RT_X] * sgn[RT_X]);
    RT_SIMD_SET(s_mat->yscal, tx->y_dim / sd->scl[RT_Y] * sgn[RT_Y]);

    RT_SIMD_SET(s_mat->xoffs, sd->pos[map[RT_X]]);
    RT_SIMD_SET(s_mat->yoffs, sd->pos[map[RT_Y]]);

    rt_word x_mask = tx->x_dim - 1;
    rt_word y_mask = tx->y_dim - 1;

    RT_SIMD_SET(s_mat->xmask, x_mask);
    RT_SIMD_SET(s_mat->ymask, y_mask);

    rt_cell x_dim = tx->x_dim;
    rt_cell x_lg2 = 0;
    while (x_dim >>= 1)
    {
        x_lg2++;
    }

    RT_SIMD_SET(s_mat->yshft, 0);
    s_mat->yshft[0] = x_lg2;

    RT_SIMD_SET(s_mat->tex_p, tx->ptex);
    RT_SIMD_SET(s_mat->cmask, 0xFF);

    RT_SIMD_SET(s_mat->l_dff, mat->lgt[0]);
    RT_SIMD_SET(s_mat->l_spc, mat->lgt[1]);
    RT_SIMD_SET(s_mat->l_pow, (rt_word)mat->lgt[2]);
    RT_SIMD_SET(s_mat->pow_p, RT_NULL);

    RT_SIMD_SET(s_mat->c_rfl, mat->prp[0]);
    RT_SIMD_SET(s_mat->c_trn, mat->prp[1]);
    RT_SIMD_SET(s_mat->c_rfr, mat->prp[2]);

    RT_SIMD_SET(s_mat->rfr_2, mat->prp[2] * mat->prp[2]);
    RT_SIMD_SET(s_mat->c_one, 1.0f);
}

/*
 * Validate texture fields by checking whether
 * texture color was defined in place,
 * texture data needs to be loaded from external file or
 * texture data was bound from local array.
 */
rt_void rt_Material::resolve_texture(rt_Registry *rg)
{
    rt_TEX *tx = &mat->tex;

    /* texture color is defined in place */
    if (tx->x_dim == 0 && tx->y_dim == 0 && tx->ptex == RT_NULL)
    {
        tx->ptex  = &tx->col.val;
        tx->x_dim = 1;
        tx->y_dim = 1;
    }

    /* texture load is requested */
    if (tx->x_dim == 0 && tx->y_dim == 0 && tx->ptex != RT_NULL)
    {
        rt_pstr name = (rt_pstr)tx->ptex;
        rt_Texture *tex = RT_NULL;

        /* traverse list of loaded textures (slow, implement hashmap later)
         * and check if requested texture already exists */
        for (tex = rg->get_tex(); tex != RT_NULL; tex = tex->next)
        {
            if (strcmp(name, tex->name) == 0)
            {
                break;
            }
        }

        if (tex == RT_NULL)
        {
            tex = new rt_Texture(rg, name);
        }

        *tx = tex->tex;
    }

    /* texture bind doesn't need extra validation */
}

/*
 * Destroy material.
 */
rt_Material::~rt_Material()
{
    /* restore original texture data */
    if (otx.x_dim == 0 && otx.y_dim == 0)
    {
        mat->tex = otx;
    }
}

/******************************************************************************/
/******************************************************************************/
/******************************************************************************/
