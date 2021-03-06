/****************************************************************/
/* MOOSE - Multiphysics Object Oriented Simulation Environment  */
/*                                                              */
/*          All contents are licensed under LGPL V2.1           */
/*             See LICENSE for full restrictions                */
/****************************************************************/

// Navier-Stokes includes
#include "NS.h"
#include "NSIntegratedBC.h"

// FluidProperties includes
#include "IdealGasFluidProperties.h"

// MOOSE includes
#include "MooseMesh.h"

template <>
InputParameters
validParams<NSIntegratedBC>()
{
  InputParameters params = validParams<IntegratedBC>();

  params.addRequiredCoupledVar(NS::velocity_x, "x-velocity");
  params.addCoupledVar(NS::velocity_y, "y-velocity"); // only required in >= 2D
  params.addCoupledVar(NS::velocity_z, "z-velocity"); // only required in 3D

  params.addRequiredCoupledVar(NS::density, "density");
  params.addRequiredCoupledVar(NS::momentum_x, "x-momentum");
  params.addCoupledVar(NS::momentum_y, "y-momentum"); // only required in >= 2D
  params.addCoupledVar(NS::momentum_z, "z-momentum"); // only required in 3D
  params.addRequiredCoupledVar(NS::total_energy, "total energy");
  params.addRequiredParam<UserObjectName>("fluid_properties", "The name of the user object for fluid properties");

  return params;
}

NSIntegratedBC::NSIntegratedBC(const InputParameters & parameters)
  : IntegratedBC(parameters),
    _u_vel(coupledValue(NS::velocity_x)),
    _v_vel(_mesh.dimension() >= 2 ? coupledValue(NS::velocity_y) : _zero),
    _w_vel(_mesh.dimension() == 3 ? coupledValue(NS::velocity_z) : _zero),

    _rho(coupledValue(NS::density)),
    _rho_u(coupledValue(NS::momentum_x)),
    _rho_v(_mesh.dimension() >= 2 ? coupledValue(NS::momentum_y) : _zero),
    _rho_w(_mesh.dimension() == 3 ? coupledValue(NS::momentum_z) : _zero),
    _rho_E(coupledValue(NS::total_energy)),

    _grad_rho(coupledGradient(NS::density)),
    _grad_rho_u(coupledGradient(NS::momentum_x)),
    _grad_rho_v(_mesh.dimension() >= 2 ? coupledGradient(NS::momentum_y) : _grad_zero),
    _grad_rho_w(_mesh.dimension() == 3 ? coupledGradient(NS::momentum_z) : _grad_zero),
    _grad_rho_E(coupledGradient(NS::total_energy)),

    // Variable numberings
    _rho_var_number(coupled(NS::density)),
    _rhou_var_number(coupled(NS::momentum_x)),
    _rhov_var_number(_mesh.dimension() >= 2 ? coupled(NS::momentum_y) : libMesh::invalid_uint),
    _rhow_var_number(_mesh.dimension() == 3 ? coupled(NS::momentum_z) : libMesh::invalid_uint),
    _rhoE_var_number(coupled(NS::total_energy)),

    _dynamic_viscosity(getMaterialProperty<Real>("dynamic_viscosity")),
    _viscous_stress_tensor(getMaterialProperty<RealTensorValue>("viscous_stress_tensor")),

    // FluidProperties UserObject
    _fp(getUserObject<IdealGasFluidProperties>("fluid_properties"))
{
}

bool
NSIntegratedBC::isNSVariable(unsigned var)
{
  if (var == _rho_var_number ||
      var == _rhou_var_number ||
      var == _rhov_var_number ||
      var == _rhow_var_number ||
      var == _rhoE_var_number)
    return true;
  else
    return false;
}

unsigned
NSIntegratedBC::mapVarNumber(unsigned var)
{
  // Convert the Moose numbering to:
  // 0 for rho
  // 1 for rho*u
  // 2 for rho*v
  // 3 for rho*w
  // 4 for rho*e
  // regardless of the problem dimension, etc.
  unsigned int mapped_var_number;

  if (var == _rho_var_number)
    mapped_var_number = 0;
  else if (var == _rhou_var_number)
    mapped_var_number = 1;
  else if (var == _rhov_var_number)
    mapped_var_number = 2;
  else if (var == _rhow_var_number)
    mapped_var_number = 3;
  else if (var == _rhoE_var_number)
    mapped_var_number = 4;
  else
    mooseError2("Invalid var!");

  return mapped_var_number;
}
