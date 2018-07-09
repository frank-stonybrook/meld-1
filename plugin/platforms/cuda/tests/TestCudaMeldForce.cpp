/*
   Copyright 2015 by Justin MacCallum, Alberto Perez, Ken Dill
   All rights reserved
*/


#include "MeldForce.h"
#include "openmm/internal/AssertionUtilities.h"
#include "openmm/Context.h"
//Cong added
//#include "openmm/cuda/CudaContext.h"
//Cong added end
#include "openmm/Platform.h"
#include "openmm/System.h"
#include "openmm/VerletIntegrator.h"
#include <cmath>
#include <iostream>
#include <vector>

using namespace MeldPlugin;
using namespace OpenMM;
using namespace std;

extern "C" OPENMM_EXPORT void registerMeldCudaKernelFactories();

// Cong added tess
void testPeriodic() {

// setup system
Vec3 vx(10,0,0);
Vec3 vy(0,10,0);
Vec3 vz(0,0,10);
const int numParticles = 2;
double x0 = 5, y0 = 5, z0 = 5;
vector<Vec3> positions(numParticles);
System system;
system.addParticle(1.0);
system.addParticle(1.0);
system.setDefaultPeriodicBoxVectors(vx, vy, vz);
// setup force
MeldForce* force = new MeldForce();
force->setUsesPeriodicBoundaryConditions(true);
int k = 1.0;
int restIdx = force->addDistanceRestraint(0,1,1.0,2.0,3.0,4.0,k);
std::vector<int> restIndices(1);
restIndices[0] = restIdx;
int groupIdx = force->addGroup(restIndices,1);
std::vector<int> groupIndices(1);
groupIndices[0] = groupIdx;
force->addCollection(groupIndices, 1);
system.addForce(force);
ASSERT(force->usesPeriodicBoundaryConditions());
// setup context
ASSERT(system.usesPeriodicBoundaryConditions());

VerletIntegrator integrator(0.01);
Platform& platform = Platform::getPlatformByName("CUDA");
Context context(system,integrator, platform); //The real problem comes from here!!!!
// five region and expected force and energy;
// r1=10.5, force=(-1.0,0,0), energy=1.0, 
// r2=11.5, force=(-0.5,0,0), energy=0.125, 
// r3=12.5, force=(0.0,0,0), energy=0.0, 
// r4=13.5, force=(0.5,0,0), energy=0.125, 
// r5=14.5, force=(1.0,0,0), energy=1.0, 

// test region I
positions[0] = Vec3(x0,y0,z0);
positions[1] = Vec3(10.5,5.0,5.0);
context.setPositions(positions);
float expectedEnergy = 1.0;
Vec3 expectedForce = Vec3(-1.0,0.0,0.0);

State stateI = context.getState(State::Positions | State::Energy | State::Forces);  // this line causes segmentation fault.
ASSERT_EQUAL_VEC(expectedForce, stateI.getForces()[0],1e-5);
ASSERT_EQUAL_TOL(expectedEnergy, stateI.getPotentialEnergy(),1e-5);
ASSERT_EQUAL_VEC(-expectedForce, stateI.getForces()[1],1e-5);
std::cout << "Testing regionI:" << stateI.getPositions()[1] << "Success." << std::endl;

// test region II
positions[0] = Vec3(x0,y0,z0);
positions[1] = Vec3(11.5,5.0,5.0);
context.setPositions(positions);
expectedEnergy = 0.125;
expectedForce = Vec3(-0.5,0.0,0.0);

State stateII = context.getState(State::Positions | State::Energy | State::Forces);  // this line causes segmentation fault.
ASSERT_EQUAL_VEC(expectedForce, stateII.getForces()[0],1e-5);
ASSERT_EQUAL_TOL(expectedEnergy, stateII.getPotentialEnergy(),1e-5);
ASSERT_EQUAL_VEC(-expectedForce, stateII.getForces()[1],1e-5);
std::cout << "Testing regionII:" << stateII.getPositions()[1] << "Success." << std::endl;

// test region III
positions[0] = Vec3(x0,y0,z0);
positions[1] = Vec3(12.5,5.0,5.0);
context.setPositions(positions);
expectedEnergy = 0.0;
expectedForce = Vec3(0.0,0.0,0.0);

State stateIII = context.getState(State::Positions | State::Energy | State::Forces);  // this line causes segmentation fault.
ASSERT_EQUAL_VEC(expectedForce, stateIII.getForces()[0],1e-5);
ASSERT_EQUAL_TOL(expectedEnergy, stateIII.getPotentialEnergy(),1e-5);
ASSERT_EQUAL_VEC(-expectedForce, stateIII.getForces()[1],1e-5);
std::cout << "Testing regionIII:" << stateIII.getPositions()[1] << "Success." << std::endl;

// test region VI
positions[0] = Vec3(x0,y0,z0);
positions[1] = Vec3(13.5,5.0,5.0);
context.setPositions(positions);
expectedEnergy = 0.125;
expectedForce = Vec3(0.5,0.0,0.0);

State stateIV = context.getState(State::Positions | State::Energy | State::Forces);  // this line causes segmentation fault.
ASSERT_EQUAL_VEC(expectedForce, stateIV.getForces()[0],1e-5);
ASSERT_EQUAL_TOL(expectedEnergy, stateIV.getPotentialEnergy(),1e-5);
ASSERT_EQUAL_VEC(-expectedForce, stateIV.getForces()[1],1e-5);
std::cout << "Testing regionIV:" << stateIV.getPositions()[1] << "Success." <<std::endl;

// test region V
positions[0] = Vec3(x0,y0,z0);
positions[1] = Vec3(14.5,5.0,5.0);
context.setPositions(positions);
expectedEnergy = 1.0;
expectedForce = Vec3(1.0,0.0,0.0);

State stateV = context.getState(State::Positions | State::Energy | State::Forces);  // this line causes segmentation fault.
ASSERT_EQUAL_VEC(expectedForce, stateV.getForces()[0],1e-5);
ASSERT_EQUAL_TOL(expectedEnergy, stateV.getPotentialEnergy(),1e-5);
ASSERT_EQUAL_VEC(-expectedForce, stateV.getForces()[1],1e-5);
std::cout << "Testing regionV:" << stateV.getPositions()[1] << "Success." << std::endl;
}

