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

// $Revision: 1.1 $
// $Date: 2002-12-19 21:06:24 $
// $Source: /usr/local/cvs/OpenSees/SRC/element/forceBeamColumn/HingeRadauBeamIntegration3d.cpp,v $

#include <HingeRadauBeamIntegration3d.h>

#include <Matrix.h>
#include <Vector.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>

HingeRadauBeamIntegration3d::HingeRadauBeamIntegration3d(double e,
							 double a,
							 double iz,
							 double iy,
							 double g,
							 double j,
							 double lpi,
							 double lpj):
  BeamIntegration(BEAM_INTEGRATION_TAG_HingeRadau3d),
  E(e), A(a), Iz(iz), Iy(iy), G(g), J(j), lpI(lpi), lpJ(lpj)
{
  // Nothing to do
}

HingeRadauBeamIntegration3d::HingeRadauBeamIntegration3d():
  BeamIntegration(BEAM_INTEGRATION_TAG_HingeRadau3d),
  E(0.0), A(0.0), Iz(0.0), Iy(0.0), G(0.0), J(0.0), lpI(0.0), lpJ(0.0)
{

}

HingeRadauBeamIntegration3d::~HingeRadauBeamIntegration3d()
{
  // Nothing to do
}

void
HingeRadauBeamIntegration3d::getSectionLocations(int numSections, double L,
						 double *xi)
{
  xi[0] = 0.0;
  xi[1] = 1.0;
  for (int i = 2; i < numSections; i++)
    xi[i] = 0.0;
}

void
HingeRadauBeamIntegration3d::getSectionWeights(int numSections, double L,
					       double *wt)
{
  double oneOverL = 1.0/L;

  wt[0] = lpI*oneOverL;
  wt[1] = lpJ*oneOverL;
  for (int i = 2; i < numSections; i++)
    wt[i] = 1.0;
}

int
HingeRadauBeamIntegration3d::addElasticFlexibility(double L, Matrix &fElastic)
{
  double oneOverL = 1.0/L;

  double Le = L-lpI-lpJ;
  fElastic(0,0) += Le/(E*A);
  fElastic(5,5) += Le/(G*J);

  double x[4];
  double w[4];
  
  static const double eight3 = 8.0/3.0;
  static const double oneOverRoot3 = 1.0/sqrt(3.0);

  double oneOverEIz = 1.0/(E*Iz);
  double oneOverEIy = 1.0/(E*Iy);
  
  x[0] = eight3*lpI;
  w[0] = 3.0*lpI;

  x[1] = L-eight3*lpJ;
  w[1] = 3.0*lpJ;

  Le = L-4.0*(lpI+lpJ);

  x[2] = 4.0*lpI + 0.5*Le*(1.0-oneOverRoot3);
  w[2] = 0.5*Le;

  x[3] = 4.0*lpI + 0.5*Le*(1.0+oneOverRoot3);
  w[3] = w[2];

  double tmpZ = 0.0;
  double tmpY = 0.0;
  double xL, xL1, wt;
  for (int i = 0; i < 4; i++) {
    xL  = x[i]*oneOverL;
    xL1 = xL-1.0;
    wt = w[i]*oneOverEIz;
    fElastic(1,1) += xL1*xL1*wt;
    fElastic(2,2) += xL*xL*wt;
    tmpZ          += xL*xL1*wt;
    wt = w[i]*oneOverEIy;
    fElastic(3,3) += xL1*xL1*wt;
    fElastic(4,4) += xL*xL*wt;
    tmpY          += xL*xL1*wt;
  }
  fElastic(1,2) += tmpZ;
  fElastic(2,1) += tmpZ;
  fElastic(3,4) += tmpY;
  fElastic(4,3) += tmpY;

  return -1;
}

void
HingeRadauBeamIntegration3d::addElasticDeformations(ElementalLoad *theLoad,
						    double loadFactor,
						    double L, double *v0)
{
  return;
}

BeamIntegration*
HingeRadauBeamIntegration3d::getCopy(void)
{
  return new HingeRadauBeamIntegration3d(E, A, Iz, Iy, G, J, lpI, lpJ);
}

int
HingeRadauBeamIntegration3d::sendSelf(int cTag, Channel &theChannel)
{
  static Vector data(8);

  data(0) = E;
  data(1) = A;
  data(2) = Iz;
  data(3) = Iy;
  data(4) = G;
  data(5) = J;
  data(6) = lpI;
  data(7) = lpJ;

  int dbTag = this->getDbTag();

  if (theChannel.sendVector(dbTag, cTag, data) < 0) {
    g3ErrorHandler->warning("HingeRadauBeamIntegration3d::sendSelf() - %s\n",
			    "failed to send Vector data");
    return -1;
  }    

  return 0;
}

int
HingeRadauBeamIntegration3d::recvSelf(int cTag, Channel &theChannel,
				      FEM_ObjectBroker &theBroker)
{
  static Vector data(8);

  int dbTag = this->getDbTag();

  if (theChannel.recvVector(dbTag, cTag, data) < 0)  {
    g3ErrorHandler->warning("HingeRadauBeamIntegration3d::recvSelf() - %s\n",
			    "failed to receive Vector data");
    return -1;
  }
  
  E   = data(0);
  A   = data(1);
  Iz  = data(2);
  Iy  = data(3);
  G   = data(4);
  J   = data(5);
  lpI = data(6);
  lpJ = data(7);

  return 0;
}
  
