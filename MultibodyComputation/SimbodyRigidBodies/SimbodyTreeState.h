#ifndef SIMTK_SIMBODY_TREE_STATE_H_
#define SIMTK_SIMBODY_TREE_STATE_H_

#include "simbody/Simbody.h"

#include <cassert>
#include <vector>

namespace simtk {


class SimbodyTreeResults {
public:
    SimbodyTreeResults() : stage(UninitializedStage) { } // everything has length 0
    // default copy, copy assign, destruct

    // This allocation routine should be called by realizeModeling(). Before that
    // we don't know enough about what to put here.

    // nDofs==nu==#joint forces
    // nSqDofs=sum(nu[j]^2) for all joints j
    void allocateCache(int nBodies, int nDofs, int nSqDofs, int maxNQs,
                       int npc, int nvc, int nac) // pos, vel, acc constraints
    {
        assert(stage >= BuiltStage);
        stage = BuiltStage; // roll back if necessary

        // These contain uninitialized junk.
        bodyConfigInParent.resize(nBodies);
        bodyConfigInGround.resize(nBodies);
        bodySpatialInertia.resize(nBodies);
        positionConstraintErrors.resize(npc);
        storageForHt.resize(2,nDofs);
        bodyVelocityInParent.resize(nBodies);
        bodyVelocityInGround.resize(nBodies);
        velocityConstraintErrors.resize(nvc);
        qdot.resize(maxNQs);
        articulatedBodyInertia.resize(nBodies);
        bodyAccelerationInGround.resize(nBodies);
        coriolisForces.resize(nBodies);
        udot.resize(nDofs);
        lambda.resize(nac);
        accelerationConstraintErrors.resize(nac);
        netHingeForces.resize(nDofs);
        qdotdot.resize(maxNQs);
        storageForDI.resize(nSqDofs);
        storageForG.resize(2,nDofs);
        nu.resize(nDofs);
        epsilon.resize(nDofs);
    }

    SBStage stage;     // must be kept up to date by State changes

    // TODO: constraint runtimes

    // TODO: Modeling
    //   counts of various things resulting from modeling choices,
    //   constraint enabling, prescribed motion

    // TODO: Parameters
    //   body mass props; particle masses
    //   X_BJ, X_PJi transforms
    //   distance constraint distances & station positions

    // Configuration
    std::vector<TransformMat> bodyConfigInParent; // nb (joint config)
    std::vector<TransformMat> bodyConfigInGround; // nb
    Vector_<SpatialMat>       bodySpatialInertia; // nb

    Vector positionConstraintErrors;              // npc
    Matrix_<Vec3> storageForHt;                   // 2 x ndof

    // Motion
    Vector_<SpatialVec> bodyVelocityInParent;     // nb (joint velocity)
    Vector_<SpatialVec> bodyVelocityInGround;     // nb

    Vector velocityConstraintErrors;              // nvc
    Vector qdot;                                  // nq

    // Dynamics
    Vector_<SpatialMat> articulatedBodyInertia;   // nb (P)
    Vector_<SpatialVec> bodyAccelerationInGround; // nb
    Vector_<SpatialVec> coriolisForces;           // nb (& gyroscopic, Pa+b)

    Vector udot;                                  // nu
    Vector lambda;                                // nac
    Vector accelerationConstraintErrors;          // nac
    Vector netHingeForces;                        // nu (T-(~Am+R(F+C))
    Vector qdotdot;                               // nq

    // dynamic temporaries
    Vector_<Real>       storageForDI;   // sum(nu[j]^2)
    Matrix_<Vec3>       storageForG;    // 2 X ndof
    Vector              nu;
    Vector              epsilon;

};

/** 
 * Generalized state variable collection for a SimbodyMultibodyTree. 
 * Variables are divided into Stages, according to when their values
 * are needed during a calculation. The Stages that matter to the
 * MultibodyTree are:
 *       (Construction: not part of the state)
 *     Modeling:        choice of coordinates, knowns & unknowns, methods, etc.
 *     Parametrization: setting of physical parameters, e.g. mass
 *       (Time: not relevant to MultibodyTree)
 *     Configuration:   position and orientation values (2nd order continuous)
 *     Velocity:        rates
 *     Dynamics:        forces & prescribed accelerations   
 */
class SimbodyTreeVariables {
public:
    SimbodyTreeVariables() : useEulerAngles(false) { }

    // nDofs==nu==#joint forces

    // Call this after realizeConstruction(). These are the variables we need
    // to specify our modeling choices. We can't allocate the rest until we see
    // how we'll be modeling.
    void allocateModelingVars(int nBodies, int nConstraints) {
        useEulerAngles = false;
        prescribed.resize(nBodies);         prescribed.assign(nBodies,false);
        enabled.resize(nConstraints);       enabled.assign(nConstraints,false);
    }

    // Call this after realizeModeling(). We now know everything we need to know
    // to allocate and initialize the remaining state variables.
    void allocateAllVars(int nDofs, int maxNQs, int nac) // acc constraints
    {
        const int nBodies      = prescribed.size();
        const int nConstraints = enabled.size();
        q.resize(maxNQs);                   q.setToNaN();
        u.resize(nDofs);                    u.setToNaN();
        appliedBodyForces.resize(nBodies);  appliedBodyForces.setToNaN();
        appliedJointForces.resize(nDofs);   appliedJointForces.setToNaN();
        prescribedUdot.resize(nDofs);       prescribedUdot.setToNaN();
    }

    void setVelocitiesToZero() {
        u.setToZero();
    }

    void clearForces() {
        appliedBodyForces.setToZero();
        appliedJointForces.setToZero();
    }

    // This locks all the joints that are prescribed.
    void setPrescribedAccelerationsToZero() {
        prescribedUdot.setToZero();
    }

    // Modeling
    bool              useEulerAngles;
    std::vector<bool> prescribed;           // nb
    std::vector<bool> enabled;              // nac

    // Parametrization
    // TODO: body masses, etc.

    // Configuration
    Vector q;                               // nq

    // Motion
    Vector u;                               // nu

    // Dynamics
    Vector_<SpatialVec> appliedBodyForces;  // nb
    Vector              appliedJointForces; // nu
    Vector              prescribedUdot;     // nu

};

}; // namespace simtk

#endif // SIMTK_SIMBODY_TREE_STATE_H_