void testDistRest() {
    // setup system
    const int numParticles = 2;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    system.addParticle(1.0);

    // setup meld force
    MeldForce* force = new MeldForce();
    int k = 1.0;
    int restIdx = force->addDistanceRestraint(0, 1, 1.0, 2.0, 3.0, 4.0, k);
    std::vector<int> restIndices(1);
    restIndices[0] = restIdx;
    int groupIdx = force->addGroup(restIndices, 1);
    std::vector<int> groupIndices(1);
    groupIndices[0] = groupIdx;
    force->addCollection(groupIndices, 1);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // There are five regions:
    // I:       r < 1
    // II:  1 < r < 2
    // III: 2 < r < 3
    // IV:  3 < r < 4
    // V:   4 < r

    // test region I
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(0.5, 0.0, 0.0);
    context.setPositions(positions);

    float expectedEnergy = 1.0;
    Vec3 expectedForce = Vec3(-1.0, 0.0, 0.0);

    State stateI = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateI.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateI.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateI.getForces()[1], 1e-5);

    // test region II
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(1.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 0.125;
    expectedForce = Vec3(-0.5, 0.0, 0.0);

    State stateII = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateII.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateII.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateII.getForces()[1], 1e-5);

    // test region III
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(2.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 0.0;
    expectedForce = Vec3(0.0, 0.0, 0.0);

    State stateIII = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateIII.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateIII.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateIII.getForces()[1], 1e-5);

    // test region IV
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(3.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 0.125;
    expectedForce = Vec3(0.5, 0.0, 0.0);

    State stateIV = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateIV.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateIV.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateIV.getForces()[1], 1e-5);

    // test region V
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(4.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 1.0;
    expectedForce = Vec3(1.0, 0.0, 0.0);

    State stateV = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateV.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateV.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateV.getForces()[1], 1e-5);
}

