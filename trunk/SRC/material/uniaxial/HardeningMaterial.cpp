/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */

// $Revision: 1.6 $
// $Date: 2002-04-02 23:12:57 $
// $Source: /usr/local/cvs/OpenSees/SRC/material/uniaxial/HardeningMaterial.cpp,v $

// Written: MHS
// Created: May 2000
// Revision: A
//
// Description: This file contains the class implementation for 
// HardeningMaterial. 

#include <HardeningMaterial.h>
#include <Vector.h>
#include <Channel.h>
#include <math.h>
#include <float.h>

HardeningMaterial::HardeningMaterial(int tag, double e, double s,
				     double hi, double hk, double n)
:UniaxialMaterial(tag,MAT_TAG_Hardening),
 E(e), sigmaY(s), Hiso(hi), Hkin(hk), eta(n)
{
	// Initialize variables
    this->revertToStart();
}

HardeningMaterial::HardeningMaterial()
:UniaxialMaterial(0,MAT_TAG_Hardening),
 E(0.0), sigmaY(0.0), Hiso(0.0), Hkin(0.0), eta(0.0)
{
	// Initialize variables
	this->revertToStart();
}

HardeningMaterial::~HardeningMaterial()
{
    // Does nothing
}

int 
HardeningMaterial::setTrialStrain (double strain, double strainRate)
{
    // Set total strain
    Tstrain = strain;
    
    // Elastic trial stress
    Tstress = E * (Tstrain-CplasticStrain);
    
    // Compute trial stress relative to committed back stress
    double xsi = Tstress - CbackStress;

    // Compute yield criterion
    double f = fabs(xsi) - (sigmaY + Hiso*Chardening);
    
    // Elastic step ... no updates required
    if (f <= -DBL_EPSILON * E)
    {
	// Set trial tangent
	Ttangent = E;
    }
    // Plastic step ... perform return mapping algorithm
    else
    {
      double etadt = eta/ops_Dt;

	// Compute consistency parameter
	double dGamma = f / (E+Hiso+Hkin+etadt);

	// Find sign of xsi
	int sign = (xsi < 0) ? -1 : 1;

	// Bring trial stress back to yield surface
	Tstress -= dGamma*E*sign;
	
	// Update plastic strain
	TplasticStrain = CplasticStrain + dGamma*sign;
	
	// Update back stress
	TbackStress = CbackStress + dGamma*Hkin*sign;
	
	// Update internal hardening variable
	Thardening = Chardening + dGamma;
	
	// Set trial tangent
	Ttangent = E*(Hkin+Hiso+etadt) / (E+Hkin+Hiso+etadt);
    }

    return 0;
}

double 
HardeningMaterial::getStress(void)
{
    return Tstress;
}

double 
HardeningMaterial::getTangent(void)
{
    return Ttangent;
}

double 
HardeningMaterial::getStrain(void)
{
    return Tstrain;
}

int 
HardeningMaterial::commitState(void)
{
    // Commit trial history variables
    CplasticStrain = TplasticStrain;
    CbackStress = TbackStress;
    Chardening = Thardening;
    
    return 0;
}

int 
HardeningMaterial::revertToLastCommit(void)
{
	// Nothing to do here
    return 0;
}

int 
HardeningMaterial::revertToStart(void)
{
    // Reset committed history variables
    CplasticStrain = 0.0;
    CbackStress = 0.0;
    Chardening = 0.0;

	// Reset trial history variables
    TplasticStrain = 0.0;
	TbackStress = 0.0;
	Thardening = 0.0;

	// Initialize state variables
	Tstrain = 0.0;
	Tstress = 0.0;
	Ttangent = E;

    return 0;
}

UniaxialMaterial *
HardeningMaterial::getCopy(void)
{
    HardeningMaterial *theCopy =
	new HardeningMaterial(this->getTag(), E, sigmaY, Hiso, Hkin, eta);

    // Copy committed history variables
    theCopy->CplasticStrain = CplasticStrain;
    theCopy->CbackStress = CbackStress;
    theCopy->Chardening = Chardening;

	// Copy trial history variables
    theCopy->TplasticStrain = TplasticStrain;
    theCopy->TbackStress = TbackStress;
    theCopy->Thardening = Thardening;

    // Copy trial state variables
    theCopy->Tstrain = Tstrain;
    theCopy->Tstress = Tstress;
    theCopy->Ttangent = Ttangent;
    
    return theCopy;
}

int 
HardeningMaterial::sendSelf(int cTag, Channel &theChannel)
{
  int res = 0;
  
  static Vector data(9);
  
  data(0) = this->getTag();
  data(1) = E;
  data(2) = sigmaY;
  data(3) = Hiso;
  data(4) = Hkin;
  data(5) = eta;
  data(6) = CplasticStrain;
  data(7) = CbackStress;
  data(8) = Chardening;
  
  res = theChannel.sendVector(this->getDbTag(), cTag, data);
  if (res < 0) 
    cerr << "HardeningMaterial::sendSelf() - failed to send data\n";

  return res;
}

int 
HardeningMaterial::recvSelf(int cTag, Channel &theChannel, 
			       FEM_ObjectBroker &theBroker)
{
  int res = 0;
  
  static Vector data(9);
  res = theChannel.recvVector(this->getDbTag(), cTag, data);
  
  if (res < 0) {
      cerr << "HardeningMaterial::recvSelf() - failed to receive data\n";
      E = 0; 
      this->setTag(0);      
  }
  else {
    this->setTag((int)data(0));
    E = data(1);
    sigmaY = data(2);
    Hiso = data(3);
    Hkin = data(4);
    eta = data(5);
    CplasticStrain = data(6);
    CbackStress = data(7);
    Chardening = data(8);
  }
    
  return res;
}

void 
HardeningMaterial::Print(ostream &s, int flag)
{
    s << "HardeningMaterial, tag: " << this->getTag() << endl;
    s << "  E: " << E << endl;
    s << "  sigmaY: " << sigmaY << endl;
    s << "  Hiso: " << Hiso << endl;
    s << "  Hkin: " << Hkin << endl;
    s << "  eta: " << eta << endl;
}