void testHyperbolicDistRest() {
    // setup system
    const int numParticles = 2;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    system.addParticle(1.0);

    // setup meld force
    MeldForce* force = new MeldForce();
    float k = 1.0;
    float asymptote = 3.0;

    int restIdx = force->addHyperbolicDistanceRestraint(0, 1, 1.0, 2.0, 3.0, 4.0, k, asymptote);
    std::vector<int> restIndices(1);
    restIndices[0] = restIdx;
    int groupIdx = force->addGroup(restIndices, 1);
    std::vector<int> groupIndices(1);
    groupIndices[0] = groupIdx;
    force->addCollection(groupIndices, 1);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // There are five regions:
    // I:       r < 1
    // II:  1 < r < 2
    // III: 2 < r < 3
    // IV:  3 < r < 4
    // V:   4 < r

    // test region I
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(0.5, 0.0, 0.0);
    context.setPositions(positions);

    float expectedEnergy = 1.0;
    Vec3 expectedForce = Vec3(-1.0, 0.0, 0.0);

    State stateI = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateI.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateI.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateI.getForces()[1], 1e-5);

    // test region II
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(1.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 0.125;
    expectedForce = Vec3(-0.5, 0.0, 0.0);

    State stateII = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateII.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateII.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateII.getForces()[1], 1e-5);

    // test region III
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(2.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 0.0;
    expectedForce = Vec3(0.0, 0.0, 0.0);

    State stateIII = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateIII.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateIII.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateIII.getForces()[1], 1e-5);

    // test region IV
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(3.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 0.250;
    expectedForce = Vec3(1.0, 0.0, 0.0);

    State stateIV = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateIV.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateIV.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateIV.getForces()[1], 1e-5);

    // test region V
    // set the postitions, compute the forces and energy
    // test to make sure they have the expected values
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(4.5, 0.0, 0.0);
    context.setPositions(positions);

    expectedEnergy = 1.666666666;
    expectedForce = Vec3(0.888888888888, 0.0, 0.0);

    State stateV = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, stateV.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce, stateV.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(-expectedForce, stateV.getForces()[1], 1e-5);
}

void testDistRestChangingParameters() {
    // Create particles
    const int numParticles = 2;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    positions[0] = Vec3(0.0, 0.0, 0.0);
    system.addParticle(1.0);
    positions[1] = Vec3(3.5, 0.0, 0.0);

    // Define distance restraint
    MeldForce* force = new MeldForce();
    float k = 1.0;
    int restIdx = force->addDistanceRestraint(0, 1, 1.0, 2.0, 3.0, 4.0, k);
    std::vector<int> restIndices(1);
    restIndices[0] = restIdx;
    int groupIdx = force->addGroup(restIndices, 1);
    std::vector<int> groupIndices(1);
    groupIndices[0] = groupIdx;
    force->addCollection(groupIndices, 1);
    system.addForce(force);

    // Compute the forces and energy.
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);
    context.setPositions(positions);
    State state = context.getState(State::Energy | State::Forces);

    // See if the energy is correct.
    float expectedEnergy = 0.125;
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);

    // Modify the parameters.
    float k2 = 2.0;
    force->modifyDistanceRestraint(0, 0, 1, 1.0, 2.0, 3.0, 4.0, k2);
    force->updateParametersInContext(context);
    state = context.getState(State::Energy);

    // See if the energy is correct after modifying force const.
    expectedEnergy = 0.25;
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
}

void testTorsRest() {
    // Create particles
    const int numParticles = 4;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    positions[0] = Vec3(-3.0, -3.0, 0.0);
    system.addParticle(1.0);
    positions[1] = Vec3(-3.0, 0.0, 0.0);
    system.addParticle(1.0);
    positions[2] = Vec3(3.0, 0.0, 0.0);
    system.addParticle(1.0);
    positions[3] = Vec3(3.0, 3.0, 0.0);

    // Define torsion restraint
    MeldForce* force = new MeldForce();
    float k = 1.0;
    int restIdx = force->addTorsionRestraint(0, 1, 2, 3, 0.0, 0.0, k);
    std::vector<int> restIndices(1);
    restIndices[0] = restIdx;
    int groupIdx = force->addGroup(restIndices, 1);
    std::vector<int> groupIndices(1);
    groupIndices[0] = groupIdx;
    force->addCollection(groupIndices, 1);
    system.addForce(force);

    // Compute the forces and energy.
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);
    context.setPositions(positions);
    State state = context.getState(State::Energy | State::Forces);

    // See if the energy is correct.
    float expectedEnergy = 16200;
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
}

void testDistProfileRest() {
    const int numParticles = 2;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    positions[0] = Vec3(0.0, 0.0, 0.0);
    system.addParticle(1.0);
    positions[1] = Vec3(2.5, 0.0, 0.0);

    MeldForce* force = new MeldForce();
    int nBins = 5;
    int restIdx = 0;
    try {
        std::vector<double> a(nBins);
        for(int i=0; i<a.size(); i++) {
            a[i] = 1.0;
        }
        restIdx = force->addDistProfileRestraint(0, 1, 1.0, 4.0, nBins, a, a, a, a, 1.0);
    }
    catch (std::bad_alloc& ba)
    {
        std::cerr << "bad_alloc caught: " << ba.what() << '\n';
    }
    std::vector<int> restIndices(1);
    restIndices[0] = restIdx;
    int groupIdx = force->addGroup(restIndices, 1);
    std::vector<int> groupIndices(1);
    groupIndices[0] = groupIdx;
    force->addCollection(groupIndices, 1);
    system.addForce(force);

    // Compute the forces and energy.
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);
    context.setPositions(positions);
    State state = context.getState(State::Energy | State::Forces);

    // See if the energy is correct.
    float expectedEnergy = 75.8565;
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
}

void testTorsProfileRest() {
    // Create particles
    const int numParticles = 4;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    positions[0] = Vec3(-3.0, -3.0, 0.0);
    system.addParticle(1.0);
    positions[1] = Vec3(-3.0, 0.0, 0.0);
    system.addParticle(1.0);
    positions[2] = Vec3(3.0, 0.0, 0.0);
    system.addParticle(1.0);
    positions[3] = Vec3(3.0, 3.0, 0.0);

    // Define torsion restraint
    MeldForce* force = new MeldForce();
    int nBins = 5;
    int restIdx = 0;
    try {
        std::vector<double> a(nBins);
        for(int i=0; i<a.size(); i++) {
            a[i] = 1.0;
        }
        restIdx = force->addTorsProfileRestraint(0, 1, 2, 3, 0, 1, 2, 3, nBins, a, a, a, a, a, a, a, a, a, a, a, a, a, a, a, a, 1.0);
    }
    catch (std::bad_alloc& ba)
    {
        std::cerr << "bad_alloc caught: " << ba.what() << '\n';
    }
    std::vector<int> restIndices(1);
    restIndices[0] = restIdx;
    int groupIdx = force->addGroup(restIndices, 1);
    std::vector<int> groupIndices(1);
    groupIndices[0] = groupIdx;
    force->addCollection(groupIndices, 1);
    system.addForce(force);

    // Compute the forces and energy.
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);
    context.setPositions(positions);
    State state = context.getState(State::Energy | State::Forces);

    // See if the energy is correct.
    float expectedEnergy = 1.0;
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
}

void testGroupSelectsCorrectly() {
    // setup system
    const int numParticles = 3;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    system.addParticle(1.0);
    system.addParticle(1.0);

    // setup meld force
    MeldForce* force = new MeldForce();
    int restIdx1 = force->addDistanceRestraint(0, 1, 0.0, 0.0, 3.0, 999.0, 1.0);
    int restIdx2 = force->addDistanceRestraint(1, 2, 0.0, 0.0, 3.0, 999.0, 1.0);

    // setup group
    std::vector<int> group(2);
    group[0] = restIdx1;
    group[1] = restIdx2;
    int groupIdx = force->addGroup(group, 1);

    // setup collection
    std::vector<int> collection(1);
    collection[0] = groupIdx;
    force->addCollection(collection, 1);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // set the positions
    // the first has length 4.0
    // the second has length 5.0
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(4.0, 0.0, 0.0);
    positions[2] = Vec3(9.0, 0.0, 0.0);
    context.setPositions(positions);

    // the expected energy is 0.5 * (4 - 3)**2 = 0.5
    float expectedEnergy = 0.5;

    // the force on atom 1 should be
    // f = - k * (4 - 3) = 1.0
    Vec3 expectedForce1 = Vec3(1.0, 0.0, 0.0);
    Vec3 expectedForce2 = -expectedForce1;
    // should be no force on atom 3
    Vec3 expectedForce3 = Vec3(0.0, 0.0, 0.0);

    State state = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce1, state.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce2, state.getForces()[1], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce3, state.getForces()[2], 1e-5);
}

void testCollectionSelectsCorrectly() {
    // setup system
    const int numParticles = 3;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    system.addParticle(1.0);
    system.addParticle(1.0);

    // setup meld force
    MeldForce* force = new MeldForce();
    int restIdx1 = force->addDistanceRestraint(0, 1, 0.0, 0.0, 3.0, 999.0, 1.0);
    int restIdx2 = force->addDistanceRestraint(1, 2, 0.0, 0.0, 3.0, 999.0, 1.0);

    // setup group1
    std::vector<int> group1(1);
    group1[0] = restIdx1;
    int groupIdx1 = force->addGroup(group1, 1);

    // setup group2
    std::vector<int> group2(1);
    group2[0] = restIdx2;
    int groupIdx2 = force->addGroup(group2, 1);

    // setup collection
    std::vector<int> collection(2);
    collection[0] = groupIdx1;
    collection[1] = groupIdx2;
    force->addCollection(collection, 1);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // set the positions
    // the first has length 4.0
    // the second has length 5.0
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(4.0, 0.0, 0.0);
    positions[2] = Vec3(9.0, 0.0, 0.0);
    context.setPositions(positions);

    // the expected energy is 0.5 * (4 - 3)**2 = 0.5
    float expectedEnergy = 0.5;

    // the force on atom 1 should be
    // f = - k * (4 - 3) = 1.0
    Vec3 expectedForce1 = Vec3(1.0, 0.0, 0.0);
    Vec3 expectedForce2 = -expectedForce1;
    // should be no force on atom 3
    Vec3 expectedForce3 = Vec3(0.0, 0.0, 0.0);

    State state = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce1, state.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce2, state.getForces()[1], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce3, state.getForces()[2], 1e-5);
}


void testSingleGroup() {
    // setup system
    const int numParticles = 3;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    system.addParticle(1.0);
    system.addParticle(1.0);

    // setup meld force
    MeldForce* force = new MeldForce();
    // add restraints between particle 1 and both particles 1 and 2
    int restIdx1 = force->addDistanceRestraint(0, 1, 0.0, 0.0, 0.0, 9999.0, 100.0);
    int restIdx2 = force->addDistanceRestraint(0, 2, 0.0, 0.0, 0.0, 9999.0, 100.0);

    // setup group
    std::vector<int> group(2);
    group[0] = restIdx1;
    group[1] = restIdx2;
    int groupIdx = force->addGroup(group, 2);

    // setup collection
    std::vector<int> collection(1);
    collection[0] = groupIdx;
    force->addCollection(collection, 1);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // set the positions
    // the first spring is stretched by 1 nm
    // the second is stretched by 2 nm
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(1.0, 0.0, 0.0);
    positions[2] = Vec3(2.0, 0.0, 0.0);
    context.setPositions(positions);

    // the expected energy is 0.5 * 100 (1**2 + 2**2) = 0
    float expectedEnergy = 250.0;

    // the force on atom 1 should be
    // f = - 100 * 1 = -100
    Vec3 expectedForce1 = Vec3(-100.0, 0.0, 0.0);
    // the force on atom 2 should be
    // f = -100 * 2 = -200
    Vec3 expectedForce2 = Vec3(-200.0, 0.0, 0.0);
    // the force on atom 0 should be equal and opposite
    Vec3 expectedForce0 = -expectedForce1 - expectedForce2;

    State state = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce0, state.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce1, state.getForces()[1], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce2, state.getForces()[2], 1e-5);
}


void testMultipleGroups() {
    // setup system
    const int numParticles = 3;
    System system;
    vector<Vec3> positions(numParticles);
    system.addParticle(1.0);
    system.addParticle(1.0);
    system.addParticle(1.0);

    // setup meld force
    MeldForce* force = new MeldForce();
    // add restraints between particle 1 and both particles 1 and 2
    int restIdx1 = force->addDistanceRestraint(0, 1, 0.0, 0.0, 0.0, 9999.0, 100.0);
    int restIdx2 = force->addDistanceRestraint(0, 2, 0.0, 0.0, 0.0, 9999.0, 100.0);

    // setup groups
    std::vector<int> group1(1);
    group1[0] = restIdx1;
    int groupIdx1 = force->addGroup(group1, 1);
    std::vector<int> group2(1);
    group2[0] = restIdx2;
    int groupIdx2 = force->addGroup(group2, 1);

    // setup collection
    std::vector<int> collection(2);
    collection[0] = groupIdx1;
    collection[1] = groupIdx2;
    force->addCollection(collection, 2);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // set the positions
    // the first spring is stretched by 1 nm
    // the second is stretched by 2 nm
    positions[0] = Vec3(0.0, 0.0, 0.0);
    positions[1] = Vec3(1.0, 0.0, 0.0);
    positions[2] = Vec3(2.0, 0.0, 0.0);
    context.setPositions(positions);

    // the expected energy is 0.5 * 100 (1**2 + 2**2) = 0
    float expectedEnergy = 250.0;

    // the force on atom 1 should be
    // f = - 100 * 1 = -100
    Vec3 expectedForce1 = Vec3(-100.0, 0.0, 0.0);
    // the force on atom 2 should be
    // f = -100 * 2 = -200
    Vec3 expectedForce2 = Vec3(-200.0, 0.0, 0.0);
    // the force on atom 0 should be equal and opposite
    Vec3 expectedForce0 = -expectedForce1 - expectedForce2;

    State state = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce0, state.getForces()[0], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce1, state.getForces()[1], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce2, state.getForces()[2], 1e-5);
}


void testBigSystem() {
    // setup system
    const int numParticles = 2049;
    System system;
    vector<Vec3> positions(numParticles);
    for(int i=0; i<numParticles; i++) {
        system.addParticle(1.0);
    }

    // setup meld force
    MeldForce* force = new MeldForce();
    std::vector<int> groups;
    for(int i=1; i<numParticles; i++) {
        int restIdx = force->addDistanceRestraint(0, i, 0.0, 0.0, 0.0, 999999., 100.0);
        std::vector<int> group(1);
        group[0] = restIdx;
        int grpIdx = force->addGroup(group, 1);
        groups.push_back(grpIdx);
    }

    // setup collection
    force->addCollection(groups, numParticles / 2);
    system.addForce(force);

    // setup the context
    VerletIntegrator integ(1.0);
    Platform& platform = Platform::getPlatformByName("CUDA");
    Context context(system, integ, platform);

    // set the positions
    // each particle is 1 nm further along the x-axis
    for(int i=0; i<numParticles; i++) {
        positions[i] = Vec3(1.0 * i, 0.0, 0.0);
    }
    context.setPositions(positions);

    float expectedEnergy = 17921920000.0;

    // the force on the 1024th atom should be
    // f = - 100 * 1024 = -102400
    Vec3 expectedForce1024 = Vec3(-102400.0, 0.0, 0.0);
    // the force on the 1025th atom should be zero
    Vec3 expectedForce1025 = Vec3(0.0, 0.0, 0.0);

    State state = context.getState(State::Energy | State::Forces);
    ASSERT_EQUAL_TOL(expectedEnergy, state.getPotentialEnergy(), 1e-5);
    ASSERT_EQUAL_VEC(expectedForce1024, state.getForces()[1024], 1e-5);
    ASSERT_EQUAL_VEC(expectedForce1025, state.getForces()[1025], 1e-5);
}


int main(int argc, char* argv[]) {
// The modified version can run continuiously.
registerMeldCudaKernelFactories();
//if (argc > 1)
//            Platform::getPlatformByName("CUDA").setPropertyDefaultValue("CudaPrecision", string(argv[1]));
//	    testPeriodic();
//	    //testDistRest();
//return 0;
//}
    try {
        registerMeldCudaKernelFactories();
        if (argc > 1)
            Platform::getPlatformByName("CUDA").setPropertyDefaultValue("CudaPrecision", string(argv[1]));
	testPeriodic();	
//	testDistRest();
      //char* test=testDistRest();
      //std::cout << test << std::endl;
     // testDistRestChangingParameters();
     // testHyperbolicDistRest();
     // testTorsRest();
     // testDistProfileRest();
     // testTorsProfileRest();
     // testGroupSelectsCorrectly();
     // testCollectionSelectsCorrectly();
     // testSingleGroup();
     // testMultipleGroups();
     // testBigSystem();
    }
    catch(const std::exception& e) {
        std::cout << "exception: " << e.what() << std::endl;
        return 1;
    }
    std::cout << "Done" << std::endl;
    return 0;
}
